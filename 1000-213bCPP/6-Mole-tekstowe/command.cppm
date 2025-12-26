export module command;

import <cstddef>;
import <format>;
import <iostream>;
import <regex>;
import <stdexcept>;
import <string>;
import <utility>;

import colony;

using std::cout;
using std::format;
using std::invalid_argument;
using std::regex;
using std::regex_match;
using std::size_t;
using std::smatch;
using std::stoul;
using std::string;

export class Command {
    public:
        virtual ~Command() noexcept = default;

        /**
         * Executes the command on a given `colony`.
         *
         * @throws `std::invalid_argument` or `std::out_of_range` if given
         * arguments are invalid.
         */
        virtual void execute(Colony& colony) const = 0;
};

export class TextCommand : public Command {
    private:
        size_t n;
        string text;

    public:
        static constexpr string command_tag = "TEXT";

        /**
         * Initializes the `TEXT` command from its textual representation.
         *
         * @throws `std::invalid_argument` if `command_str` does not match the
         * command format.
         */
        TextCommand(const string& command_str);

        /**
         * Creates a new text `<TEXT>` with number `<T>`.
         *
         * @throws `std::invalid_argument` if the number `<T>` is already taken
         * or if the string `<TEXT>` is empty.
         */
        void execute(Colony& colony) const override;
};

export class MothCommand : public Command {
    private:
        size_t n, position, vitality, P;
        MothType type;

    public:
        static constexpr string command_tag = "MOTH";

        /**
         * Initializes the `MOTH` command from its textual representation.
         *
         * @throws `std::invalid_argument` if `command_str` does not match the
         * command format.
         */
        MothCommand(const string& command_str);

        /**
         * Constructs a new moth with type `<R>`, vitality `<V>` and the
         * parameter `<P>` in the text number `<T>`.
         *
         * @throws `std::out_of_range` if the text does not exist.
         * @throws `std::invalid_argument` if given parameters are invalid.
         */
        void execute(Colony& colony) const override;
};

export class FeedCommand : public Command {
    private:
        size_t n, num_cycles;

    public:
        static constexpr string command_tag = "FEED";

        /**
         * Initializes the `FEED` command from its textual representation.
         *
         * @throws `std::invalid_argument` if `command_str` does not match the
         * command format.
         */
        FeedCommand(const string& command_str);

        /**
         * Performs `<C>` moth feeding cycles on the text number `<T>`.
         *
         * @throws `std::out_of_range` if the text number `<T>` does not exist.
         */
        void execute(Colony& colony) const override;
};

export class PrintMCommand : public Command {
    private:
        size_t n;

    public:
        static constexpr string command_tag = "PRINTM";

        /**
         * Initializes the `PRINTM` command from its textual representation.
         *
         * @throws `std::invalid_argument` if `command_str` does not match the
         * command format.
         */
        PrintMCommand(const string& command_str);

        /**
         * Prints all moths feeding on the text number `<T>`.
         *
         * @throws `std::out_of_range` if the text number `<T>` does not exist.
         */
        void execute(Colony& colony) const override;
};

export class PrintTCommand : public Command {
    private:
        size_t n;

    public:
        static constexpr string command_tag = "PRINTT";

        /**
         * Initializes the `PRINTT` command from its textual representation.
         *
         * @throws `std::invalid_argument` if `command_str` does not match the
         * command format.
         */
        PrintTCommand(const string& command_str);

        /**
         * Prints the current form of the text number `<T>`.
         *
         * @throws `std::out_of_range` if the text number `<T>` does not exist.
         */
        void execute(Colony& colony) const override;
};

export class DeleteCommand : public Command {
    private:
        size_t n;

    public:
        static constexpr string command_tag = "DELETE";

        /**
         * Initializes the `DELETE` command from its textual representation.
         *
         * @throws `std::invalid_argument` if `command_str` does not match the
         * command format.
         */
        DeleteCommand(const string& command_str);

        /**
         * Removes the text number `<T>`.
         *
         * @throws `std::out_of_range` if the text number `<T>` does not exist.
         */
        void execute(Colony& colony) const override;
};

namespace detail {
    /**
     * Stores the RegEx pattern and the format of a command.
     */
    struct CommandInfo {
        const regex pattern;
        const string format;

        CommandInfo(string pattern_str, string format_str)
            : pattern(std::move(pattern_str)), format(std::move(format_str)) {}
    };

    /**
     * Matches the given command RegEx pattern against `command_str`.
     *
     * @returns The detailed match result.
     *
     * @throws `std::invalid_argument` if `command_str` does not match.
     */
    smatch match(const string& command_str, const CommandInfo& info) {
        smatch match_results;

        // Check if `command_str` matches the regular expression.
        if (!regex_match(command_str, match_results, info.pattern)) {
            const string what = format(
                "TEXT: string '{}' does not match the command format '{}'!",
                command_str, info.format
            );

            throw invalid_argument(what);
        }

        return match_results;
    }

} /* namespace detail */

// Methods that use at least one import (including constructors and destructors)
// are defined below to prevent implicit inlining.

// This avoids linker errors caused by importing modules that
// do not have the full visibility of this module's dependencies.

TextCommand::TextCommand(const string& command_str) {
    // RegEx pattern and the format of a `TEXT` command.
    static const detail::CommandInfo info {
        R"(^TEXT (0|[1-9][0-9]*) ([!-~]+)$)",
        "TEXT <T> <TEXT>"
    };

    const smatch args = detail::match(command_str, info);

    // Parse and initialize `TEXT` command arguments.
    n = stoul(args[1]);
    text = args[2].str();
}

void TextCommand::execute(Colony& colony) const {
    colony.add_text(n, text);
}

MothCommand::MothCommand(const string& command_str) {
    // RegEx pattern and the format of a `MOTH` command.
    static const detail::CommandInfo info {
        R"(^MOTH (0|[1-9][0-9]*) (0|[1-9][0-9]*) (.) ([1-9][0-9]*) ([1-9][0-9]?)$)",
        "MOTH <T> <N> <R> <V> <P>"
    };

    const smatch args = detail::match(command_str, info);

    // Parse and initialize `MOTH` command arguments.
    n = stoul(args[1]);
    position = stoul(args[2]);
    type = static_cast<MothType>(args[3].str()[0]);
    vitality = stoul(args[4]);
    P = stoul(args[5]);
}

void MothCommand::execute(Colony& colony) const {
    const MothDetails details = {position, type, vitality, P};
    colony.add_moth(n, details);
}

FeedCommand::FeedCommand(const string& command_str) {
    // RegEx pattern and the format of a `FEED` command.
    static const detail::CommandInfo info {
        R"(^FEED (0|[1-9][0-9]*) ([1-9][0-9]*)$)",
        "FEED <T> <C>"
    };

    const smatch args = detail::match(command_str, info);

    // Parse and initialize `FEED` command arguments.
    n = stoul(args[1]);
    num_cycles = stoul(args[2]);
}

void FeedCommand::execute(Colony& colony) const {
    colony.feed(n, num_cycles);
}

PrintMCommand::PrintMCommand(const string& command_str) {
    // RegEx pattern and the format of a `PRINTM` command.
    static const detail::CommandInfo info {
        R"(^PRINTM (0|[1-9][0-9]*)$)",
        "PRINTM <T>"
    };

    const smatch args = detail::match(command_str, info);

    // Parse and initialize `PRINTM` command arguments.
    n = stoul(args[1]);
}

void PrintMCommand::execute(Colony& colony) const {
    cout << colony[n];
}

PrintTCommand::PrintTCommand(const string& command_str) {
    // RegEx pattern and the format of a `PRINTT` command.
    static const detail::CommandInfo info {
        R"(^PRINTT (0|[1-9][0-9]*)$)",
        "PRINTT <T>"
    };

    const smatch args = detail::match(command_str, info);

    // Parse and initialize `PRINTT` command arguments.
    n = stoul(args[1]);
}

void PrintTCommand::execute(Colony& colony) const {
    cout << colony[n].get_text() << '\n';
}

DeleteCommand::DeleteCommand(const string& command_str) {
    // RegEx pattern and the format of a `DELETE` command.
    static const detail::CommandInfo info {
        R"(^DELETE (0|[1-9][0-9]*)$)",
        "DELETE <T>"
    };

    const smatch args = detail::match(command_str, info);

    // Parse and initialize `DELETE` command arguments.
    n = stoul(args[1]);
}

void DeleteCommand::execute(Colony& colony) const {
    colony.delete_text(n);
}
