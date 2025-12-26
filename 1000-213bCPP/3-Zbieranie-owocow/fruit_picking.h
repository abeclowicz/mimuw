#ifndef FRUIT_PICKING_H
#define FRUIT_PICKING_H

#include <algorithm>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <list>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

using std::advance;
using std::derived_from;
using std::forward;
using std::get;
using std::initializer_list;
using std::is_const_v;
using std::list;
using std::make_move_iterator;
using std::min;
using std::move;
using std::multiset;
using std::ostream;
using std::prev;
using std::remove_cvref_t;
using std::remove_reference_t;
using std::size_t;
using std::string;
using std::string_view;
using std::tuple;
using std::unordered_map;
using std::weak_ordering;

enum class Taste { SWEET, SOUR };
enum class Size { LARGE, MEDIUM, SMALL };
enum class Quality { HEALTHY, ROTTEN, WORMY };

namespace hidden {
    using fruit_tuple_t = tuple<Taste, Size, Quality>;
}

class Fruit {
    private:
        Taste taste_;
        Size size_;
        Quality quality_;

        /// Returns a string representation of the fruit's taste.
        constexpr string taste_str() const {
            switch (taste_) {
                case Taste::SWEET: return "słodki";
                case Taste::SOUR: return "kwaśny";
            }

            return "";
        }

        /// Returns a string representation of the fruit's size.
        constexpr string size_str() const {
            switch (size_) {
                case Size::LARGE: return "duży";
                case Size::MEDIUM: return "średni";
                case Size::SMALL: return "mały";
            }

            return "";
        }

        /// Returns a string representation of the fruit's quality.
        constexpr string quality_str() const {
            switch (quality_) {
                case Quality::HEALTHY: return "zdrowy";
                case Quality::ROTTEN: return "nadgniły";
                case Quality::WORMY: return "robaczywy";
            }

            return "";
        }

    public:
        constexpr Taste taste() const noexcept {
            return taste_;
        }

        constexpr Size size() const noexcept {
            return size_;
        }

        constexpr Quality quality() const noexcept {
            return quality_;
        }

        constexpr Fruit() = delete;

        explicit constexpr Fruit(const Taste& t, const Size& s,
                                 const Quality& q)
            : taste_(t), size_(s), quality_(q) {}

        constexpr Fruit(const Fruit& fruit) = default;

        constexpr Fruit(Fruit&& fruit) noexcept = default;

        /// Constructs a fruit from a tuple of (`Taste`, `Size`, `Quality`).
        explicit constexpr Fruit(const hidden::fruit_tuple_t& tp) noexcept
            : Fruit(get<0>(tp), get<1>(tp), get<2>(tp)) {}

        /// Converts the fruit into a tuple of (`Taste`, `Size`, `Quality`).
        explicit constexpr operator hidden::fruit_tuple_t() const {
            return hidden::fruit_tuple_t{taste_, size_, quality_};
        }

        constexpr Fruit& operator=(const Fruit& fruit) = default;
        
        constexpr Fruit& operator=(Fruit&& fruit) noexcept = default;

        /// Makes a healthy fruit rotten.
        constexpr void go_rotten() {
            if (quality_ == Quality::HEALTHY)
                quality_ = Quality::ROTTEN;
        }

        /// Infests a healthy fruit with worms.
        constexpr void become_worm_infested() {
            if (quality_ == Quality::HEALTHY)
                quality_ = Quality::WORMY;
        }

        constexpr bool operator==(const Fruit& other) const = default;

        constexpr auto operator<=>(const Fruit& other) const = delete;

        friend ostream& operator<<(ostream& os, const Fruit& fruit) {
            string repr = "["
              + fruit.taste_str() + " "
              + fruit.size_str() + " "
              + fruit.quality_str() + "]";
            
            return os << repr;
        }
};

constexpr inline Fruit YUMMY_ONE(
    Taste::SWEET,
    Size::LARGE,
    Quality::HEALTHY);

constexpr inline Fruit ROTTY_ONE(
    Taste::SOUR,
    Size::SMALL,
    Quality::ROTTEN);

class Picker {
    private:
        string name_;
        list<Fruit> picked_fruits_;

        unordered_map<Taste, size_t> cnt_taste_;
        unordered_map<Size, size_t> cnt_size_;
        unordered_map<Quality, size_t> cnt_quality_;

        /// Increments counters for taste, size and quality of a given `fruit`.
        void count_added_fruit(const Fruit& fruit) {
            cnt_taste_[fruit.taste()]++;
            cnt_size_[fruit.size()]++;
            cnt_quality_[fruit.quality()]++;
        }

        /// Decrements counters for taste, size and quality of a given `fruit`.
        void count_removed_fruit(const Fruit& fruit) {
            cnt_taste_[fruit.taste()]--;
            cnt_size_[fruit.size()]--;
            cnt_quality_[fruit.quality()]--;
        }

        static bool is_sweet(const Fruit& fruit) {
            return fruit.taste() == Taste::SWEET;
        }

        static bool iw_healthy(const Fruit& fruit) {
            return fruit.quality() == Quality::HEALTHY;
        }

        static bool is_rotten(const Fruit& fruit) {
            return fruit.quality() == Quality::ROTTEN;
        }

        static bool is_wormy(const Fruit& fruit) {
            return fruit.quality() == Quality::WORMY;
        }

    public:
        explicit Picker(string_view name = {})
            : name_(name.empty() ? string("Anonim") : string(name)) {}
    
        Picker(const Picker&) = default;

        Picker(Picker&&) noexcept = default;

        Picker& operator=(const Picker&) = default;

        Picker& operator=(Picker&&) noexcept = default;

        string get_name() const {
            return name_;
        }

        template <typename T>
            requires derived_from<remove_cvref_t<T>, Fruit>
        Picker& operator+=(T&& fruit) {
            // Add the picked fruit and update its count.
            picked_fruits_.push_back(forward<T>(fruit));
            count_added_fruit(picked_fruits_.back());

            if (picked_fruits_.size() >= 2) {
                Fruit& last = picked_fruits_.back();
                Fruit& prev_last = *prev(picked_fruits_.end(), 2);

                // If the new fruit is healthy and the previous one was rotten,
                // the new fruit becomes rotten.
                if (iw_healthy(last) && is_rotten(prev_last)) {
                    count_removed_fruit(last);
                    last.go_rotten();
                    count_added_fruit(last);
                }

                // If the new fruit is rotten and the previous one was healthy,
                // the previous fruit becomes rotten.
                else if (is_rotten(last) && iw_healthy(prev_last)) {
                    count_removed_fruit(prev_last);
                    prev_last.go_rotten();
                    count_added_fruit(prev_last);
                }

                // If the new fruit is wormy, all previously collected healthy
                // and sweet fruits become wormy.
                else if (is_wormy(last)) {
                    for (Fruit& f : picked_fruits_)
                        if (iw_healthy(f) && is_sweet(f)) {
                            count_removed_fruit(f);
                            f.become_worm_infested();
                            count_added_fruit(f);
                        }
                }
            }

            return *this;
        }

        template <typename T>
        requires derived_from<remove_cvref_t<T>, Picker>
              && (!is_const_v<remove_reference_t<T>>)
        Picker& operator+=(T&& other) {
            // Skip if trying to steal from self or if `other` has no fruits.
            if (this == &other || other.picked_fruits_.empty())
                return *this;

            Fruit stolen = move(other.picked_fruits_.front());

            // Remove the stolen fruit from `other` and update its count.
            other.picked_fruits_.pop_front();
            other.count_removed_fruit(stolen);

            return *this += stolen;
        }

        template <typename T>
            requires derived_from<remove_cvref_t<T>, Picker>
                  && (!is_const_v<remove_reference_t<T>>)
        Picker& operator-=(T&& other) {
            other += *this;
            return *this;
        }

        /// Returns the total number of picked fruits.
        size_t count_fruits() const noexcept {
            return picked_fruits_.size();
        }

        /// Returns the number of picked fruits that have a given `taste`.
        size_t count_taste(const Taste& taste) const noexcept {
            auto it = cnt_taste_.find(taste);
            return (it != cnt_taste_.end()) ? it->second : 0;
        }

        /// Returns the number of picked fruits that have a given `size`.
        size_t count_size(const Size& size) const noexcept {
            auto it = cnt_size_.find(size);
            return (it != cnt_size_.end()) ? it->second : 0;
        }

        /// Returns the number of picked fruits that have a given `quality`.
        size_t count_quality(const Quality& quality) const noexcept {
            auto it = cnt_quality_.find(quality);
            return (it != cnt_quality_.end()) ? it->second : 0;
        }

        /// Sorts pickers in descending order - less means a better picker.
        weak_ordering operator<=>(const Picker& other) const {
            if (auto c = other.count_quality(Quality::HEALTHY)
                           <=> count_quality(Quality::HEALTHY); c != 0) 
                return c;

            if (auto c = other.count_taste(Taste::SWEET) 
                            <=> count_taste(Taste::SWEET); c != 0)
                return c;

            if (auto c = other.count_size(Size::LARGE)
                            <=> count_size(Size::LARGE); c != 0)
                return c;

            if (auto c = other.count_size(Size::MEDIUM)
                            <=> count_size(Size::MEDIUM); c != 0)
                return c;

            if (auto c = other.count_size(Size::SMALL)
                            <=> count_size(Size::SMALL); c != 0)
                return c;

            if (auto c = other.count_fruits()
                            <=> count_fruits(); c != 0)
                return c;

            return weak_ordering::equivalent;
        }

        bool operator==(const Picker& other) const {
            return name_ == other.name_ 
                && picked_fruits_ == other.picked_fruits_;
        }

        friend ostream& operator<<(ostream& os, const Picker& picker) {
            os << picker.get_name() << ':';

            // List each picked fruit on a separate tab-indented line.
            // No newline (LF) on the last line.
            for (const Fruit& fruit : picker.picked_fruits_) {
                os << "\n\t" << fruit;
            }

            return os;
        }
};

class Ranking {
    private:
        multiset<Picker> pickers_;

    public:
        Ranking() = default;

        /// Constructs a ranking from a list of pickers.
        Ranking(initializer_list<Picker> pickers_list)
            : pickers_(pickers_list) {}

        Ranking(const Ranking& ranking) = default;

        Ranking(Ranking&& ranking) noexcept = default;

        Ranking& operator=(const Ranking& ranking) = default;

        Ranking& operator=(Ranking&& ranking) noexcept = default;

        template <typename T>
            requires derived_from<remove_cvref_t<T>, Picker>
        Ranking& operator+=(T&& picker) {
            pickers_.insert(forward<T>(picker));
            return *this;
        }

        /// Removes the first occurence of `picker` from the ranking.
        Ranking& operator-=(const Picker& picker) {
            // Find the range of possible positions for the `picker`.
            auto it_begin = pickers_.lower_bound(picker);
            auto it_end = pickers_.upper_bound(picker);

            // Iterate through the range and erase the first match.
            for (auto it = it_begin; it != it_end; ++it)
                if (*it == picker) {
                    pickers_.erase(it);
                    break;
                }

            return *this;
        }

        constexpr Ranking& operator+=(const Ranking& other) {
            // Duplicate the ranking if added to self when copied.
            if (this == &other) {
                multiset<Picker> copy = pickers_;
                pickers_.insert(copy.begin(), copy.end());
                return *this;
            }

            pickers_.insert(other.pickers_.begin(), other.pickers_.end());
            return *this;
        }

        constexpr Ranking& operator+=(Ranking&& other) {
            // Leave the ranking as is if added to self when moved.
            if (this == &other)
                return *this;

            // Move and merge `other` ranking into this ranking.
            pickers_.insert(
                make_move_iterator(other.pickers_.begin()),
                make_move_iterator(other.pickers_.end())
            );

            return *this;
        }

        /**
         * Returns a read-only reference to the picker at a given `index`
         * (0-based).
         * 
         * If the `index` is out of range, returns the last picker in the
         * ranking.
         */
        const Picker& operator[](const size_t& index) const {
            // Avoid undefined `Picker` constructor behavior
            // e.g. Picker picker = ranking[0];
            //                      ^~~~~~~~~~
            if (pickers_.empty()) {
                static const Picker dummy = Picker();
                return dummy;
            }

            auto it = pickers_.begin();
            advance(it, min(index, pickers_.size() - 1));
            return *it;
        }

        /// Returns the total number of pickers in the ranking.
        size_t count_pickers() const noexcept {
            return pickers_.size();
        }

        friend ostream& operator<<(ostream& os, const Ranking& ranking) {
            // List each picker on a separate line.
            for (const Picker& picker : ranking.pickers_) {
                os << picker << '\n';
            }

            return os;
        }
};

template <typename T1, typename T2>
    requires derived_from<remove_cvref_t<T1>, Ranking>
          && derived_from<remove_cvref_t<T2>, Ranking>
Ranking operator+(T1&& lhs, T2&& rhs) {
    Ranking result = forward<T1>(lhs);
    result += forward<T2>(rhs);
    return result;
}

#endif /* FRUIT_PICKING_H */
