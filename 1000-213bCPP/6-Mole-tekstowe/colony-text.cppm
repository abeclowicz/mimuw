export module colony:text;

import <cstddef>;
import <list>;
import <memory>;
import <ostream>;
import <stdexcept>;
import <string>;
import <utility>;

import :moth;

using std::invalid_argument;
using std::list;
using std::ostream;
using std::size_t;
using std::string;
using std::swap;
using std::unique_ptr;

export class Text {
    private:
        string text;
        list<unique_ptr<Moth>> moths_ptrs;
        list<Moth*> active_moths_ptrs;

    public:
        Text(string text);

        // As both destructor and the copy constructor NEED to be defined,
        // we define all five (rule of five):

        ~Text() noexcept;                       // destructor
        Text(const Text& other);                // copy constructor
        Text& operator=(Text other) noexcept;   // copy/move assignment operator
        Text(Text&&) noexcept;                  // move constructor

        /**
         * Constructs a new moth (from `details`) in this text.
         *
         * @throws `std::invalid_argument` if given `details` are invalid.
         * @throws `std::bad_alloc` if the memory allocation failed.
         */
        void add_moth(const MothDetails& details);

        /**
         * Performs `num_cycles` moth feeding cycles on this text.
         */
        void feed(size_t num_cycles) noexcept;

        /**
         * Returns the current form of this text.
         */
        const string& get_text() const noexcept;

        /**
         * Writes the textual representation of the state of `text` to `os`.
         */
        friend ostream& operator<<(ostream& os, const Text& text);
};

// Methods that use at least one import (including constructors and destructors)
// are defined below to prevent implicit inlining.

// This avoids linker errors caused by importing modules that
// do not have the full visibility of this module's dependencies.

Text::Text(string text) : text(std::move(text)) {
    if (this->text.empty()) {
        throw invalid_argument("TEXT: Cannot create an empty text!");
    }
}

Text::~Text() noexcept = default;

Text::Text(const Text& other) : text(other.text) {
    // Deep-copy `moths_ptrs` and reconstruct `active_moths_ptrs`.
    for (const auto& moth_ptr : other.moths_ptrs) {
        moths_ptrs.push_back(moth_ptr->clone());

        if (moth_ptr->is_active()) {
            active_moths_ptrs.push_back(moths_ptrs.back().get());
        }
    }
}

Text& Text::operator=(Text other) noexcept {
    swap(this->text, other.text);
    swap(this->moths_ptrs, other.moths_ptrs);
    swap(this->active_moths_ptrs, other.active_moths_ptrs);

    return *this;
}

Text::Text(Text&&) noexcept = default;

void Text::add_moth(const MothDetails& details) {
    moths_ptrs.push_back(Moth::from_details(details, text.length()));

    if (details.vitality > 0) {
        // Strong exception guarantee.
        try {
            active_moths_ptrs.push_back(moths_ptrs.back().get());
        }
        catch (...) {
            moths_ptrs.pop_back();
            throw;
        }
    }
}

void Text::feed(size_t num_cycles) noexcept {
    while (!active_moths_ptrs.empty() && num_cycles--) {
        auto it = active_moths_ptrs.begin();

        while (it != active_moths_ptrs.end()) {
            Moth* moth_ptr = *it;
            auto [position, moved] = moth_ptr->move();

            // Moth may have lost its vitality
            // while trying to move.
            if (moved) {
                moth_ptr->eat(text[position]);
            }

            // Moth may have lost its vitality
            // after (possibly) eating a space character.
            if (moth_ptr->is_active()) {
                ++it;
            }
            else {
                it = active_moths_ptrs.erase(it);
            }
        }
    }
}

const string& Text::get_text() const noexcept {
    return text;
}

ostream& operator<<(ostream& os, const Text& text) {
    for (const auto& moth_ptr : text.moths_ptrs) {
        os << *moth_ptr << '\n';
    }

    return os;
}
