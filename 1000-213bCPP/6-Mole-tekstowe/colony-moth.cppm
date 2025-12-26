export module colony:moth;

import <algorithm>;
import <cctype>;
import <concepts>;
import <cstddef>;
import <format>;
import <functional>;
import <memory>;
import <ostream>;
import <stdexcept>;
import <string>;
import <unordered_map>;
import <utility>;

using std::format;
using std::function;
using std::invalid_argument;
using std::isalnum;
using std::isdigit;
using std::isgraph;
using std::islower;
using std::isspace;
using std::isupper;
using std::make_unique;
using std::min;
using std::ostream;
using std::pair;
using std::same_as;
using std::size_t;
using std::string;
using std::unique_ptr;
using std::unordered_map;

/**
 * Enum representing the moth type. Possible values are:
 * - `COMMON`, also given by the `*` character,
 * - `LETTER`, also given by the `A` character,
 * - `DIGIT`, also given by the `1` character,
 * - `PICKY`, also given by the `!` character.
 */
export enum class MothType : char {
    COMMON = '*',
    LETTER = 'A',
    DIGIT = '1',
    PICKY = '!'
};

/**
 * Stores the position, type, vitality and the parameter P of a moth.
 */
export struct MothDetails {
    const size_t position;
    const MothType type;
    const size_t vitality;
    const size_t P;
};

export class Moth {
    private:
        /**
         * Returns `true` if moth eats the `c` character, `false` otherwise.
         */
        virtual bool is_consumable(char c) const noexcept = 0;

        /**
         * Consumes the `c` character, replacing it with a space character.
         */
        void consume(char& c) noexcept;

        /**
         * Returns the character representation of the moth type.
         */
        virtual char type() const noexcept = 0;

    protected:
        size_t position, vitality;
        const size_t P;

        // As this is an abstract class,
        // we make the constructor protected.

        Moth(size_t position, size_t vitality, size_t P);

    public:
        /**
         * Constructs a new moth from unpacked `details` and `text_length`.
         *
         * @throws `std::invalid_argument` if given `details` are invalid.
         */
        static unique_ptr<Moth> from_details(const MothDetails& details,
                                             size_t text_length);

        virtual ~Moth() noexcept;

        /**
         * Clones the `Moth` object.
         */
        virtual unique_ptr<Moth> clone() const = 0;

        /**
         * Returns `true` if moth has a positive vitality, `false` otherwise.
         */
        bool is_active() const noexcept {
            return vitality > 0;
        }

        /**
         * Tries to update moth's position.
         *
         * @returns A `[position, moved]` pair, where:
         *
         * - `position` is the moth`s position after the move,
         *
         * - `moved = true` if the moth did move, `false` otherwise.
         */
        virtual pair<size_t, bool> move() noexcept = 0;

        /**
         * Tries to eat the `c` character.
         */
        void eat(char& c) noexcept;

        /**
         * Writes the textual representation of the state of `moth` to `os`.
         */
        friend ostream& operator<<(ostream& os, const Moth& moth);
};

export class StaticMoveMoth: public Moth {
    private:
        const size_t text_length;

    protected:
        // As this is an abstract class,
        // we make the constructor protected.

        StaticMoveMoth(size_t position, size_t vitality, size_t P,
                       size_t text_length);

    public:
        /**
         * Tries to advance this moth's position by `P`.
         *
         * @returns A `[position, moved]` pair, where:
         *
         * - `position` is the moth`s position after the move,
         *
         * - `moved = true` if the moth did move, `false` otherwise.
         */
        pair<size_t, bool> move() noexcept override;
};

export class CyclicMoveMoth: public Moth {
    private:
        const size_t text_length;
        size_t move_amount = 1;

    protected:
        // As this is an abstract class,
        // we make the constructor protected.

        CyclicMoveMoth(size_t position, size_t vitality, size_t P,
                       size_t text_length);

    public:
        /**
         * Tries to advance moth's position by `1`, `2`, ..., `P`, `1`, `2` etc.
         *
         * @returns A `[position, moved]` pair, where:
         *
         * - `position` is the moth`s position after the move,
         *
         * - `moved = true` if the moth did move, `false` otherwise.
         */
        pair<size_t, bool> move() noexcept override;
};

export class CommonMoth: public StaticMoveMoth {
    private:
        /**
         * Returns the character representation of the `CommonMoth` type.
         */
        char type() const noexcept override {
            return static_cast<char>(MothType::COMMON);
        }

        /**
         * Returns `true` if the `c` character:
         * - has an ASCII code between 33 and 126 (inclusive).
         *
         * Otherwise, returns `false`.
         */
        bool is_consumable(char c) const noexcept override;

    public:
        CommonMoth(size_t position, size_t vitality, size_t P,
                   size_t text_length);

        /**
         * Clones the `CommonMoth` object.
         */
        unique_ptr<Moth> clone() const override;
};

export class LetterMoth: public StaticMoveMoth {
    private:
        /**
         * Returns the character representation of the `LetterMoth` type.
         */
        char type() const noexcept override {
            return static_cast<char>(MothType::LETTER);
        }

        /**
         * Returns `true` if the `c` character:
         * - is a lowercase letter, or
         * - is an uppercase letter.
         *
         * Otherwise, returns `false`.
         */
        bool is_consumable(char c) const noexcept override;

    public:
        LetterMoth(size_t position, size_t vitality, size_t P,
                   size_t text_length);

        /**
         * Clones the `LetterMoth` object.
         */
        unique_ptr<Moth> clone() const override;
};

export class DigitMoth: public StaticMoveMoth {
    private:
        /**
         * Returns the character representation of the `DigitMoth` type.
         */
        char type() const noexcept override {
            return static_cast<char>(MothType::DIGIT);
        }

        /**
         * Returns `true` if the `c` character:
         * - is a digit.
         *
         * Otherwise, returns `false`.
         */
        bool is_consumable(char c) const noexcept override;

    public:
        DigitMoth(size_t position, size_t vitality, size_t P,
                  size_t text_length);

        /**
         * Clones the `DigitMoth` object.
         */
        unique_ptr<Moth> clone() const override;
};

export class PickyMoth: public CyclicMoveMoth {
    private:
        /**
         * Returns the character representation of the `PickyMoth` type.
         */
        char type() const noexcept override {
            return static_cast<char>(MothType::PICKY);
        }

        /**
         * Returns `true` if the `c` character:
         * - has an ASCII code between 33 and 126 (inclusive), and
         * - is not a lowercase letter, and
         * - is not an uppercase letter, and
         * - is not a digit.
         *
         * Otherwise, returns `false`.
         */
        bool is_consumable(char c) const noexcept override;

    public:
        PickyMoth(size_t position, size_t vitality, size_t P,
                  size_t text_length);

        /**
         * Clones the `PickyMoth` object.
         */
        unique_ptr<Moth> clone() const override;
};

namespace detail {
    using Creator = function<unique_ptr<Moth>(const MothDetails&, size_t)>;

    template <typename... Args>
    unordered_map<MothType, Creator> init_map(same_as<MothType> auto... keys) {
        return {
            {keys, [](const MothDetails& d, size_t len) {
                return make_unique<Args>(d.position, d.vitality, d.P, len);
            }}...
        };
    }

    /**
     * Tries to advance moth's `position` by `move_amount`.
     *
     * @returns `true` if the moth did move, `false` otherwise.
     */
    bool move(size_t& position, size_t& vitality, size_t text_length,
              size_t move_amount) {
        // Check if we can afford the cost of moving (10 for each position).
        if (vitality >= 10 * move_amount) {
            position = (position + move_amount) % text_length;
            vitality -= 10 * move_amount;
            return true;
        }
        else {
            vitality = 0;
            return false;
        }
    }

} /* namespace detail */

// Methods that use at least one import (including constructors and destructors)
// are defined below to prevent implicit inlining.

// This avoids linker errors caused by importing modules that
// do not have the full visibility of this module's dependencies.

Moth::Moth(size_t position, size_t vitality, size_t P)
    : position{position}, vitality{vitality}, P{P} {}

unique_ptr<Moth> Moth::from_details(const MothDetails& details,
                                    size_t text_length) {
    static const unordered_map<MothType, detail::Creator> factory =
        detail::init_map<DigitMoth, PickyMoth, CommonMoth, LetterMoth>(
            MothType::DIGIT, MothType::PICKY, MothType::COMMON, MothType::LETTER
        );

    // Check if the given position lies within the text.
    if (details.position >= text_length) {
        const string what = format(
            "MOTH: position {} is out of bounds (text length = {})!",
            details.position, text_length
        );

        throw invalid_argument(what);
    }

    // Check if we received a supported moth type.
    if (auto it = factory.find(details.type); it != factory.end()) {
        return it->second(details, text_length);
    }

    throw invalid_argument("MOTH: moth type not present in ::from_details()!");
}

Moth::~Moth() noexcept = default;

void Moth::consume(char& c) noexcept {
    if (isspace(c)) {
        // Consuming a space character reduces vitality.
        vitality -= min(vitality, static_cast<size_t>(c));
    }
    else {
        vitality += static_cast<size_t>(c);
        c = ' ';
    }
}

void Moth::eat(char& c) noexcept {
    if (isspace(c) || is_consumable(c)) {
        consume(c);
    }
}

ostream& operator<<(ostream& os, const Moth& moth) {
    os << moth.type() << ' '
       << moth.P << ' '
       << moth.position << ' '
       << moth.vitality;

    return os;
}

pair<size_t, bool> StaticMoveMoth::move() noexcept {
    bool moved = detail::move(position, vitality, text_length, P);
    return {position, moved};
}

StaticMoveMoth::StaticMoveMoth(size_t position, size_t vitality, size_t P,
                               size_t text_length)
    : Moth(position, vitality, P), text_length{text_length} {}

pair<size_t, bool> CyclicMoveMoth::move() noexcept {
    if (detail::move(position, vitality, text_length, move_amount)) {
        move_amount = (move_amount % P) + 1; // Update the move amount.
        return {position, true};
    }

    return {position, false};
}

CyclicMoveMoth::CyclicMoveMoth(size_t position, size_t vitality, size_t P,
                               size_t text_length)
    : Moth(position, vitality, P), text_length{text_length} {}

bool CommonMoth::is_consumable(char c) const noexcept {
    return isgraph(c); // All characters (ASCII 33-126).
}

CommonMoth::CommonMoth(size_t position, size_t vitality, size_t P,
                       size_t text_length)
    : StaticMoveMoth(position, vitality, P, text_length) {}

unique_ptr<Moth> CommonMoth::clone() const {
    return make_unique<CommonMoth>(*this);
}

bool LetterMoth::is_consumable(char c) const noexcept {
    // Lowercase and uppercase letters.
    return (islower(c) || isupper(c));
}

LetterMoth::LetterMoth(size_t position, size_t vitality, size_t P,
                       size_t text_length)
    : StaticMoveMoth(position, vitality, P, text_length) {}

unique_ptr<Moth> LetterMoth::clone() const {
    return make_unique<LetterMoth>(*this);
}

bool DigitMoth::is_consumable(char c) const noexcept {
    return isdigit(c); // Digits.
}

DigitMoth::DigitMoth(size_t position, size_t vitality, size_t P,
                     size_t text_length)
    : StaticMoveMoth(position, vitality, P, text_length) {}

unique_ptr<Moth> DigitMoth::clone() const {
    return make_unique<DigitMoth>(*this);
}

bool PickyMoth::is_consumable(char c) const noexcept {
    // Characters other than letters and digits.
    return (isgraph(c) && !isalnum(c));
}

PickyMoth::PickyMoth(size_t position, size_t vitality, size_t P,
                     size_t text_length)
    : CyclicMoveMoth(position, vitality, P, text_length) {}

unique_ptr<Moth> PickyMoth::clone() const {
    return make_unique<PickyMoth>(*this);
}
