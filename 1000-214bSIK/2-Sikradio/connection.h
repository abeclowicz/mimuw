#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstddef>
#include <exception>
#include <optional>
#include <stdint.h>
#include <string>
#include <sys/types.h>

#include <openssl/types.h>

#include "http.h"

struct StreamData {
    std::string audio;
    std::string metadata;
};

class ConnectionClosedException : public std::exception {
    public:
        const char* what() const noexcept override;
};

class TCPConnection {
    public:
        TCPConnection(const std::string& url, int address_family, bool include_metadata,
                      int verbosity, SSL_CTX* ctx);

        ~TCPConnection();

        TCPConnection(const TCPConnection&) = delete;

        TCPConnection& operator=(const TCPConnection&) = delete;

        TCPConnection(TCPConnection&&) = default;

        TCPConnection& operator=(TCPConnection&&) = default;

        void reconnect();

        int get_socket_fd() const;

        bool has_buffered_data() const;

        StreamData read_stream() const;

    private:
        int socket_fd = -1;
        SSL* ssl = nullptr;

        mutable uint32_t audio_left = 0;
        mutable uint32_t metadata_left = 0;
        mutable bool last_metadata_block_empty = true;

        struct {
            std::string url;
            int address_family;
            bool include_metadata;
            int verbosity;
            SSL_CTX* ctx;
        } config;

        struct {
            bool is_secure;
            std::optional<uint32_t> icy_metaint;
        } meta;

        void disconnect();

        void handshake(const std::string& host, const std::string& port,
                       int address_family);

        ssize_t send_bytes(const void* buffer, std::size_t n) const;

        ssize_t read_bytes(void* buffer, std::size_t n) const;

        void send_http_request(const HTTPRequest& request) const;

        HTTPResponse read_http_response() const;
};

#endif /* CONNECTION_H */
