/**
 * Kurs programowania w C++, 2025
 * Przykład do zadania nr 3 "Zbieranie owoców"
 * Plik:        fruit_picking_example.cpp
 * Autor:       Przemysław Rutka
 * Utworzono:   30.10.2025
 */

#include "fruit_picking.h"

#ifdef NDEBUG
    #undef NDEBUG
#endif

#include <iostream>
#include <cassert>
#include <string>
#include <string_view>
#include <tuple>

#define FRUIT_ASSERTS(f, t, s, q) \
do { \
    assert(f.taste() == t); \
    assert(f.size() == s); \
    assert(f.quality() == q); \
} while(0) \

#define PICKER_ASSERTS(p, all, swe, sou, lar, med, sma, hea, rot, wor) \
do { \
    assert(p.count_fruits() == all); \
    assert(p.count_taste(Taste::SWEET) == swe); \
    assert(p.count_taste(Taste::SOUR) == sou); \
    assert(p.count_size(Size::LARGE) == lar); \
    assert(p.count_size(Size::MEDIUM) == med); \
    assert(p.count_size(Size::SMALL) == sma); \
    assert(p.count_quality(Quality::HEALTHY) == hea); \
    assert(p.count_quality(Quality::ROTTEN) == rot); \
    assert(p.count_quality(Quality::WORMY) == wor); \
} while(0) \

using std::tuple, std::make_tuple;
using std::cout;
using std::string, std::string_view;
using fruit_tuple_t = tuple<Taste, Size, Quality>;

namespace {
    void fruit_examples() {
        Fruit f1{Taste::SWEET, Size::LARGE, Quality::HEALTHY};
        FRUIT_ASSERTS(f1, Taste::SWEET, Size::LARGE, Quality::HEALTHY);
        Fruit f1b = f1;
        FRUIT_ASSERTS(f1b, Taste::SWEET, Size::LARGE, Quality::HEALTHY);
        Fruit f2{f1};
        FRUIT_ASSERTS(f2, Taste::SWEET, Size::LARGE, Quality::HEALTHY);
        f1.go_rotten();
        assert(f1.quality() == Quality::ROTTEN);
        f2.become_worm_infested();
        assert(f2.quality() == Quality::WORMY);
        Fruit f3{make_tuple(Taste::SOUR, Size::MEDIUM, Quality::WORMY)};
        FRUIT_ASSERTS(f3, Taste::SOUR, Size::MEDIUM, Quality::WORMY);
        fruit_tuple_t fruit_tuple{f1};
        auto const& [taste, size, quality] {fruit_tuple};
        assert(taste == Taste::SWEET &&
               size == Size::LARGE &&
               quality == Quality::ROTTEN);
        [[maybe_unused]] fruit_tuple_t ft = static_cast<fruit_tuple_t>(f1);
        constexpr Fruit cf1{Taste::SWEET, Size::SMALL, Quality::HEALTHY};
        static_assert(cf1.taste() == Taste::SWEET &&
                      cf1.size() == Size::SMALL &&
                      cf1.quality() == Quality::HEALTHY);
        assert(f1b == YUMMY_ONE);
        assert(f1 != f2);
    }

    void picker_examples() {
        Picker p1{"Alojzy"};
        assert(p1.get_name() == "Alojzy");
        PICKER_ASSERTS(p1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        p1 += YUMMY_ONE;
        PICKER_ASSERTS(p1, 1, 1, 0, 1, 0, 0, 1, 0, 0);
        p1 += YUMMY_ONE;
        PICKER_ASSERTS(p1, 2, 2, 0, 2, 0, 0, 2, 0, 0);
        p1 += ROTTY_ONE;
        PICKER_ASSERTS(p1, 3, 2, 1, 2, 0, 1, 1, 2, 0);
        p1 += Fruit{Taste::SWEET, Size::MEDIUM, Quality::WORMY};
        PICKER_ASSERTS(p1, 4, 3, 1, 2, 1, 1, 0, 2, 2);
        p1 += Fruit{Taste::SOUR, Size::LARGE, Quality::HEALTHY};
        PICKER_ASSERTS(p1, 5, 3, 2, 3, 1, 1, 1, 2, 2);
        Picker p2{"Gerwazy"};
        p2 += Fruit{Taste::SOUR, Size::LARGE, Quality::HEALTHY};
        p2 += p1;
        PICKER_ASSERTS(p1, 4, 2, 2, 2, 1, 1, 1, 2, 1);
        PICKER_ASSERTS(p2, 2, 1, 1, 2, 0, 0, 1, 0, 1);
        Picker p3{"Protazy"};
        p1 -= p3;
        PICKER_ASSERTS(p1, 3, 1, 2, 1, 1, 1, 1, 1, 1);
        PICKER_ASSERTS(p3, 1, 1, 0, 1, 0, 0, 0, 1, 0);
        assert(p2 < p3);
        assert(p2 <= p3);
        assert(p2 != p3);
        assert(!(p2 == p3));
        assert((p2 <=> p3) < 0);
        assert(p3 > p1);
        assert(p3 >= p1);
        assert((p3 <=> p1) > 0);
        Picker p4{string{"Arnold"}}, p4b{string_view{"Arnold"}};
        p4 += YUMMY_ONE;
        p4b += YUMMY_ONE;
        Picker p5{"Sylvester"};
        p5 += YUMMY_ONE;
        assert(p4 == p4b);
        assert(p4 != p5);
        assert((p4 <=> p5) == 0); // To jest sensowne na potrzeby rankingu!
        assert(p4 == p4);
        Picker p6{"Chuck"};
        p6 += ROTTY_ONE;
        assert(p4 != p6);
    }

    void ranking_examples() {
        Picker p1{"Arnold"};
        p1 += YUMMY_ONE;
        p1 += ROTTY_ONE;
        Picker p2{"Sylvester"};
        p2 += YUMMY_ONE;
        p2 += YUMMY_ONE;
        p2 += ROTTY_ONE;
        Picker p3{};
        p3 += Fruit{Taste::SOUR, Size::MEDIUM, Quality::WORMY};

        Ranking r1{p3, p2, p1};
        assert(r1.count_pickers() == 3);
        assert(r1[0] == p2);
        assert(r1[1] == p1);
        assert(r1[2] == p3);
        cout << "RANKING 1\n" << r1 << '\n';
        Ranking r2{};
        assert(r2.count_pickers() == 0);
        r2 += p1;
        assert(r2.count_pickers() == 1);
        Picker p4{"Chuck"};
        p4 += Fruit{Taste::SOUR, Size::MEDIUM, Quality::WORMY};
        r2 += p4;
        r2 += p3;
        r2 += p4;
        assert(r2.count_pickers() == 4);
        r2 -= p3;
        assert(r2.count_pickers() == 3);
        r2 -= p3;
        assert(r2.count_pickers() == 3);
        cout << "RANKING 2\n" << r2 << '\n';
        Ranking r3{};
        r3 += r1;
        assert(r3.count_pickers() == 3);
        r3 += r2;
        assert(r3.count_pickers() == 6);
        cout << "RANKING 3\n" << r3 << '\n';
        assert((r3 + r1).count_pickers() == 9);
        Ranking r4{r1};
        assert(r4.count_pickers() == r1.count_pickers());
        assert(r4[0].get_name() == "Sylvester");
        assert(r4[1].get_name() == "Arnold");
        assert(r4[2].get_name() == "Anonim");
        Ranking r5{};
        r5 = r2;
        assert(r5.count_pickers() == r2.count_pickers());
        assert(r5[0].get_name() == "Arnold");
        assert(r5[1].get_name() == "Chuck");
        assert(r5[2].get_name() == "Chuck");
    }
} // anonymous namespace

int main() {
    fruit_examples();
    picker_examples();
    ranking_examples();
}
