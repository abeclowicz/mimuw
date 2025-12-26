#include <format>
#include <regex>
#include <stdexcept>
#include <utility>

#include "http.h"

HTTPRequest::HTTPRequest(const std::string& target, const std::string& host,
                         std::optional<std::string> cookie, bool include_metadata)
    : target{target}
    , host{host}
    , cookie{cookie}
    , include_metadata{include_metadata} {}

std::string HTTPRequest::to_string() const {
    auto cookie_str = cookie.has_value()
        ? std::format("Cookie: {}\r\n", cookie.value())
        : "";

    return std::format(
        "GET {} HTTP/1.1\r\n"
        "Host: {}\r\n"
        "Connection: Keep-Alive\r\n"
        "{}{}" // Cookie and Icy-MetaData (optional)
        "\r\n",
        target, host, cookie_str, (include_metadata ? "Icy-MetaData: 1\r\n" : "")
    );
}

std::ostream& operator<<(std::ostream& os, const HTTPRequest& request) {
    return os << request.to_string();
}

HTTPResponse::HTTPResponse(const std::string& response_str) : response_str{response_str} {
    static const std::regex response_re(
        R"(^(?:HTTP/1\.[01]|ICY) (\d{3})(?: ([^\r\n]*))?\r\n(?:[^:\s]+:[ \t]*[^\r\n]*\r\n)*\r\n$)"
    );

    std::smatch matches;

    if (!std::regex_search(response_str, matches, response_re)) {
        throw std::invalid_argument("invalid HTTP Response format");
    }

    status = std::stoi(matches[1]);
    reason = matches[2].str();

    auto get_header = [&](const std::string& name) -> std::optional<std::string> {
        std::regex re(R"(\r\n)" + name + R"(:[ \t]*([^\r\n]*))", std::regex::icase);

        if (std::regex_search(response_str, matches, re)) {
            return matches[1].str();
        }

        return std::nullopt;
    };

    location = get_header("Location");
    cookie = get_header("Set-Cookie");

    if (auto metaint_str = get_header("icy-metaint"); metaint_str.has_value()) {
        metaint = std::stoul(metaint_str.value());
    }
}

int HTTPResponse::get_status() const {
    return status;
}

std::string HTTPResponse::get_reason() const {
    return reason;
}

std::optional<std::string> HTTPResponse::get_location() const {
    return location;
}

std::optional<std::string> HTTPResponse::get_cookie() const {
    return cookie;
}

std::optional<uint32_t> HTTPResponse::get_metaint() const {
    return metaint;
}

std::ostream& operator<<(std::ostream& os, const HTTPResponse& response) {
    return os << response.response_str;
}
