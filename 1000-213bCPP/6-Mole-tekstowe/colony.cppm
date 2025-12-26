export module colony;

import <cstddef>;
import <format>;
import <stdexcept>;
import <string>;
import <unordered_map>;
import <utility>;

import :moth;
import :text;

using std::format;
using std::invalid_argument;
using std::out_of_range;
using std::size_t;
using std::string;
using std::unordered_map;

export using MothType = MothType;
export using MothDetails = MothDetails;

export class Colony {
    private:
        unordered_map<size_t, Text> texts;

    public:
        Colony();

        // As the destructor NEEDS to be defined,
        // we define all five (rule of five):

        ~Colony() noexcept;                     // destructor
        Colony(const Colony&);                  // copy constructor
        Colony& operator=(const Colony&);       // copy assignment operator
        Colony(Colony&&) noexcept;              // move constructor
        Colony& operator=(Colony&&) noexcept;   // move assignment operator

        /**
         * Creates a new text (from `text`) with number `n`.
         *
         * @throws `std::invalid_argument` if the number `n` is already taken or
         * if the string `text` is empty.
         */
        void add_text(size_t n, string text);

        /**
         * Constructs a new moth (from `details`) in the text number `n`.
         *
         * @throws `std::out_of_range` if the text does not exist.
         * @throws `std::invalid_argument` if given `details` are invalid.
         */
        void add_moth(size_t n, const MothDetails& details);

        /**
         * Performs `num_cycles` moth feeding cycles on the text number `n`.
         *
         * @throws `std::out_of_range` if the text number `n` does not exist.
         */
        void feed(size_t n, size_t num_cycles);

        /**
         * Returns the text number `n`.
         *
         * @throws `std::out_of_range` if the text number `n` does not exist.
         */
        const Text& operator[](size_t n) const;

        /**
         * Removes the text number `n`.
         *
         * @throws `std::out_of_range` if the text number `n` does not exist.
         */
        void delete_text(size_t n);
};

// Methods that use at least one import (including constructors and destructors)
// are defined below to prevent implicit inlining.

// This avoids linker errors caused by importing modules that
// do not have the full visibility of this module's dependencies.

Colony::Colony() = default;

Colony::~Colony() noexcept = default;
Colony::Colony(const Colony&) = default;
Colony& Colony::operator=(const Colony&) = default;
Colony::Colony(Colony&&) noexcept = default;
Colony& Colony::operator=(Colony&&) noexcept = default;

void Colony::add_text(size_t n, string text) {
    if (texts.contains(n)) {
        const string what = format("TEXT: number {} is already taken!", n);
        throw invalid_argument(what);
    }

    texts.emplace(n, Text(std::move(text)));
}

void Colony::add_moth(size_t n, const MothDetails& details) {
    texts.at(n).add_moth(details);
}

void Colony::feed(size_t n, size_t num_cycles) {
    texts.at(n).feed(num_cycles);
}

const Text& Colony::operator[](size_t n) const {
    return texts.at(n);
}

void Colony::delete_text(size_t n) {
    if (!texts.erase(n)) {
        const string what = format("DELETE: text number {} does not exist!", n);
        throw out_of_range(what);
    }
}
