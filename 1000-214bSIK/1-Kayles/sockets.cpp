#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <ctime>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/time.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

#include "messages.h"
#include "sockets.h"

namespace detail {

    sockaddr_in make_sockaddr(in_addr_t address, in_port_t port) {
        sockaddr_in addr{};

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(address);
        addr.sin_port = htons(port);

        return addr;
    }

    sockaddr* make_ptr(sockaddr_in& addr) {
        return reinterpret_cast<sockaddr*>(&addr);
    }

    const sockaddr* make_ptr(const sockaddr_in& addr) {
        return reinterpret_cast<const sockaddr*>(&addr);
    }

    void send(int socket_fd, const std::vector<char>& bytes, const sockaddr_in& addr) {
        ssize_t sent = sendto(socket_fd, bytes.data(), bytes.size(), 0, make_ptr(addr), sizeof(addr));

        if (sent < 0) {
            throw std::system_error(errno, std::generic_category());
        }

        if (static_cast<std::size_t>(sent) != bytes.size()) {
            throw std::runtime_error("Incomplete send");
        }
    }

    void send(int socket_fd, const Serializable& serializable, const sockaddr_in& addr) {
        send(socket_fd, serializable.serialize(), addr);
    }

    std::pair<std::vector<char>, sockaddr_in> receive(int socket_fd, size_t max_n) {
        std::vector<char> bytes(max_n);
        sockaddr_in addr{};

        socklen_t len = sizeof(addr);
        ssize_t received = recvfrom(socket_fd, bytes.data(), max_n, 0, make_ptr(addr), &len);

        if (received < 0) {
            throw std::system_error(errno, std::generic_category());
        }

        bytes.resize(received);

        return std::make_pair(bytes, addr);
    }

    Socket::Socket() {
        if (socket_fd = socket(AF_INET, SOCK_DGRAM, 0); socket_fd < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    Socket::~Socket() {
        close(socket_fd);
    }

} /* namespace detail */

ClientSocket::ClientSocket(time_t timeout) {
    const timeval tv {
        .tv_sec = timeout,
        .tv_usec = 0
    };

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        throw std::system_error(errno, std::generic_category());
    }
}

void ClientSocket::send(const Message& message, const sockaddr_in& addr) const {
    detail::send(socket_fd, message, addr);
}

void ClientSocket::send(const Message& message, in_addr_t address, in_port_t port) const {
    send(message, detail::make_sockaddr(address, port));
}

State ClientSocket::receive() const {
    auto [bytes, addr] = detail::receive(socket_fd, State::MAX_SIZE + 1);
    return State::deserialize(bytes);
}

ServerSocket::ServerSocket(const sockaddr_in& addr) {
    if (bind(socket_fd, detail::make_ptr(addr), sizeof(addr)) < 0) {
        throw std::system_error(errno, std::generic_category());
    }
}

ServerSocket::ServerSocket(in_addr_t address, in_port_t port)
    : ServerSocket(detail::make_sockaddr(address, port)) {}

void ServerSocket::send(const State& state, const sockaddr_in& addr) const {
    detail::send(socket_fd, state, addr);
}

void ServerSocket::send(const State& state, in_addr_t address, in_port_t port) const {
    send(state, detail::make_sockaddr(address, port));
}

std::pair<Message, sockaddr_in> ServerSocket::receive() const {
    auto [bytes, addr] = detail::receive(socket_fd, Message::MAX_SIZE + 1);
    return std::make_pair(Message::deserialize(bytes, false), addr);
}
