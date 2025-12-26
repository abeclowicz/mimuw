# Mole tekstowe

> [!NOTE]
> Made in collaboration with [@mguszczin](https://github.com/mguszczin).

## Wstęp

W standardzie C++20 wprowadzono do języka C++ moduły. Mimo że w dostępnych kompilatorach ich implementacja nie jest kompletna, a w niektórych aspektach eksperymentalna, proponujemy zapoznanie się z tym nowym rozszerzeniem języka. Celem tego zadania jest poznanie:

- podstaw używania modułów w C++,
- paradygmatu programowania obiektowego w C++.

## Mole

Mole tekstowe żywią się znakami o kodach ASCII od 33 do 126 włącznie. Znak po zjedzeniu przez mola zamienia się w niejadalną spację – kod ASCII 32. Atrybutem mola jest jego witalność, która jest nieujemną liczbą całkowitą. Aktywność mola polega na przesunięciu się o zadaną liczbę znaków, a potem na próbie zjedzenia znaku. Na przemieszczenie się o jeden znak mol traci 10 jednostek witalności. Jeśli witalność mola jest za mała, aby się przemieścić na pozycję docelową, to mol traci resztkę swojej witalności przy próbie opuszczenia dotychczasowej pozycji. W konsekwencji pozostaje on na miejscu i przestaje być aktywny. Mol zwiększa swoją witalność o wartość kodu ASCII zjedzonego znaku lub zmniejsza swoją witalność o wartość kodu ASCII niejadalnej spacji, ale nie bardziej niż do zera. Mole traktują tekst, jakby był zapętlony cyklicznie, czyli po dojściu do końca tekstu zaczynają się po nim przemieszczać od początku. Jest kilka rodzajów moli różniących się rodzajem zjadanych znaków i sposobem przemieszczania się. Rozróżniamy następujące rodzaje moli:

- zwykły, który zjada wszystkie dopuszczalne znaki,
- literowy, który zjada tylko małe i duże litery,
- cyfrowy, który zjada tylko cyfry,
- wybredny, który zjada znaki inne niż litery i cyfry.

Mole zwykłe, literowe i cyfrowe przesuwają się o zadaną liczbę znaków, podaną jako parametr `P` przy ich tworzeniu. Mole wybredne przesuwają się kolejno o `1, 2, …, P` znaków i potem znów o `1, 2` itd.

## Polecenie

Napisz program obsługujący kolonię moli żerujących na napotkanym tekście. Program czyta dane ze standardowego wejścia liniami. Poprawna linia z danymi wejściowymi ma jeden z następujących formatów:

- `TEXT <T> <TEXT>`

    Wczytuje tekst `<TEXT>` o numerze `<T>`.

- `MOTH <T> <N> <R> <V> <P>`

    Umieszcza w tekście o numerze `<T>` na polu o numerze `<N>` mola rodzaju `<R>` o początkowej witalności `<V>` z parametrem `<P>`.

- `FEED <T> <C>`

    Wykonuje `<C>` cykli żerowania kolonii moli na tekście o numerze `<T>`. Mole wykonują swoją aktywność w kolejności, w jakiej zostały umieszczone na tekście. Każdy aktywny mol przemieszcza się i próbuje się pożywić.

- `PRINTM <T>`

    Drukuje na standardowe wyjście informację o molach żerujących na tekście `<T>` w kolejności ich dodawania do listy. Dla każdego mola w osobnej linii drukuje jego rodzaj, wartość parametru, pozycję w tekście i witalność, oddzielone pojedynczą spacją. Każda wypisywana linia jest zakończona znakiem nowej linii i nie ma w niej innych białych znaków niż wyżej opisane.

- `PRINTT <T>`

    Drukuje aktualną postać tekstu numer `<T>`.

- `DELETE <T>`

    Usuwa tekst o numerze `<T>` wraz ze wszystkimi żerującymi na nim molami.

Każdy nowy mol jest umieszczany na końcu listy. Nowy mol zaczyna być aktywny w następnym cyklu żerowania.

Numer tekstu `<T>` jest liczbą całkowitą nieujemną. Text `<TEXT>` zawiera znaki ASCII o kodach od 33 do 126. Numer pola `<N>` w tekście jest liczbą całkowitą nieujemną mniejszą niż długość tekstu. Rodzaj mola `<R>` jest znakiem * dla zwykłego mola, literą `A` dla mola literowego, cyfrą `1` dla mola cyfrowego, znakiem `!` dla mola wybrednego. Witalność `<V>` jest dodatnią liczą całkowitą. Parametr mola `<P>` jest liczbą całkowitą z przedziału od 1 do 99. Liczba cykli `<C>` jest dodatnią liczbą całkowitą.

Nazwa polecenia i jego parametry oddzielone są pojedynczą spacją. Linie wejściowe są rozdzielone znakiem nowej linii. Po ostatniej linii znak nowej linii jest opcjonalny. Poprawne dane wejściowe nie zawierają innych białych znaków niż wymienione powyżej.

## Obsługa błędów

Program na bieżąco sprawdza, czy dane wejściowe nie zawierają błędów. Dla każdej błędnej linii program wypisuje na standardowe wyjście diagnostyczne komunikat

`ERROR <L>`

gdzie `<L>` oznacza numer linii. Linie są numerowane od 1. Komunikat o błędzie kończy się znakiem nowej linii. Program ignoruje zawartość błędnych linii.

Jako błędną należy potraktować linię, jeśli w poleceniu `TEXT` podano numer istniejącego tekstu, a w innych poleceniach podano numer nieistniejącego tekstu, jeśli podano niepoprawny numer pola.

## Wymagania formalne

Program powinien kończyć się kodem 0.

Implementacja powinna korzystać z paradygmatu programowania obiektowego.

Implementacja powinna być podzielona na kilka modułów. Każdy moduł powinien znajdować się w osobnym pliku z rozszerzeniem `.cppm`. Oprócz tego może być jeden plik z rozszerzeniem `.cpp` zawierający funkcję `main` programu.

Nie wolno korzystać z poleceń preprocesora zaczynających się znakiem `#`.

Należy dostarczyć skrypt `makefile` lub `Makefile` umożliwiający skompilowanie programu za pomocą polecenia `make`. Wywołanie tego polecenia bez parametrów powinno tworzyć plik wykonywalny `text_moths`. Skrypt powinien zawierać cele `.PHONY` i `clean`. Może też zawierać inne cele, np. `test`. Skrypt powinien zawierać reguły kompilowania przyrostowego i opisywać zależności między plikami.

Jako rozwiązanie należy wstawić w Moodle wspomniane wyżej pliki. Nie wolno wstawiać innych plików.

## Wskazówki dotyczące kompilowania

Polecenie kompilujące wstępnie plik nagłówkowy `vector`:

```bash
clang++ -std=c++23 -O2 -Wall -Wextra -Wno-experimental-header-units -Wno-pragma-system-header-outside-header -xc++-system-header --precompile vector -o vector.pch
```

Polecenie kompilujące wstępnie moduł `module1` zawarty w pliku `module1.cppm` i zależny od modułów `vector` oraz `module2` (zakładamy, że w bieżącym katalogu są pliki `vector.pch` i `module2.pcm`):

```bash
clang++ -std=c++23 -O2 -Wall -Wextra -Wno-experimental-header-units -fprebuilt-module-path=. --precompile -fmodule-file=vector.pch module1.cppm -o module1.pcm
```

Polecenie kompilujące moduł `module1`:

```bash
clang++ -std=c++23 -O2 -Wall -Wextra -fprebuilt-module-path=. -c module1.pcm -o module1.o
```

Polecenie kompilujące plik `program.cpp` zawierający funkcję `main`:

```bash
clang++ -std=c++23 -O2 -Wall -Wextra -Wno-experimental-header-units -fprebuilt-module-path=. -c -o program.o
```

Polecenie linkujące całość:

```bash
clang++ program.o module1.o module2.o -o executable
```

Rozwiązanie będzie kompilowane i testowane na maszynie `students`.

## Ocenianie rozwiązania

Rozwiązanie niespełniające wymagań formalnych lub niekompilujące się może dostać zero punktów.

### Ocena automatyczna

Za testy automatyczne zostanie przyznana ocena z przedziału od 0 do 6 punków. Za ostrzeżenia wypisywane przez kompilator zostanie odjęty 1 punkt. Nie ma punktów ułamkowych.

### Ocena jakości kodu

Ocena jakości kodu jest z przedziału od 0 do 4 punktów. Nie ma punktów ułamkowych. Odejmujemy punkty za:

- złe formatowanie kodu (niepoprawne wstawianie spacji, wcięć, odstępów, magiczne stałe itd.);
- niedostateczne komentarze;
- zbędne pliki;
- niedostateczny podział na moduły;
- niezastosowanie się do paradygmatu programowania obiektowego:
    - brak wirtualnych destruktorów (tam gdzie trzeba), jeśli nie zostanie wykryte przez testy automatyczne;
    - zbyt skomplikowane klasy, brak wydzielenia klas pomocniczych;
    - zależności cykliczne typów;
    - brak właściwej enkapsulacji, m.in. zwracanie modyfikowalnych struktur danych, zbyt duże możliwości modyfikowania stanu obiektu (zbyt dużo publicznych metod, „głupie” publiczne settery lub gettery);
    - instrukcja `switch` zamiast wykorzystania polimorfizmu;
    - zbyt duże klasy lub metody;
    - zbyt dużo powiązań między klasami;
    - powtórzenia kodu;
    - rzutowanie w dół, tego NIE powinno być;
    - niewłaściwe użycie słów kluczowych `public`, `protected`, `private`, `virtual`, `override`, `final`;
- problemy z zarządzaniem pamięcią, niekorzystanie ze sprytnych wskaźników;
- użycie typu zmiennoprzecinkowego lub obliczeń wielokrotnej precyzji;
- niedostateczny `makefile` lub `Makefile`:
    - brak reguły `.PHONY` lub `clean`;
    - wiele szczegółowych reguł zamiast reguły ogólnej;
    - braki w opisie zależności między plikami;
    - braki w kompilowaniu przyrostowym;
- niezastosowanie się do wymagań i uwag sformułowanych w poprzednich zadaniach, np. braki w użyciu słów kluczowych `const` i `noexcept`.

Dodajemy jeden punkt za skorzystanie z `regex` do walidowania poprawności danych wejściowych. Przy czym ocena nie może być wyższa niż 4 pkt.

## Przykład użycia

Przykład użycia jest częścią specyfikacji. Formatowanie wypisywanych danych powinno być dokładnie zgodne z formatowaniem w podanym przykładzie użycia. Przykład użycia znajduje się w załączonych w plikach.
