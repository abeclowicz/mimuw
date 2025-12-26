# Plejlisty

> [!NOTE]
> Made in collaboration with [@jbryjamimuw](https://github.com/jbryjamimuw).

## Wstęp

Studenci powinni poznać:

- poziomy odporności na wyjątki;
- schematy pozwalające zapewnić co najmniej silną odporność na wyjątki;
- zarządzanie pamięcią z użyciem sprytnych wskaźników;
- schemat implementowania semantyki kopiowania przy modyfikowaniu.

# Polecenie

Należy zaimplementować szablon klasy `playlist`, czyli kontener wspomagający tworzenie i odtwarzanie list utworów, dostępny w przestrzeni nazw `cxx` o następującej deklaracji:

```cpp
namespace cxx {
  template <typename T, typename P> class playlist;
}
```

Typ `T` reprezentuje utwory, które mają być odtwarzane. Typ `P` reprezentuje parametry odtwarzania konkretnego utworu. Typ `T` ma publiczne konstruktory kopiujący i przenoszący, operator(y) przypisania i destruktor. Na typie `T` zdefiniowany jest porządek liniowy i można na obiektach tego typu wykonywać wszelkie porównania. Równość obiektów typu `T` oznacza, że reprezentują one ten sam utwór. Typ `P` ma publiczny konstruktor kopiujący i destruktor.

Obiekty typu `T` mogą być bardzo duże. Należy zadbać o oszczędne używanie pamięci. Plejlista może zawierać wiele powtórzeń tego samego utworu. Należy przechowywać tylko po jednej kopii takiego samego utworu i po jednej kopii parametrów dla każdego odtworzenia utworu. Kontener przejmuje na własność wstawiane obiekty typu `T` i `P`, więc wstawianie obiektów do plejlisty musi wykonać ich kopie. Należy minimalizować liczbę wykonywanych kopii tymczasowych.

Kontener powinien realizować semantykę kopiowania przy modyfikowaniu (ang. *copy on write*). Kopiowanie przy modyfikowaniu to technika optymalizacji szeroko stosowana m.in. w strukturach danych z biblioteki Qt oraz dawniej w implementacjach `std::string`. Podstawowa jej idea jest taka, że gdy tworzymy kopię obiektu (w C++ za pomocą konstruktora kopiującego lub operatora przypisania), to współdzieli ona wszystkie wewnętrzne zasoby (które mogą być przechowywane w oddzielnym obiekcie na stercie) z obiektem źródłowym. Taki stan trwa do momentu, w którym jedna z kopii musi zostać zmodyfikowana. Wtedy modyfikowany obiekt tworzy własną kopię zasobów, na których wykonuje modyfikację. Udostępnienie referencji nie-`const` umożliwiającej modyfikowanie stanu struktury uniemożliwia jej (dalsze) współdzielenie do czasu unieważnienia udzielonej referencji. Przyjmujemy, że taka referencja ulega unieważnieniu po dowolnej modyfikacji struktury.

Szablon klasy `playlist` powinien udostępniać niżej opisane operacje. Przy każdej operacji podana jest jej złożoność czasowa przy założeniu, że nie trzeba wykonać kopii. Złożoność czasowa kopiowania plejlisty to $\mathcal{O}(n \log n)$, gdzie $n$ jest liczbą przechowywanych odtworzeń utworów. Wszystkie operacje muszą zapewniać co najmniej silną gwarancję odporności na wyjątki, a tam gdzie to jest możliwe i pożądane (na przykład konstruktor przenoszący i destruktor), nie mogą zgłaszać wyjątków.

- Konstruktory: bezparametrowy tworzący pustą plejlistę, kopiujący i przenoszący. Przenoszenie pozostawia pustą plejlistę w spójnym stanie. Złożoność $\mathcal{O}(1)$.

```cpp
playlist();
playlist(playlist const &);
playlist(playlist &&);
```

- Destruktor. Złożoność $\mathcal{O}(1)$ plus czas niszczenia niepotrzebnych już składowych obiektu.

```cpp
~playlist();
```

- Operator przypisania, który przyjmuje argument przez wartość. Złożoność $\mathcal{O}(1)$ plus czas niszczenia nadpisywanego obiektu.

```cpp
playlist & operator=(playlist);
```

- Metoda `push_back` wstawia do odtworzenia na koniec listy utwór `track` z parametrami odtwarzania `params`. Złożoność $\mathcal{O}(\log n)$ .

```cpp
void push_back(T const &track, P const &params);
```

- Metoda `pop_front` usuwa z początku listy utwór i parametry jego odtwarzania. Zgłasza wyjątek `std::out_of_range`, jeśli plejlista jest pusta. Złożoność $\mathcal{O}(1)$.

```cpp
void pop_front();
```

- Metoda `front` zwraca referencje na utwór i parametry jego odtwarzania z początku listy. Zgłasza wyjątek `std::out_of_range`, jeśli plejlista jest pusta. Złożoność $\mathcal{O}(1)$.

```cpp
const std::pair<T const &, P const &> front();
```

- Metoda `remove` usuwa z plejlisty wszystkie odtworzenia podanego utworu `track`. Zgłasza wyjątek `std::invalid_argument`, jeśli nie ma takiego utworu na plejliście. Złożoność $\mathcal{O}((k+1)\log n)$, gdzie $k$ jest liczbą usuniętych odtworzeń.

```cpp
void remove(T const &track);
```

- Metoda `clear` opróżnia plejlistę, czyli usuwa z niej wszystkie utwory i parametry ich odtwarzania. Złożoność $\mathcal{O}(n)$.

```cpp
void clear();
```

- Metoda `size` zwraca liczbę utworów do odtworzenia na plejliście (łącznie z powtórzeniami). Złożoność $\mathcal{O}(1)$.

```cpp
size_t size();
```

- Metoda `play` zwraca referencje na utwór i parametry jego odtwarzania wskazywane przez podany iterator `it`. Złożoność $\mathcal{O}(1)$.

```cpp
const std::pair<T const &, P const &> play(play_iterator const &it);
```

- Metoda `pay` zwraca referencję na utwór wskazywany przez podany iterator `it` i liczbę jego wystąpień na plejliście. Złożoność $\mathcal{O}(k)$, gdzie $k$ jest liczbą wystąpień utworu.

```cpp
const std::pair<T const &, size_t> pay(sorted_iterator const &it);
```

- Metody `params` dają dostęp do parametrów utworu wskazywanego przez iterator `it`. Złożoność $\mathcal{O}(1)$.

```cpp
P & params(play_iterator const &it);
const P & params(play_iterator const &it) const;
```

- Iterator `play_iterator` umożliwiający przeglądanie utworów w kolejności ich umieszczenia na plejliście (z powtórzeniami).

- Iterator `sorted_iterator` umożliwiający przeglądanie utworów na plejliście w porządku wyznaczonym przez typ `T` (bez powtórzeń). Ten iteratory służy jedynie do przeglądania plejlisty i za jego pomocą nie można modyfikować plejlisty, więc zachowuje się jak `const_iterator`.

- Metody `play_begin` i `play_end` zwracają `play_iterator` wskazujący odpowiednio na pierwszy i za ostatni utwór na plejliście (w kolejności ich wstawiania, uwzględniając powtórzenia).

- Metody `sorted_begin` i `sorted_end` zwracają `sorted_iterator` wskazujący odpowiednio na pierwszy i za ostatni utwór na plejliście (posortowane bez powtórzeń).

Iteratory powinny udostępniać operatory przypisania `=`, porównania `==` i `!=`, prefiksowy i postfiksowy `++`. Złożoność metod zwracających iteratory powinna być $\mathcal{O}(1)$.

Tam gdzie jest to możliwe i uzasadnione, należy opatrzyć metody kwalifikatorami `const` i `noexcept`.

Szablon klasy `playlist` powinien być przezroczysty na wyjątki, czyli powinien przepuszczać wszelkie wyjątki zgłaszane przez wywoływane przez niego funkcje i przez operacje na jego składowych, a obserwowalny stan obiektu nie powinien się wtedy zmieniać. W szczególności operacje modyfikujące zakończone niepowodzeniem nie mogą unieważniać iteratorów.

Rozwiązanie będzie kompilowane i uruchamiane na maszynie `students` poleceniem

```bash
g++ -Wall -Wextra -O2 -std=c++23 *.cpp
```

Rozwiązanie powinno być zawarte w pliku `playlist.h`, który należy wstawić w Moodle.

## Ocenianie rozwiązania

### Ocena z testów automatycznych

Przyznawany jest jeden punkt za przejście wszystkich testów z każdej z sześciu grup testów. Zarządzanie pamięcią będzie sprawdzane poleceniem

```bash
valgrind --error-exitcode=123 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --run-cxx-freeres=yes -q
```

Za błędną nazwę pliku zostanie odjęty 1 punkt. Za ostrzeżenia wypisywane przez kompilator zostanie odjęty 1 punkt. Nie ma punktów ułamkowych.

### Ocena jakości tekstu źródłowego

Ocena jakości kodu jest z przedziału od 0 do 4 punktów. Nie ma punktów ułamkowych. Odejmujemy punkty za:

- brzydkie formatowanie kodu;
- niedotrzymanie wymaganej złożoności czasowej;
- nieprzejrzysty kod – sprawdzający nie mógł łatwo zrozumieć, jaką złożoność ma dana operacja, dlaczego implementacja poszczególnych metod zapewnia wymaganą gwarancję odporności na wyjątki, dlaczego nie cieknie pamięć, ewentualnie gdzie cieknie pamięć;
- nieprawidłowe oznaczenie lub brak oznaczenia metod jako `noexcept`;
- zaśmiecanie globalnej przestrzeni nazw, nieukrywanie funkcji i struktur pomocniczych jako prywatnych w implementowanej klasie;
- zbędne lub nadmiarowe `#include` (nawet gdy plik się kompiluje);
- niezastosowanie się do kryteriów oceniania poprzednich zadań;
- niezastosowanie się do uwag udzielonych przy ocenie poprzednich zadań.

O ile nie zostanie to wykryte przez testy automatyczne, to będą też odejmowane punkty za:

- brak `header guard`;
- braki `const`;
- nieefektywną implementację kopiowania przy modyfikowaniu, np. niepotrzebne lub zbyt częste kopiowanie;
- jawne użycie operatora `new`, np. zamiast użycia `std::make_shared`.

## Przykłady użycia

Przykłady użycia znajdują się w pliku `playlist_example.cpp`. Wydruk działania tego programu jest w pliku `playlist_example.log`.
