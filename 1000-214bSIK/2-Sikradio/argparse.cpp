#include <charconv>
#include <optional>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

#include "argparse.h"

namespace detail {

    std::optional<int> parse_integer(const std::string& str) {
        int target;

        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), target);

        if (ptr != str.data() + str.size() || ec != std::errc()) {
            return std::nullopt;
        }

        return target;
    }

    int parse_timeout(const std::string& str) {
        if (auto opt = parse_integer(str); opt.has_value()) {
            if (int timeout = opt.value(); timeout >= 100 && timeout <= 100000) {
                return timeout;
            }
        }

        throw std::invalid_argument("Timeout must be an integer between 100 and 100000");
    }

    int parse_verbosity(const std::string& str) {
        if (auto opt = parse_integer(str); opt.has_value()) {
            if (int verbosity = opt.value(); verbosity >= 0 && verbosity <= 4) {
                return verbosity;
            }
        }

        throw std::invalid_argument("Verbosity must be an integer between 0 and 4");
    }

} /* namespace detail */

std::tuple<std::string, bool, int, int, int> parse_args(int argc, char* argv[]) {
    opterr = 0;
    optind = 0;

    // Initialize default arguments
    std::string url;
    bool include_metadata = false;
    int timeout = 5000;
    int address_family = AF_UNSPEC;
    int verbosity = 2;

    // Flags for the '-4' and '-6' options
    bool ipv4 = false;
    bool ipv6 = false;

    int opt;
    while ((opt = getopt(argc, argv, "u:mt:46v:q")) != -1) {
        switch (opt) {
            case 'u':
                url = std::string(optarg);
                break;
            case 'm':
                include_metadata = true;
                break;
            case 't':
                timeout = detail::parse_timeout(std::string(optarg));
                break;
            case '4':
                ipv4 = true;
                break;
            case '6':
                ipv6 = true;
                break;
            case 'v':
                verbosity = detail::parse_verbosity(std::string(optarg));
                break;
            case 'q':
                verbosity = 0;
                break;
            default:
                throw std::invalid_argument("Unknown argument");
        }
    }

    if (url.empty()) {
        throw std::invalid_argument("Missing required argument: -u");
    }

    // Specify the address family if only one IP version is requested
    if (ipv4 ^ ipv6) {
        address_family = ipv4 ? AF_INET : AF_INET6;
    }

    return std::make_tuple(url, include_metadata, timeout, address_family, verbosity);
}
