#ifndef SOCKETS_H
#define SOCKETS_H

#include <ctime>
#include <netinet/in.h>
#include <utility>

#include "messages.h"

namespace detail {

    class Socket {
        public:
            Socket();

            Socket(const Socket&) = delete;

            virtual ~Socket();

            Socket& operator=(const Socket&) = delete;

        protected:
            int socket_fd;
    };

} /* namespace detail */

class ClientSocket : public detail::Socket {
    public:
        ClientSocket(time_t timeout);

        void send(const Message& message, const sockaddr_in& addr) const;

        void send(const Message& message, in_addr_t address, in_port_t port) const;

        State receive() const;
};

class ServerSocket : public detail::Socket {
    public:
        ServerSocket(const sockaddr_in& addr);

        ServerSocket(in_addr_t address, in_port_t port);

        void send(const State& state, const sockaddr_in& addr) const;

        void send(const State& state, in_addr_t address, in_port_t port) const;

        std::pair<Message, sockaddr_in> receive() const;
};

#endif /* SOCKETS_H */
