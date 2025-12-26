#ifndef HTTP_H
#define HTTP_H

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

class HTTPRequest {
    public:
        HTTPRequest(const std::string& target, const std::string& host,
                    std::optional<std::string> cookie, bool include_metadata);

        std::string to_string() const;

        friend std::ostream& operator<<(std::ostream& os, const HTTPRequest& request);

    private:
        std::string target;
        std::string host;
        std::optional<std::string> cookie;
        bool include_metadata;
};

class HTTPResponse {
    public:
        HTTPResponse(const std::string& response_str);

        int get_status() const;

        std::string get_reason() const;

        std::optional<std::string> get_location() const;

        std::optional<std::string> get_cookie() const;

        std::optional<uint32_t> get_metaint() const;

        friend std::ostream& operator<<(std::ostream& os, const HTTPResponse& response);

    private:
        std::string response_str;
        int status;
        std::string reason;
        std::optional<std::string> location;
        std::optional<std::string> cookie;
        std::optional<uint32_t> metaint;
};

#endif /* HTTP_H */
