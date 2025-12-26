#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <ctime>
#include <format>
#include <iomanip>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <tuple>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "connection.h"
#include "logging.h"
#include "urlparse.h"

namespace detail {

    auto get_current_datetime() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        // Convert to local time
        std::tm datetime;
        localtime_r(&time, &datetime);

        // Format datetime as: YYYY.MM.DD HH.MM.SS
        std::ostringstream oss;
        oss << std::put_time(&datetime, "%Y.%m.%d %H.%M.%S");

        return oss.str();
    }

    auto address_to_str(in_addr_t address, const std::string& port, int address_family) {
        // Create a placeholder string for the address
        std::string address_str(
            address_family == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN,
            '\0'
        );

        inet_ntop(address_family, &address, address_str.data(), address_str.length());

        // Enclose IPv6 addresses in square brackets
        if (address_family == AF_INET6) {
            address_str = '[' + address_str + ']';
        }

        return address_str + ':' + port;
    }

} /* namespace detail */

const char* ConnectionClosedException::what() const noexcept {
    return "server has closed the connection";
}

TCPConnection::TCPConnection(const std::string& url, int address_family,
                             bool include_metadata, int verbosity, SSL_CTX* ctx)
{
    config.url = url;
    config.address_family = address_family;
    config.include_metadata = include_metadata;
    config.verbosity = verbosity;
    config.ctx = ctx;

    meta.is_secure = false;
    meta.icy_metaint = std::nullopt;

    reconnect();
}

TCPConnection::~TCPConnection() {
    disconnect();
}

void TCPConnection::reconnect() {
    Logger::info(detail::get_current_datetime(), config.verbosity);

    auto [scheme, host, port, target] = parse_uri(config.url);
    std::optional<std::string> cookie = std::nullopt;

    while (true) {
        // Establish a TCP connection with the server
        meta.is_secure = (scheme == "https");
        handshake(host, port, config.address_family);

        // Create and send an HTTP request
        HTTPRequest request(target, host, cookie, config.include_metadata);
        send_http_request(request);

        Logger::info(request, config.verbosity);

        // Read the HTTP response
        auto response = read_http_response();

        Logger::info(response, config.verbosity);

        auto status_code = response.get_status();

        // Redirection, parse and follow the next URL
        if (300 <= status_code && status_code < 400) {
            auto location = response.get_location();

            if (!location.has_value()) {
                throw std::runtime_error("redirection response missing the Location header");
            }

            std::tie(scheme, host, port, target) = parse_uri(
                location.value()
            );

            // Check if the response contains cookies for the next request
            cookie = response.get_cookie();

            continue;
        }

        // Drop the invalid connection (1xx/4xx/5xx status)
        if (status_code < 200 || 400 <= status_code) {
            auto error_msg = std::format("{} {}",
                status_code,
                response.get_reason()
            );

            throw std::runtime_error(error_msg);
        }

        if (config.include_metadata) {
            // Try to extract the ICY metadata interval from response headers
            if (auto metaint = response.get_metaint(); metaint.has_value()) {
                meta.icy_metaint = metaint.value();

                // Initialize counters for TCPConnection::read_stream
                audio_left = metaint.value();
                metadata_left = 0;

                last_metadata_block_empty = true;
            }
            else {
                Logger::warn(
                    "metadata requested, but not provided by the server",
                    config.verbosity
                );
            }
        }

        break;
    }
}

int TCPConnection::get_socket_fd() const {
    return socket_fd;
}

bool TCPConnection::has_buffered_data() const {
    // Check for readable bytes buffered in an SSL object
    return meta.is_secure && SSL_has_pending(ssl);
}

StreamData TCPConnection::read_stream() const {
    static char buffer[4096];
    StreamData data;

    auto read = read_bytes(buffer, sizeof(buffer));

    // If there is no metadata, treat the entire buffer as raw audio bytes
    if (!meta.icy_metaint.has_value()) {
        data.audio.assign(buffer, read);
        return data;
    }

    // Otherwise, the stream is in the following (repeating) format:
    // [audio (Icy-MetaInt bytes)] + [size (1 byte)] + [metadata (16*size bytes)]

    for (ssize_t i = 0; i < read; ) {
        // Process the [audio] block
        if (audio_left > 0) {
            auto available = std::min(
                audio_left,
                static_cast<uint32_t>(read - i)
            );

            // Append audio bytes
            data.audio.append(&buffer[i], available);

            audio_left -= available;
            i += available;
        }
        else {
            // Process [size] and [metadata] blocks
            if (metadata_left > 0) {
                auto available = std::min(
                    metadata_left,
                    static_cast<uint32_t>(read - i)
                );

                // Append metadata bytes
                data.metadata.append(&buffer[i], available);

                metadata_left -= available;
                i += available;
            }
            else {
                // Cast to unsigned in case buffer[i++] > 127
                metadata_left = 16 * static_cast<uint8_t>(buffer[i++]);

                // Add newline ('\n') at the end of each [metadata] block
                if (!last_metadata_block_empty) {
                    data.metadata.push_back('\n');
                }

                last_metadata_block_empty = (metadata_left == 0);
            }

            // Reset to the initial state (end of the [metadata] block)
            if (metadata_left == 0) {
                audio_left = meta.icy_metaint.value();
            }
        }
    }

    return data;
}

void TCPConnection::disconnect() {
    if (ssl != nullptr) {
        // Shut down the TLS/SSL connection
        if (SSL_shutdown(ssl) == 0) {
            SSL_shutdown(ssl);
        }

        SSL_free(ssl);
        ssl = nullptr;
    }

    if (socket_fd != -1) {
        Logger::debug("disconnecting..", config.verbosity);

        close(socket_fd);
        socket_fd = -1;

        Logger::debug("done", config.verbosity);
    }
}

void TCPConnection::handshake(const std::string& host, const std::string& port,
                              int address_family)
{
    addrinfo hints{};
    hints.ai_family = address_family;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;

    Logger::info(std::format("resolving name {}", host), config.verbosity);

    auto host_cpy = host;

    // If the host is an IPv6 address, remove the outer square brackets (for getaddrinfo)
    if (host.front() == '[' && host.back() == ']') {
        host_cpy = host.substr(1, host.length() - 2);
    }

    // Resolve a host address
    if (auto ret = getaddrinfo(host_cpy.c_str(), port.c_str(), &hints, &res); ret != 0) {
        throw std::runtime_error(gai_strerror(ret));
    }

    // Ensure any previous connections are closed
    disconnect();

    // Create a new socket
    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    auto address_str = detail::address_to_str(
        reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr.s_addr,
        port,
        res->ai_family
    );

    Logger::info(
        std::format("connecting to server {}", address_str),
        config.verbosity
    );

    // Perform a TCP handshake with the server
    if (connect(socket_fd, res->ai_addr, res->ai_addrlen) < 0) {
        throw std::system_error(errno, std::generic_category());
    }

    freeaddrinfo(res);

    // If the scheme is 'https', wrap the socket with a TLS/SSL layer
    if (meta.is_secure) {
        Logger::debug("initializing a secure connection..", config.verbosity);

        ssl = SSL_new(config.ctx);

        if (ssl == nullptr) {
            throw std::runtime_error("Failed to create an SSL object");
        }

        // Enable SNI (Server Name Indication)
        SSL_set_tlsext_host_name(ssl, host.c_str());

        SSL_set_fd(ssl, socket_fd);

        Logger::debug("connecting..", config.verbosity);

        // Perform a TLS/SSL handshake with the server
        if (auto ret = SSL_connect(ssl); ret <= 0) {
            auto err = SSL_get_error(ssl, ret);
            throw std::runtime_error(ERR_error_string(err, NULL));
        }

        Logger::debug("done", config.verbosity);
    }
}

ssize_t TCPConnection::send_bytes(const void* buffer, std::size_t n) const {
    // Send encrypted bytes to the TLS/SSL connection
    const auto send_secure = [&] {
        auto written = SSL_write(ssl, buffer, n);

        // Handle SSL errors
        if (written <= 0) {
            auto err = SSL_get_error(ssl, written);
            throw std::runtime_error(ERR_error_string(err, NULL));
        }

        return written;
    };

    // Send raw bytes to the socket
    const auto send_raw = [&] {
        auto sent = send(socket_fd, buffer, n, MSG_NOSIGNAL);

        if (sent < 0) {
            // Check if the connection has been closed
            if (errno == EPIPE || errno == ECONNRESET) {
                throw ConnectionClosedException();
            }

            throw std::system_error(errno, std::generic_category());
        }

        return sent;
    };

    return meta.is_secure ? send_secure() : send_raw();
};

ssize_t TCPConnection::read_bytes(void* buffer, std::size_t n) const {
    // Read decrypted bytes from the TLS/SSL connection
    const auto read_secure = [&] {
        auto read = SSL_read(ssl, buffer, n);

        // Handle SSL errors
        if (read <= 0) {
            auto err = SSL_get_error(ssl, read);
            throw std::runtime_error(ERR_error_string(err, NULL));
        }

        return read;
    };

    // Read raw bytes from the socket
    const auto read_raw = [&] {
        auto read_ = read(socket_fd, buffer, n);

        if (read_ <= 0) {
            // Check if the connection has been closed
            if (read_ == 0 || errno == ECONNRESET) {
                throw ConnectionClosedException();
            }

            throw std::system_error(errno, std::generic_category());
        }

        return read_;
    };

    return meta.is_secure ? read_secure() : read_raw();
}

void TCPConnection::send_http_request(const HTTPRequest& request) const {
    auto request_str = request.to_string();

    Logger::debug("sending HTTP request..", config.verbosity);

    // Send the HTTP request (can be sent in chunks, hence the for loop)
    for (std::size_t sent = 0; sent < request_str.size(); ) {
        sent += send_bytes(
            request_str.data() + sent,
            request_str.size() - sent
        );

        Logger::debug(
            std::format("sent {}/{} bytes", sent, request_str.size()),
            config.verbosity
        );
    }

    Logger::debug("done", config.verbosity);
}

HTTPResponse TCPConnection::read_http_response() const {
    std::string response_str;

    Logger::debug("receiving HTTP response..", config.verbosity);

    // Read the HTTP response byte by byte
    while (!response_str.ends_with("\r\n\r\n")) {
        response_str.push_back('\0');
        read_bytes(&response_str.back(), 1);
    }

    Logger::debug(
        std::format("received {} bytes", response_str.size()),
        config.verbosity
    );

    // Parse the received HTTP response
    Logger::debug("parsing..", config.verbosity);
    HTTPResponse response(response_str);

    Logger::debug("done", config.verbosity);
    return response;
}
