#include "eventstream.h"

#include <iostream>
#include <optional>
#include <functional>

using namespace eventstream;

namespace {
    // Obserwator wypisujący zdarzenia.
    // Zwraca parę {stan, instrukcja}.
    // Stanem jest referencja do strumienia wyjściowego (std::ostream&).
    constexpr auto print_observer =
        [](auto value, auto state) {
            // Parametr state to std::reference_wrapper<std::ostream>
            // przekazane przez std::ref.
            state.get() << value << " ";

            return std::pair{state, control::continue_};
        };
}

int main() {
    // Używamy std::ref(std::cout) jako stanu początkowego, ponieważ
    // strumienie IO nie są kopiowalne, a protokół wymaga zwracania stanu.

    std::cout << "--- 1. Emit i map ---\n";
    {
        auto s = emit(10)
               | map([](int x) { return x * 2; });

        // Wypisuje: 20
        s(print_observer, std::ref(std::cout));
        std::cout << "\n";
    }

    std::cout << "\n--- 2. Counter | Filter | Take ---\n";
    {
        auto s = counter()
               | filter([](int x) { return x % 2 == 0; })
               | take(5);

        // Wypisuje: 2 4 6 8 10
        s(print_observer, std::ref(std::cout));
        std::cout << "\n";
    }

    std::cout << "\n--- 3. Generate ---\n";
    {
        auto step = [](int x) -> std::optional<int> {
            if (x > 100) return std::nullopt;
            return x * 2;
        };

        // Wywołanie generate(1, step) produkuje 1, 2, 4, 8, 16, 32, 64, ...
        auto s1 = generate(1, step);

        // Wywołanie take(6) wymusza koniec konsumenta wcześniej (na 32).
        auto s2 = generate(1, step) | take(6);


        // Wypisuje: 1 2 4 8 16 32 64 128
        s1(print_observer, std::ref(std::cout));
        std::cout << "\n";

        // Wypisuje: 1 2 4 8 16 32
        s2(print_observer, std::ref(std::cout));
        std::cout << "\n";
    }

    std::cout << "\n--- 4. Flatten ---\n";
    {
        // Wywołanie emit tworzy strumień emitujący jedno zdarzenie, którym jest
        // inny strumień. Wywołanie flatten „rozpakowuje” ten wewnętrzny
        // strumień i emituje jego zdarzenia.
        auto s = emit(counter() | take(3))
               | flatten();

        // Wypisuje: 1 2 3
        s(print_observer, std::ref(std::cout));
        std::cout << "\n";
    }

    std::cout << "\n--- 5. Tap ---\n";
    {
        int sum = 0;
        auto updater = [&](int x) { sum += x; };

        // Wywołanie tap daje efekt uboczny, ale nie zmienia przepływu danych.
        auto s = counter()
               | take(4)
               | tap(updater);

        // Wypisuje: 1 2 3 4
        // Suma: 10
        s(print_observer, std::ref(std::cout));
        std::cout << "\nSuma: " << sum << "\n";
    }

    std::cout << "\n--- 6. Memoize ---\n";
    {
        int calls = 0;
        auto f = [&](int x) {
            ++calls;
            return x * x;
        };

        auto mf = memoize(f);

        std::cout << mf(5) << "\n";      // Liczy: calls = 1, wynik 25.
        std::cout << mf(5) << "\n";      // Cache: calls = 1, wynik 25.
        std::cout << mf(10) << "\n";     // Liczy: calls = 2, wynik 100.
        std::cout << "calls = " << calls << "\n";
    }
}
