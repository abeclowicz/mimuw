#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <utility>

#include "urlparse.h"

namespace detail {

   std::pair<std::string, std::string> parse_scheme(const std::string& uri_suffix) {
        auto pos = uri_suffix.find("://");

        if (pos == std::string::npos) {
            return { "http", uri_suffix };
        }

        auto scheme = uri_suffix.substr(0, pos);

        std::transform(
            scheme.begin(), scheme.end(), scheme.begin(),
            [](unsigned char c) { return std::tolower(c); }
        );

        if (scheme != "http" && scheme != "https") {
            throw std::invalid_argument(
                std::format("Unsupported URL scheme: {}", scheme)
            );
        }

        return std::make_pair(scheme, uri_suffix.substr(pos + 3));
    }

    std::pair<std::string, std::string> parse_host(const std::string& uri_suffix) {
        if (!uri_suffix.empty()) {
            // IPv6 address
            if (uri_suffix.front() == '[') {
                auto end = uri_suffix.find(']');

                if (end == std::string::npos || end == 1) {
                    throw std::invalid_argument("Invalid IPv6 address");
                }

                return std::make_pair(
                    uri_suffix.substr(0, end + 1), uri_suffix.substr(end + 1)
                );
            }

            auto end = uri_suffix.find_first_of(":/?#");
            auto host = uri_suffix.substr(0, end);

            if (!host.empty()) {
                return {
                    host,
                    end == std::string::npos ? "" : uri_suffix.substr(end)
                };
            }
        }

        throw std::invalid_argument("URL is missing a host");
    }

    std::pair<std::string, std::string> parse_port(const std::string& uri_suffix) {
        if (uri_suffix.empty() || uri_suffix.front() != ':') {
            return std::make_pair("", uri_suffix);
        }

        auto end = uri_suffix.find_first_of("/?#");
        auto port = uri_suffix.substr(1, end - 1);

        if (!port.empty()) {
            int value;
            auto [ptr, ec] = std::from_chars(
                port.data(), port.data() + port.size(), value
            );

            if (ptr == port.data() + port.size() && ec == std::errc()) {
                if (1 <= value && value <= 65535) {
                    return std::make_pair(
                        port,
                        end == std::string::npos ? "" : uri_suffix.substr(end)
                    );
                }
            }
        }

        throw std::invalid_argument("Invalid URL port");
    }

    std::string parse_target(const std::string& uri_suffix) {
        if (uri_suffix.empty() || uri_suffix.front() == '#') {
            return "/";
        }

        if (uri_suffix.front() != '/' && uri_suffix.front() != '?') {
            throw std::invalid_argument("Invalid URL target");
        }

        std::string target = uri_suffix.front() == '?'
            ? "/" + uri_suffix
            : uri_suffix;

        if (auto fragment = target.find('#'); fragment != std::string::npos) {
            target.erase(fragment);
        }

        return target;
    }

} /* namespace detail */

std::tuple<std::string, std::string, std::string, std::string> parse_uri(const std::string& uri) {
    if (uri.empty()) {
        throw std::invalid_argument("Cannot parse an empty URL");
    }

    auto [scheme, suffix1] = detail::parse_scheme(uri);
    auto [host, suffix2] = detail::parse_host(suffix1);
    auto [port, suffix3] = detail::parse_port(suffix2);
    auto target = detail::parse_target(suffix3);

    if (port.empty()) {
        port = (scheme == "https" ? "443" : "80");
    }

    return std::make_tuple(scheme, host, port, target);
}
