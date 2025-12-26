# Strumienie zdarzeń

## Wstęp

Studenci powinni poznać:

- podstawy programowania funkcyjnego z użyciem C++,
- składnię wyrażeń lambda,
- różnice pomiędzy `[=]` oraz `[&]`.

## Polecenie

Celem zadania jest zaimplementowanie biblioteki do przetwarzania strumieni zdarzeń zależnie od aktualnego stanu. Zdarzenie jest dowolnym typem, który może być kopiowany. Stan jest dowolnym typem, który może być kopiowany.

### Obserwatory

Obserwator jest obiektem wywoływalnym, który przyjmuje zdarzenie oraz aktualny stan, a zwraca parę: nowy stan oraz instrukcję sterującą. W tym celu obserwator udostępnia operator wywołania, którego sygnatura jest zgodna z następującym szablonem:

```cpp
template <typename Event, typename State> std::pair<State, control> operator()(Event event, State state);
```

Instrukcja sterująca definiuje, czy obserwator chce otrzymywać kolejne zdarzenia, i jest typu `control`, który powinien mieć następującą definicję:

```cpp
enum class control {
    continue_, // Przetwarzaj dalej – obserwator oczekuje na kolejne zdarzenie.
    stop       // Natychmiast przerwij strumień – obserwator rezygnuje z dalszych zdarzeń.
};
```

Implementacje obserwatorów są dostarczane przez nas.

### Strumienie

Strumień jest obiektem wywoływalnym (funktorem lub lambdą), który emituje zdarzenia do przekazanego obserwatora. Od typu reprezentującego strumień wymagamy, aby miał operator wywołania o sygnaturze zgodnej z poniższym szablonem:

```cpp
template <typename Obs, typename State> State operator()(Obs observer, State state);
```

Uruchomienie strumienia `s` wywołaniem `s(observer, state)` powoduje sekwencyjne wywoływanie obserwatora `observer` dla kolejnych emitowanych zdarzeń `e0`, `e1`, `…` W pierwszym wywołaniu obserwator dostaje stan `state`. W każdym kolejnym wywołaniu obserwator dostaje stan zwrócony przez obserwatora w poprzednim wywołaniu.

W ramach zadania należy zaimplementować niżej opisane rodzaje strumieni.

### Zasady sterowania przepływem

Strumień kończy działanie i zwraca ostateczny stan w jednym z dwóch przypadków:

- **Decyzja obserwatora**: Obserwator zwrócił `control::stop`. W takim przypadku strumień niezwłocznie przerywa pętlę i zwraca aktualny stan (nie emituje kolejnych zdarzeń).
- **Wyczerpanie strumienia**: Strumień nie ma więcej zdarzeń do wyemitowania.

Formalnie, wynik wywołania `s(observer, state)` jest złożeniem wywołań obserwatora, dopóki nie nastąpi jeden z powyższych warunków przerwania.

### Rodzaje strumieni

Należy zaimplementować następujące funkcje tworzące strumienie:

- `auto emit(auto x)` – funkcja zwracająca strumień emitujący dokładnie jedno zdarzenie `x`, po czym kończący działanie;
- `auto generate(auto init, auto step)` – funkcja zwracająca strumień działający następująco:
    - pierwszym zdarzeniem jest `init`;
    - każde kolejne zdarzenie powstaje przez wywołanie `step(last_value)`;
    - funkcja `step` musi zwracać obiekt typu `std::optional<T>`;
    - jeśli `step` zwróci wartość (`std::optional` z zawartością), jest ona emitowana do obserwatora i staje się argumentem kolejnego kroku;
    - jeśli `step` zwróci `std::nullopt`, strumień kończy działanie;
- `auto counter()` – funkcja zwracająca strumień emitujący kolejne liczby typu `int`, zaczynając od `1`, do momentu przerwania przez obserwatora; jeśli emitowane liczby przekroczą górny zakres typu `int`, `counter` zaczyna liczyć od dolnego zakresu typu `int`; każda kopia tego strumienia musi mieć niezależny licznik.

### Transformatory i filtry

Należy zaimplementować następujące transformatory i filtry strumieni:

- `auto map(auto f, auto s)` – funkcja zwracająca strumień, który dla każdego zdarzenia `e` ze strumienia `s` emituje do swojego obserwatora wynik `f(e)`;
- `auto filter(auto pred, auto s)` – funkcja zwracająca strumień, który emituje dalej tylko te zdarzenia ze strumienia `s`, dla których predykat `pred(e)` zwraca `true`;
- `auto take(std::size_t n, auto s)` – funkcja zwracająca strumień, który emituje co najwyżej `n` pierwszych zdarzeń strumienia `s`; po wyemitowaniu `n`-tego zdarzenia, strumień `take` powinien przerwać pobieranie danych ze strumienia `s`;
- `auto flatten(auto ss)` – funkcja zwracająca strumień powstały ze spłaszczenia strumienia strumieni `ss`; każde zdarzenie wyemitowane przez `ss` jest traktowane jako strumień, który należy w całości uruchomić.

### Narzędzia pomocnicze

Dodatkowo należy zaimplementować:

- `auto memoize(auto f)` – funkcja zwracająca funkcję opakowującą (ang. *wrapper*), która przyjmuje te same argumenty co `f` i zapamiętuje jej wyniki w swojej pamięci podręcznej (ang. *cache*);
    - funkcja opakowująca wywołana z zestawem argumentów, który już był przetwarzany, zwraca zapamiętany wynik bez ponownego wołania `f`;
    - każda kopia zwróconej funkcji opakowującej ma własną, niezależną pamięć podręczną;
    - funkcja opakowująca musi obsługiwać wywołania z argumentami wymagającymi niejawnej konwersji: jeśli funkcja `f` może być wywołana z danym typem argumentu (np. `const char*` dla funkcji przyjmującej `std::string`), to funkcja opakowująca również musi na to pozwalać, poprawnie dopasowując typ klucza w pamięci podręcznej do sygnatury funkcji `f`.

### Potokowanie

Należy przeciążyć operator `|`, tak aby umożliwić zapis potokowy. Zapis `s | oper` powinien być równoważny wywołaniu funkcji transformującej, np.:

- `s | map(f) ≡ map(f, s)`
- `s | filter(p) ≡ filter(p, s)`

Ponadto należy zaimplementować:

- `auto tap(auto side_effect)` – funkcja zwracająca operator strumieniowy (do użycia w potoku), który dla każdego zdarzenia wywołuje `side_effect(x)`; funkcja ta **nie modyfikuje** zdarzenia ani **nie wpływa** na przepływ sterowania (zawsze przekazuje instrukcję `control` otrzymaną od kolejnego obserwatora w łańcuchu).

W przypadku funkcji przyjmujących tylko jeden argument (jak `tap` zwracający operator), operator | aplikuje ten operator do strumienia po lewej stronie.

### Założenia i wskazówki

Przy operacjach na strumieniach argumenty reprezentujące strumienie przekazujemy przez wartość (kopiujemy). Kopiowanie strumieni musi być semantycznie poprawne – przykładowo, skopiowanie strumienia `counter` i uruchomienie obu kopii powinno skutkować dwiema niezależnymi sekwencjami liczbowymi.

Zakładamy, że wszystkie obiekty funkcyjne przekazywane do biblioteki (predykaty w `filter`, funkcje w `map`, funkcja `step` w `generate`) oraz emitowane zdarzenia są kopiowalne (mają publiczny konstruktor kopiujący). Nie trzeba obsługiwać typów „move-only”.

Zakładamy, że użytkownik biblioteki podaje poprawne typy argumentów. Funkcja `step` w `generate` zawsze zwraca `std::optional<T>`. Funkcja transformująca w `map` przyjmuje typ, który jest emitowany przez strumień. Predykat w `filter` zwraca wartość konwertowalną na `bool`.

Funkcja przekazywana do `memoize` musi mieć ściśle określoną sygnaturę. Niedozwolone jest przekazywanie lambd generycznych (mających parametry `auto`) oraz szablonów funkcji. Argumenty funkcji `f` podanej do `memoize` są kopiowalne lub są stałą referencją oraz są porównywalne.

Nie jest wymagane sprawdzanie poprawności typów za pomocą `concept` ani `static_assert`.

## Wymagania formalne

Nie wymagamy, aby biblioteka była bezpieczna wątkowo.

Wszystkie wymagane w treści zadania definicje powinny znaleźć się w przestrzeni nazw `eventstream`. Pomocnicze definicje należy ukryć przed światem, np. w przestrzeni nazw `eventstream::detail`.

Wszelkie operacje muszą być zaimplementowane jako funkcje zwracające obiekty (najlepiej lambdy) spełniające protokół strumienia. Jeżeli mamy wybór między zdefiniowaniem własnej struktury a użyciem lambdy, należy użyć lambdę. **Wyjątkowo**, w przypadku implementacji `memoize` dozwolone jest zdefiniowanie pomocniczej ukrytej struktury, aby poprawnie obsłużyć dedukcję typów argumentów i spamiętywanie wcześniejszych wywołań.

Rozwiązanie należy umieścić w pliku `eventstream.h`, który należy wstawić na Moodle. Rozwiązanie będzie kompilowane na maszynie `students` poleceniem:

```bash
g++ -Wall -Wextra -O2 -std=c++23 *.cpp
```

## Ocenianie rozwiązania

### Ocena automatyczna

Za testy automatyczne zostanie przyznana ocena z przedziału od 0 do 6 punktów. Za błędną nazwę pliku zostanie odjęty 1 punkt. Za ostrzeżenia wypisywane przez kompilator zostanie odjęty 1 punkt. Nie ma punktów ułamkowych.

### Ocena jakości kodu

Ocena jakości kodu jest z przedziału od 0 do 4 punktów. Nie ma punktów ułamkowych. Odejmujemy punkty za:

- nieelegancki styl, złe formatowanie kodu, brak komentarzy;
- bezsensowne komentarze informujące o rzeczach, które w oczywisty sposób wynikają z kodu;
- rozwiązanie niefunkcyjne;
- tworzenie własnych struktur lub klas, jeżeli da się to samo zrobić z użyciem lambd;
- kod, którego sprawdzający nie jest w stanie zrozumieć;
- anonimową przestrzeń nazw w pliku nagłówkowym, zaśmiecanie przestrzeni nazw (np. `using namespace std`);
- wszystkie błędy, które nie powinny się już pojawiać po poprzednich zajęciach, np. brak *header guard*, przekazywanie dużych obiektów przez wartość, braki `const` itp., o ile nie zostały wykryte przez testy automatyczne.

## Przykłady użycia

Przykłady użycia znajdują się w pliku `eventstream_example.cpp`.
