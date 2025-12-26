#ifndef URLPARSE_H
#define URLPARSE_H

#include <string>
#include <tuple>
#include <variant>

std::tuple<std::string, std::string, std::string, std::string> parse_uri(const std::string& uri);

#endif /* URLPARSE_H */
