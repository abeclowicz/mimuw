#ifndef LOGGING_H
#define LOGGING_H

#include <concepts>
#include <iostream>
#include <string>

template <typename T>
concept Streamable = requires(std::ostream& os, const T& t) {
    { os << t } -> std::same_as<std::ostream&>;
};

class Logger {
    public:
        template <Streamable S>
        static void info(const S& s, int verbosity) {
            log(s, 1, verbosity);
        }

        template <Streamable S>
        static void error(const S& s, int verbosity) {
            log(s, 2, verbosity, "[error]");
        }

        template <Streamable S>
        static void warn(const S& s, int verbosity) {
            log(s, 3, verbosity, "[warn]");
        }

        template <Streamable S>
        static void debug(const S& s, int verbosity) {
            log(s, 4, verbosity, "[debug]");
        }

    private:
        template <Streamable S>
        static void log(const S& s, int level, int verbosity) {
            if (level <= verbosity) {
                std::cerr << s << '\n';
            }
        }

        template <Streamable S>
        static void log(const S& s, int level, int verbosity, const std::string& note) {
            if (level <= verbosity) {
                std::cerr << note << ' ' << s << '\n';
            }
        }
};

#endif /* LOGGING_H */
