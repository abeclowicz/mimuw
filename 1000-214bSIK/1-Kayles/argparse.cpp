#include <algorithm>
#include <arpa/inet.h>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <ctime>
#include <exception>
#include <format>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <vector>

#include "argparse.h"
#include "messages.h"

namespace detail {

    std::vector<std::string> split(const std::string& str, char sep) {
        std::vector<std::string> parts;

        std::stringstream ss(str);
        std::string part;

        while (std::getline(ss, part, sep)) {
            parts.push_back(part);
        }

        return parts;
    }

    template <std::integral T>
    std::optional<T> parse_integer(const std::string& str) {
        T target{};

        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.length(), target);

        if (ptr != str.data() + str.length() || ec != std::errc()) {
            return std::nullopt;
        }

        return target;
    }

    in_addr_t parse_address(const std::string& str) {
        addrinfo hints{};

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        addrinfo* result;

        if (int ret = getaddrinfo(str.c_str(), nullptr, &hints, &result); ret != 0) {
            throw std::runtime_error(gai_strerror(ret));
        }

        auto addr = reinterpret_cast<const sockaddr_in*>(result->ai_addr);

        freeaddrinfo(result);

        return ntohl(addr->sin_addr.s_addr);
    }

    Message parse_message(const std::string& str) {
        PackedMessage packed_message{};

        // Note: message_fields is not empty.
        std::vector<std::string> message_fields = split(str, '/');

        if (auto opt = parse_integer<uint8_t>(message_fields[0]); opt.has_value()) {
            packed_message.msg_type = opt.value();
        }
        else {
            throw std::invalid_argument("Message type must be an 8-bit integer");
        }

        // Note: validate msg_type via Message's constructor.
        Message(packed_message.msg_type, 1, 0, 0);

        std::size_t count = message_field_count(packed_message.msg_type);

        if (message_fields.size() != count) {
            throw std::invalid_argument(std::format("Message must have exactly {} fields", count));
        }

        if (count > 1) {
            if (auto opt = parse_integer<uint32_t>(message_fields[1]); opt.has_value()) {
                packed_message.player_id = opt.value();
            }
            else {
                throw std::invalid_argument("Player ID must be a 32-bit integer");
            }
        }

        if (count > 2) {
            if (auto opt = parse_integer<uint32_t>(message_fields[2]); opt.has_value()) {
                packed_message.game_id = opt.value();
            }
            else {
                throw std::invalid_argument("Game ID must be a 32-bit integer");
            }
        }

        if (count > 3) {
            if (auto opt = parse_integer<uint8_t>(message_fields[3]); opt.has_value()) {
                packed_message.pawn = opt.value();
            }
            else {
                throw std::invalid_argument("Pawn index must be an 8-bit integer");
            }
        }

        // Note: validate message via Message's constructor.
        return Message(packed_message.msg_type, packed_message.player_id, packed_message.game_id,
                       packed_message.pawn);
    }

    std::vector<bool> parse_pawn_row(const std::string& str) {
        std::vector<bool> pawn_row;

        for (const char c : str) {
            if (c != '0' && c != '1') {
                throw std::invalid_argument("Pawn row can only contain '0' and '1' characters");
            }

            pawn_row.push_back(c - '0');
        }

        if (!pawn_row.front() || !pawn_row.back()) {
            throw std::invalid_argument("Both outermost pawns in the pawn row must be '1'");
        }

        // Note: validate pawn_row via State`s constructor.
        State(0, 1, 0, WAITING_FOR_OPPONENT, pawn_row.size() - 1, pawn_row);

        return pawn_row;
    }

    in_port_t parse_port(const std::string& str) {
        if (auto opt = parse_integer<in_port_t>(str); opt.has_value()) {
            return opt.value();
        }

        throw std::invalid_argument("Port must be an integer between 0 and 65535");
    }

    time_t parse_timeout(const std::string& str) {
        if (auto opt = parse_integer<time_t>(str); opt.has_value()) {
            if (time_t timeout = opt.value(); timeout >= 1 && timeout <= 99) {
                return timeout;
            }
        }

        throw std::invalid_argument("Timeout must be an integer between 1 and 99");
    }

    ParsingResults parse_all_args(int argc, char* argv[]) {
        ParsingResults parsing_results{};

        opterr = 0;
        optind = 1;

        static const auto parse = [](auto parser, auto& target, std::exception_ptr& ex) {
            try {
                if (const std::string str = optarg ? std::string(optarg) : ""; !str.empty()) {
                    target = parser(str);
                }
            }
            catch (...) {
                ex = std::current_exception();
            }
        };

        int opt;
        while ((opt = getopt(argc, argv, "a:m:r:p:t:")) != -1) {
            switch (opt) {
                case 'a':
                    parse(parse_address, parsing_results.address, parsing_results.address_ex);
                    break;
                case 'm':
                    parse(parse_message, parsing_results.message, parsing_results.message_ex);
                    break;
                case 'r':
                    parse(parse_pawn_row, parsing_results.pawn_row, parsing_results.pawn_row_ex);
                    break;
                case 'p':
                    parse(parse_port, parsing_results.port, parsing_results.port_ex);
                    break;
                case 't':
                    parse(parse_timeout, parsing_results.timeout, parsing_results.timeout_ex);
                    break;
            }
        }

        return parsing_results;
    }

    template <>
    typename ArgType<Arg::ADDRESS>::type get_arg<Arg::ADDRESS>(const ParsingResults& parsing_results) {
        if (parsing_results.address_ex) {
            std::rethrow_exception(parsing_results.address_ex);
        }

        if (!parsing_results.address.has_value()) {
            throw std::runtime_error("Missing required argument -a (address)");
        }

        return parsing_results.address.value();
    }

    template <>
    typename ArgType<Arg::MESSAGE>::type get_arg<Arg::MESSAGE>(const ParsingResults& parsing_results) {
        if (parsing_results.message_ex) {
            std::rethrow_exception(parsing_results.message_ex);
        }

        if (!parsing_results.message.has_value()) {
            throw std::runtime_error("Missing required argument -m (message)");
        }

        return parsing_results.message.value();
    }

    template <>
    typename ArgType<Arg::PAWN_ROW>::type get_arg<Arg::PAWN_ROW>(const ParsingResults& parsing_results) {
        if (parsing_results.pawn_row_ex) {
            std::rethrow_exception(parsing_results.pawn_row_ex);
        }

        if (!parsing_results.pawn_row.has_value()) {
            throw std::runtime_error("Missing required argument -r (pawn_row)");
        }

        return parsing_results.pawn_row.value();
    }

    template <>
    typename ArgType<Arg::PORT>::type get_arg<Arg::PORT>(const ParsingResults& parsing_results) {
        if (parsing_results.port_ex) {
            std::rethrow_exception(parsing_results.port_ex);
        }

        if (!parsing_results.port.has_value()) {
            throw std::runtime_error("Missing required argument -p (port)");
        }

        return parsing_results.port.value();
    }

    template <>
    typename ArgType<Arg::TIMEOUT>::type get_arg<Arg::TIMEOUT>(const ParsingResults& parsing_results) {
        if (parsing_results.timeout_ex) {
            std::rethrow_exception(parsing_results.timeout_ex);
        }

        if (!parsing_results.timeout.has_value()) {
            throw std::runtime_error("Missing required argument -t (timeout)");
        }

        return parsing_results.timeout.value();
    }

} /* namespace detail */
