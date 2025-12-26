# Kolekcje nazwanych zbiorów częściowo uporządkowanych

> [!NOTE]
> Made in collaboration with [@myny2005](https://github.com/myny2005).

## Wstęp

Biblioteka standardowa języka C++ udostępnia bardzo przydatne kontenery, których nie ma w bibliotece C. Język C++ umożliwia definiowanie skomplikowanych struktur danych, na przykład

```cpp
unordered_map<long, map<string, pair<array<bitset<N>, N>, array<bitset<N>, N>>>>
```

Często potrzebujemy łączyć kod w C z kodem w C++.

Celem tego zadania jest napisanie w C++ modułu obsługującego kolekcje nazwanych zbiorów częściowo uporządkowanych (ang. *poset*), aby można ich używać w C. Moduł składa się z pliku nagłówkowego (z rozszerzeniem `.h`) i pliku z implementacją (z rozszerzeniem `.cpp`).

Rozwiązując to zadanie, studenci powinni poznać:

- kolejne kontenery z STL,
- sposób łączenia kodu w C++ z kodem w C,
- metody inicjowania obiektów globalnych w C++ i wynikające stąd problemy,
- przestrzenie nazw,
- preprocesor, elementy kompilowania warunkowego.

## Polecenie

Zbiór częściowo uporządkowany jest parą składającą ze zbioru i dwuargumentowej zwrotnej, antysymetrycznej i przechodniej relacji na tym zbiorze. Moduł `named_poset_collections` obsługuje zbiory częściowo uporządkowane na zbiorze, którego elementami są liczby całkowite od `0` do `N - 1`, gdzie `N` jest dodatnią stałą ustalaną w trakcie kompilowania modułu. Jej domyślna wartość to `32` i można ją zmieniać parametrem `-D` kompilatora. Zbiory częściowo uporządkowane mają nazwy. Nazwa jest niepustym napisem składającym się z małych i dużych liter alfabetu angielskiego, cyfr oraz znaku podkreślenia. Zbiory częściowo uporządkowane w obrębie kolekcji porządkujemy leksykograficznie według ich nazw. Ten porządek leksykograficzny jest indukowany przez porządek na wartościach ASCII znaków.

Moduł powinien udostępniać następujące funkcje.

```cpp
long npc_new_collection();
```

Tworzy nową, pustą kolekcję nazwanych zbiorów częściowo uporządkowanych. Wynikiem jest identyfikator tej kolekcji. Każde wywołanie zwraca nowy identyfikator, począwszy od `0`, a skończywszy na `LONG_MAX`. Jeśli wyczerpią się identyfikatory, zwraca `-1`.

```cpp
void npc_delete_collection(long id);
```

Jeżeli istnieje kolekcja nazwanych zbiorów częściowo uporządkowanych o identyfikatorze `id`, usuwa ją, a w przeciwnym przypadku niczego nie robi.

```cpp
bool npc_new_poset(long id, char const *name);
```

Jeśli istnieje kolekcja o identyfikatorze `id`, a `name` jest poprawną nazwą i nie ma w tej kolekcji zbioru częściowo uporządkowanego o nazwie `name`, tworzy w tej kolekcji nowy zbiór częściowo uporządkowany o podanej nazwie i relacji zawierającej jedynie pary `(x, x)` dla `x = 0, 1, …, N - 1`. Wynikiem jest `true`, jeśli zbiór częściowo uporządkowany został utworzony, a `false` w przeciwnym przypadku.

```cpp
void npc_delete_poset(long id, char const *name);
```

Jeśli istnieje kolekcja o identyfikatorze `id` i jest w niej zbiór częściowo uporządkowany o nazwie `name`, usuwa go, a w przeciwnym przypadku niczego nie robi.

```cpp
bool npc_copy_poset(long id, char const *name_dst, char const *name_src);
```

Jeśli istnieje kolekcja o identyfikatorze `id`, a `name_dst` jest poprawną nazwą i jest w tej kolekcji zbiór częściowo uporządkowany o nazwie `name_src`, kopiuje go na zbiór częściowo uporządkowany o nazwie `name_dst`. Wynikiem jest `true`, jeśli zbiór został skopiowany, a `false` w przeciwnym przypadku.

```cpp
char const * npc_first_poset(long id);
```

Jeśli istnieje niepusta kolekcja o identyfikatorze `id`, wynikiem jest wskaźnik na nazwę pierwszego zbioru częściowo uporządkowanego w tej kolekcji, a `NULL` w przeciwnym przypadku.

```cpp
char const * npc_next_poset(long id, char const *name);
```

Jeśli istnieje kolekcja o identyfikatorze `id`, a w niej istnieje zbiór częściowo uporządkowany następny w kolejności po zbiorze częściowo uporządkowanym o nazwie `name`, wynikiem jest wskaźnik na nazwę tego następnego zbioru częściowo uporządkowanego, a `NULL` w przeciwnym przypadku.

```cpp
bool npc_add_relation(long id, char const *name, size_t x, size_t y);
```

Jeśli istnieje kolekcja o identyfikatorze `id`, a w niej istnieje zbiór częściowo uporządkowany o nazwie `name`, elementy `x` i `y` należą do tego zbioru, ale nie są w relacji, dodaje parę `(x, y)` do relacji i domyka relację przechodnio. Wynikiem jest `true`, jeśli relacja została zmodyfikowana, a `false` w przeciwnym przypadku.

```cpp
bool npc_is_relation(long id, char const *name, size_t x, size_t y);
```

Jeśli istnieje kolekcja o identyfikatorze `id`, a w niej istnieje zbiór częściowo uporządkowany o nazwie `name` oraz para `(x, y)` należy do relacji tego zbioru, wynikiem jest `true`, a `false` w przeciwnym przypadku.

```cpp
bool npc_remove_relation(long id, char const *name, size_t x, size_t y);
```

Jeśli istnieje kolekcja o identyfikatorze `id`, a w niej istnieje zbiór częściowo uporządkowany o nazwie `name` oraz para `(x, y)` różnych elementów należy do relacji tego zbioru i nie istnieje element `z` różny od `x` i `y`, taki że pary `(x, z)` i `(z, y)` należą do relacji, usuwa parę `(x, y)` z relacji. Wynikiem jest `true`, jeśli relacja została zmodyfikowana, a `false` w przeciwnym przypadku.

```cpp
size_t npc_size();
```

Wynikiem jest liczba aktualnie istniejących kolekcji.

```cpp
size_t npc_poset_size();
```

Wynikiem jest liczba elementów zbioru częściowo uporządkowanego.

```cpp
size_t npc_collection_size(long id);
```

Jeśli istnieje kolekcja o identyfikatorze `id`, wynikiem jest liczba zbiorów częściowo uporządkowanych w tej kolekcji, a `0` w przeciwnym przypadku.

## Wymagania formalne

Oczekiwane rozwiązanie powinno korzystać z kontenerów i metod udostępnianych przez standardową bibliotekę C++. Nie należy definiować własnych struktur, unii lub klas, a przynajmniej takich, które zawierają dane.

Kolekcje powinny przechowywać kopie nazw, a nie wartości przekazanych wskaźników.

Powinna być możliwość używania wyżej opisanych funkcji w języku C++. Przy kompilowaniu pliku nagłówkowego modułu w C++ deklaracje funkcji powinny znaleźć się w przestrzeni nazw `cxx`.

Należy ukryć przed światem zewnętrznym wszystkie zmienne globalne i funkcje pomocnicze nienależące do interfejsu modułu.

Wolno założyć, że nie zabraknie pamięci. Nie wymagamy obsługiwania wyjątku braku pamięci.

Rozwiązanie powinno składać się z plików `named_poset_collections.h` i `named_poset_collections.cpp`, które należy umieścić w Moodle.

Rozwiązanie będzie kompilowane i testowane na maszynie `students`.

## Przykłady użycia

Przykłady użycia znajdują się w poniżej załączonych plikach `named_poset_collections_example_1.c` i `named_poset_collections_example_2.cpp`. Kompilowanie przykładów przy założeniu, że znajdują się w bieżącym katalogu, a rozwiązanie jest w katalogu `../rozwiązanie`:

```bash
g++ -c -Wall -Wextra -O2 -std=c++23 ../rozwiazanie/named_poset_collections.cpp -o named_poset_collections_32.o
g++ -c -Wall -Wextra -O2 -std=c++23 -DN=64 ../rozwiazanie/named_poset_collections.cpp -o named_poset_collections_64.o
gcc -c -Wall -Wextra -O2 -std=c23 -I../rozwiazanie named_poset_collections_example_1.c -o named_poset_collections_example_1.o
g++ -c -Wall -Wextra -O2 -std=c++23 -I../rozwiazanie named_poset_collections_example_2.cpp -o named_poset_collections_example_2.o
g++ named_poset_collections_example_1.o named_poset_collections_32.o -o named_poset_collections_example_1
g++ named_poset_collections_example_2.o named_poset_collections_64.o -o named_poset_collections_example_2_a
g++ named_poset_collections_64.o named_poset_collections_example_2.o -o named_poset_collections_example_2_b
```

## Ocenianie rozwiązania

### Ocena automatyczna

Za testy automatyczne zostanie przyznana ocena z przedziału od 0 do 6 punków. Za błędną nazwę pliku zostanie odjęty 1 punkt. Za ostrzeżenia wypisywane przez kompilator zostanie odjęty 1 punkt. Nie ma punktów ułamkowych.

### Ocena jakości kodu

Ocena jakości kodu jest z przedziału od 0 do 4 punktów. Nie ma punktów ułamkowych. Odejmujemy punkty za:

- brzydki styl (niepoprawne wstawianie spacji, wcięć, odstępów, brak komentarzy, magiczne stałe itd.);
- dłubanie własnych klas, struktur lub algorytmów zamiast użycia STL-owych;
- zły dobór typu lub kontenera, brak nazw typów, niewiele mówiące nazwy typów;
- rozwlekłą lub nieelegancką strukturę programu, rozpatrywanie zbyt wielu warunków brzegowych, powtarzanie kodu, nieefektywne korzystanie z klasy `string`, np. `if (str != "")` zamiast `if (!str.empty())`, przechowywanie liczb jako napisów;
- wprowadzanie sztucznych ograniczeń na rozmiar danych;
- nieusuwanie lub nieefektywne usuwanie niepotrzebnych już danych;
- nieskuteczną obsługę (lub jej brak) problemu „static initialization order fiasco” (czytanka „Inicjowanie obiektów globalnych”), o ile nie zostanie to wykryte przez testy automatyczne; inicjowanie strumienia diagnostycznego w każdej funkcji (przez powielenie kodu inicjującego);
- potencjalne wycieki pamięci albo korzystanie z `new` i `delete`;
- niepoprawne pliki nagłówkowe, brak include guard `#ifndef #define #endif`, brak `#ifdef __cplusplus`, brakujące `#include`, zbędne `#include`;
- nieukrycie przed światem zewnętrznym wszystkich zmiennych globalnych i funkcji pomocniczych nienależących do interfejsu modułu;
- użycie `typedef` zamiast `using`;
- błędy w stosowaniu przestrzeni nazw;
- wielokrotne wyszukiwanie tego samego klucza w mapie, np. za pomocą operatora `[]`;
- inne znalezione i niewymienione w powyższych kryteriach błędy niewykryte przez testy automatyczne.

Ponadto piętnujemy:

- przekazywanie funkcjom dużych argumentów przez wartość – takie obiekty przekazujemy przez stałą referencję, czyli `const &`; na razie tylko wskazujemy te błędy i nie odejmujemy za nie punktów, bo są to zagadnienia pojawiające się w kolejnych zadaniach, w których już będziemy za to karać.
