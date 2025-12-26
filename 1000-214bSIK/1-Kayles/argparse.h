#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <algorithm>
#include <ctime>
#include <exception>
#include <netinet/in.h>
#include <optional>
#include <tuple>
#include <vector>

#include "messages.h"

enum class Arg { ADDRESS, MESSAGE, PAWN_ROW, PORT, TIMEOUT };

namespace detail {

    template <Arg A> struct ArgType;
    template <> struct ArgType<Arg::ADDRESS> { using type = in_addr_t; };
    template <> struct ArgType<Arg::MESSAGE> { using type = Message; };
    template <> struct ArgType<Arg::PAWN_ROW> { using type = std::vector<bool>; };
    template <> struct ArgType<Arg::PORT> { using type = in_port_t; };
    template <> struct ArgType<Arg::TIMEOUT> { using type = time_t; };

    template <Arg... As>
    using ArgsTuple = std::tuple<typename ArgType<As>::type...>;

    struct ParsingResults {
        std::optional<typename ArgType<Arg::ADDRESS>::type> address;
        std::optional<typename ArgType<Arg::MESSAGE>::type> message;
        std::optional<typename ArgType<Arg::PAWN_ROW>::type> pawn_row;
        std::optional<typename ArgType<Arg::PORT>::type> port;
        std::optional<typename ArgType<Arg::TIMEOUT>::type> timeout;

        std::exception_ptr address_ex;
        std::exception_ptr message_ex;
        std::exception_ptr pawn_row_ex;
        std::exception_ptr port_ex;
        std::exception_ptr timeout_ex;
    };

    ParsingResults parse_all_args(int argc, char* argv[]);

    template <Arg A>
    typename ArgType<A>::type get_arg(const ParsingResults& parsing_results);

} /* namespace detail */

template <Arg... As>
detail::ArgsTuple<As...> parse_args(int argc, char* argv[]) {
    detail::ParsingResults parsing_results = detail::parse_all_args(argc, argv);
    return std::make_tuple(detail::get_arg<As>(parsing_results)...);
}

#endif /* ARGPARSE_H */
