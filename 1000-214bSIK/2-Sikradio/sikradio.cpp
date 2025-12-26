#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

#include "argparse.h"
#include "connection.h"
#include "logging.h"

using std::cerr;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::cout;
using std::exception;
using std::flush;
using std::generic_category;
using std::max;
using std::runtime_error;
using std::string;
using std::system_error;

int listen_to_music(const string& url, bool include_metadata, int timeout,
                    int address_family, int verbosity)
{
    // Get the deadline for the next message from the server
    const auto get_deadline = [&timeout] {
        return steady_clock::now() + milliseconds(timeout);
    };

    // Return the number of milliseconds until the deadline
    // Note: Returns 0 if the deadline has already passed
    const auto until_deadline = [](auto deadline) {
        auto diff = duration_cast<milliseconds>(deadline - steady_clock::now());
        return max(diff.count(), 0L);
    };

    try {
        // Create the SSL context (ciphers, settings, certificates, etc.)
        auto method = TLS_client_method();
        auto ctx = SSL_CTX_new(method);

        if (ctx == nullptr) {
            auto err = ERR_get_error();
            throw runtime_error(ERR_error_string(err, NULL));
        }

        // Connect to the server, complete the HTTP handshake and set the deadline
        TCPConnection connection(url, address_family, include_metadata, verbosity, ctx);
        auto deadline = get_deadline();

        pollfd fds[2];

        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;

        fds[1].fd = connection.get_socket_fd();
        fds[1].events = POLLIN;

        bool quit = false;

        do {
            // If there is any buffered data, poll with no timeout (non-blocking)
            // Note: See the code below for more details
            int ret = poll(fds, 2, connection.has_buffered_data()
                ? 0
                : until_deadline(deadline)
            );

            if (ret < 0) {
                throw system_error(errno, generic_category());
            }

            if (ret == 0) {
                // Poll only has a 'real' timeout if there was no buffered data
                // Note: see the code above for more details
                if (!connection.has_buffered_data()) {
                    Logger::info("data receiving timeout", verbosity);

                    // Reconnect and update the deadline
                    connection.reconnect();
                    deadline = get_deadline();

                    // Update the file descriptor (a new connection means a new socket)
                    fds[1].fd = connection.get_socket_fd();

                    continue;
                }
            }

            // Handle events on the standard input
            if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
                static string input_str;
                static char buffer[5];

                auto n = read(fds[0].fd, buffer, sizeof(buffer));

                if (n > 0) {
                    input_str.append(buffer, n);

                    // Check whether the exit command has been entered
                    quit = input_str.contains("quit\n");

                    // Keep only the last 4 characters to detect "quit\n"
                    // across multiple reads
                    if (input_str.length() > 4) {
                        input_str.erase(0, input_str.length() - 4);
                    }
                }
                else if (n == 0) {
                    // EOF, stop polling
                    fds[0].fd = -1;
                }
            }

            // Handle events on the server connection
            if (!quit && (
                (fds[1].revents & (POLLIN | POLLHUP | POLLERR)) ||
                connection.has_buffered_data()
            )) {
                StreamData response = connection.read_stream();

                // If there was new (unbuffered) data, update the deadline
                if (fds[1].revents & POLLIN) {
                    deadline = get_deadline();
                }

                cout.write(response.audio.data(), response.audio.size()) << flush;
                cerr.write(response.metadata.data(), response.metadata.size()) << flush;
            }

        } while (!quit);
    }
    catch (const ConnectionClosedException& ex) {
        Logger::warn(ex.what(), verbosity);
        return 0;
    }
    catch (const exception& ex) {
        Logger::error(ex.what(), verbosity);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    try {
        auto [url, meta, timeout, version, verbosity] = parse_args(argc, argv);

        // Initialize the OpenSSL library
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        return listen_to_music(url, meta, timeout, version, verbosity);
    }
    catch (const exception& ex) {
        Logger::error(ex.what(), INT_MAX);
        return 1;
    }
}
