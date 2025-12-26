# Gra Mastermind

## Wstęp

Biblioteka standardowa języka C++ udostępnia implementacje wielu struktur danych, np. `pair`, `tuple`, `array`, `vector`, `string`, `string_view`, `unordered_set`, `set`, `unordered_map`, `map`, `queue` itp., a także implementacje podstawowych algorytmów, np. `sort`, `lower_bound`, `upper_bound`, `max_element` itd. Celem pierwszego zadania zaliczeniowego jest przećwiczenie korzystania z tej biblioteki. Studenci powinni:

- poznać podstawy korzystania z STL-a,
- uświadomić sobie konieczność weryfikacji poprawności danych wejściowych,
- nauczyć się podejmowania decyzji programistycznych,
- ugruntować w sobie konieczność testowania programu.

## Gra

Mastermind jest znaną gra wymyśloną przez Mordechaja Meirowitza w 1970 roku. Gra dwóch graczy. Gracz *wybierający* (ang. *codemaker*) ustala sekret, który składa się z $n$ pionków, każdy w jednym z $k$ kolorów. Gracz *zgadujący* (ang. *codebreaker*) próbuje poznać sekret, pytając o zgodność podanej przez niego sekwencji pionków z sekretem. Odpowiedź gracza wybierającego składa się z $b$ czarnych pionków i $w$ białych pionków. Liczba czarnych pionków jest to liczba pozycji, na których w pytaniu i sekrecie występują pionki o tych samych kolorach. Liczba białych pionków jest to liczba pionków, których kolor się zgadza, ale są na złej pozycji. Gra kończy się, gdy gracz wybierający udzieli odpowiedzi $b=n$, $w=0$.

## Polecenie

Napisać program do interakcyjnej gry w Mastermind. Przyjmujemy, że $2≤k≤256$, $2≤n≤10$, $k^n≤2^{24}$. Kolory reprezentujemy liczbami całkowitymi od $0$ do $k−1$.

Jeśli program został wywołany z dwoma parametrami, to program jest graczem zgadującym, a użytkownik jest graczem wybierającym. Pierwszy parametr to liczba kolorów $k$, a drugi parametr to liczba pionków $n$.

Jeśli program został wywołany z więcej niż dwoma parametrami, to program jest graczem wybierającym, a użytkownik jest graczem zgadującym. Pierwszy parametr to liczba kolorów, a kolejne parametry to liczby reprezentujące kolory kolejnych pionków w sekrecie.

Program czyta dane ze standardowego wejścia, a wyniki wypisuje na standardowe wyjście. Pytanie gracza zgadującego składa się z $n$ liczb całkowitych reprezentujących kolory kolejnych pionków zgadywanego sekretu. Odpowiedź gracza wybierającego składa się z dwóch liczby całkowitych: $b$ i $w$. Sąsiednie liczby oddzielone są pojedynczą spacją. Pytanie i odpowiedź kończą się znakiem nowej linii. Nie ma żadnych innych białych znaków.

Jeśli program jest graczem zgadującym, to wypisuje pytania, a użytkownik wprowadza odpowiedzi. Program powinien implementować rozsądną strategię. Jeśli program jest graczem wybierającym, to użytkownik wprowadza pytania, a program wypisuje odpowiedzi.

## Obsługa błędów

Jeśli wystąpi błąd, program wypisuje na standardowe wyjście diagnostyczne słowo `ERROR` i znak nowej linii, i kończy się statusem 1. Należy wykrywać

- błędne argumenty programu,
- błędne dane wejściowe,
- sytuację, gdy nie istnieje sekret zgodny z odpowiedziami gracza wybierającego.

## Wymagania formalne

Program kończy się statusem 0, gdy zakończył się poprawnie lub gdy użytkownik zamknął strumień wejściowy.

Oczekiwane rozwiązanie nie powinno zawierać definicji własnych struktur i klas, a przynajmniej takich, które zawierają dane. Zamiast tego należy intensywnie korzystać z kontenerów i algorytmów dostarczanych przez standardową bibliotekę języka C++. Obsługę wejścia i wyjścia należy zrealizować za pomocą strumieni.

Rozwiązanie należy umieścić w pliku `mastermind.cpp`, który należy wstawić do Moodle. Rozwiązanie będzie kompilowane na maszynie students poleceniem

```bash
g++ -Wall -Wextra -O2 -std=c++23 mastermind.cpp -o mastermind
```

## Ocenianie rozwiązania

### Ocena automatyczna

Za testy automatyczne zostanie przyznana ocena z przedziału od 0 do 6 punków. Za błędną nazwę pliku zostanie odjęty 1 punkt. Za ostrzeżenia wypisywane przez kompilator zostanie odjęty 1 punkt. Nie ma punktów ułamkowych.

### Ocena jakości kodu

Ocena jakości kodu jest z przedziału od 0 do 4 punktów. Nie ma punktów ułamkowych. Odejmujemy punkty za:

- brzydki styl (niepoprawne wstawianie spacji, wcięć, odstępów, brak komentarzy, magiczne stałe itd.);
- dłubanie własnych klas, struktur lub algorytmów zamiast użycia STL-owych;
- brak nazw typów, niewiele mówiące nazwy typów;
- rozwlekłą lub nieelegancką strukturę programu, rozpatrywanie zbyt wielu warunków brzegowych, powtarzanie kodu, nieefektywne korzystanie z klasy `string`, np. `if (str != "")` zamiast `if (!str.empty())`, trzymanie liczb jako napisów i używanie w każdym porównaniu komparatora konwertującego napis na liczbę itp.;
- korzystanie z wejścia-wyjścia dostarczanego przez bibliotekę C zamiast ze strumieni lub dłubanie własnego kodu zamiast użycia np. funkcji `getline`;
- potencjalne wycieki pamięci albo korzystanie z `new` i `delete`;
- zły dobór typów całkowitoliczbowych, np. używanie `int` lub `unsigned` zamiast `size_t` jako typu wartości zwracanej przez metody `size`, `length`, używanie `int` lub `long` zamiast `int32_t`, gdy potrzebujemy typu 32-bitowego;
- używanie typu zmiennoprzecinkowego;
- zły wybór kontenera, np. `map`, gdy wystarczyłby `unordered_map`;
- wprowadzanie sztucznych ograniczeń na rozmiar danych;
- nieusuwanie lub nieefektywne usuwanie niepotrzebnych już danych;
- inne znalezione i niewymienione w powyższych kryteriach błędy.

Ponadto:

- piętnujemy przekazywanie funkcjom dużych argumentów (np. typu `string`) przez wartość, takie obiekty przekazuje się przez stałą referencję;
- wytykamy nieukrywanie globalnych zmiennych, struktur, funkcji przed światem zewnętrznym za pomocą anonimowej przestrzeni nazw, choć studenci powinni wiedzieć, że można to też osiągnąć podobnie jak w języku C, czyli deklarując je jako `static`;
- sugerujemy stosowanie `using` zamiast `typedef`.

Na razie tylko wskazujemy te błędy i nie odejmujemy za nie punktów, bo są to zagadnienia pojawiające się w drugim zadaniu, w którym już będziemy za to karać.

## Przykład użycia

Przykład użycia jest częścią specyfikacji. Dla programu uruchomionego z parametrami `6 4 1 2 3 4` i wprowadzonych przez użytkownika następujących pytań:

```text
1 1 1 1 1
1 4 4 4 3
1 4 5 4 5
4 1 2 3 4
```

poprawne odpowiedzi programu są takie:

```text
1 0
0 4
0 3
5 0
```

Przykładowe pytania wypisane przez program uruchomiony z parametrami `6 4` wyglądają tak:

```text
5 5 5 5
1 0 0 3
3 1 2 3
1 3 1 2
1 2 3 4
```

przy założeniu, że odpowiednie odpowiedzi udzielone na te pytania przez użytkownika są takie:

```text
0 0
1 1
0 3
1 2
4 0
```
