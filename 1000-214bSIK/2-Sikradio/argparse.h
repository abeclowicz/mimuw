#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <string>
#include <tuple>
#include <variant>

std::tuple<std::string, bool, int, int, int> parse_args(int argc, char* argv[]);

#endif /* ARGPARSE_H */
