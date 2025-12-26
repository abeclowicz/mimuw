export module reader;

import <algorithm>;
import <cstddef>;
import <concepts>;
import <format>;
import <functional>;
import <ios>;
import <iostream>;
import <memory>;
import <stdexcept>;
import <string>;
import <unordered_map>;

import colony;
import command;

using std::cerr;
using std::cin;
using std::derived_from;
using std::format;
using std::function;
using std::getline;
using std::invalid_argument;
using std::ios_base;
using std::istream;
using std::make_unique;
using std::max;
using std::min;
using std::size_t;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::convertible_to;

/**
 * Handles input parsing and command dispatching for the moth colony simulation.
 *
 * The `Reader` reads data line-by-line from the `input` stream.
 *
 * It is responsible for recognizing command tags and initializing the
 * appropriate `Command` objects.
 *
 * All commands must be separated by exactly one newline. Extra whitespace
 * (leading, inside, trailing) is not allowed.
 *
 * Supported command formats:
 *
 * - `TEXT <T> <TEXT>`
 * Creates a new text `<TEXT>` with number `<T>`.
 *
 * - `MOTH <T> <N> <R> <V> <P>`
 * Constructs a new moth with type `<R>`, vitality `<V>` and the parameter `<P>`
 * in the text number `<T>`.
 *
 * - `FEED <T> <C>`
 * Performs `<C>` moth feeding cycles on the text number `<T>`.
 *
 * - `PRINTM <T>`
 * Prints all moths feeding on the text number `<T>`.
 *
 * - `PRINTT <T>`
 * Prints the current form of the text number `<T>`.
 *
 * - `DELETE <T>`
 * Removes the text number `<T>`.
 */
export class Reader {
    private:
        Colony colony;
        istream& input;

    public:
        Reader(istream& input = cin);

        /**
         * Starts the moth colony simulation.
         *
         * @throws `std::ios_base::failure` if the stream gets closed or
         * encounters an error before EOF.
         */
        void start();
};

namespace detail {
    using Creator = function<unique_ptr<Command>(const string&)>;
    using command_map_t = unordered_map<string, Creator>;

    /**
     * Structure that contains:
     * - mapped constructors for `Command` classes, and
     * - the maximum length of their tags (for `Reader::start()` optimization).
     */
    struct CommandRegistry {
        command_map_t map;
        size_t max_tag_len;
    };

    /**
     * Concept that requires typename `T` to:
     * - be derived from the `Command` class, and
     * - have a `command_tag` attribute.
     *
     * Used to properly initialize `mapped_commands` in `Reader::start()`.
     */
    template <typename T>
    concept CommandWithSizedTag =
        derived_from<T, Command> &&
        requires {
            { T::command_tag } -> convertible_to<string>;
            { T::command_tag.size() } -> convertible_to<size_t>;
        };

    /**
     * Registers command types given as template parameters.
     *
     * For each type `Args`, the function maps its static `command_tag` to a
     * factory function that creates the command from a `std::string`.
     *
     * The returned registry contains a `std::map` from command tags to command
     * factory functions, and the maximum length of all command tags, used to
     * optimize `Reader::start()`.
     *
     * All template parameters must satisfy `CommandWithSizedTag`.
     */
    template <CommandWithSizedTag... Args>
    CommandRegistry register_commands() {
        return {
            {{
                Args::command_tag,
                [](const string& s) { return make_unique<Args>(s); }
            }...},
            max({Args::command_tag.size()...})
        };
    }

} /* namespace detail */

// Methods that use at least one import (including constructors and destructors)
// are defined below to prevent implicit inlining.

// This avoids linker errors caused by importing modules that
// do not have the full visibility of this module's dependencies.

Reader::Reader(istream& input) : input(input) {}

void Reader::start(){
    static const detail::CommandRegistry registry = detail::register_commands<
        TextCommand, MothCommand, FeedCommand,
        PrintMCommand, PrintTCommand, DeleteCommand
    >();

    static const detail::command_map_t& mapped_commands = registry.map;

    string line;
    size_t line_number = 0;

    while (getline(input, line) && ++line_number) {
        try {
            // Optimization: only search within the first `length` characters.
            const size_t length = min(registry.max_tag_len + 1, line.size());

            // Try to parse the command tag (`TEXT`, `MOTH` etc.).
            const string window = line.substr(0, length);
            const size_t space_pos = window.find_first_of(' ');

            if (space_pos == string::npos) {
                const string what = format(
                    "Command '{}' too long or missing a space delimeter!", line
                );

                throw invalid_argument(what);
            }

            // Find the appropriate `Command` class and initialize its object.
            const string key = window.substr(0, space_pos);
            unique_ptr<Command> command = mapped_commands.at(key)(line);

            command->execute(colony);
        } catch (...) {
            cerr << "ERROR " << line_number << '\n';
        }
    }

    if (!input.eof()) {
        throw ios_base::failure("Input stream not ending with EOF!");
    }
}
