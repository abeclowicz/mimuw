# Ewaluator

W typowym zastosowaniu uczenia maszynowego do zadań interaktywnych, potrzebujemy zewaluować naszą politykę w danym środowisku. Środowisko to np. gra, w której możemy przejść z jednego stanu do innego wykonując akcję, dostając przy tym nową obserwację stanu. Polityka to coś, np. głęboka sieć neuronowa, co dla danej obserwacji zwraca akcje (na cele tego zadania: jedną akcję).

W pseudo-kodzie synchroniczna ewaluacja wygląda następująco:

```python
def evaluate():
    policy = startSubprocess(/path/to/policy, 0, extraArgs…)

    while (testName := stdin.readline()):
        env = startSubprocess(/path/to/environment, testName, extraArgs…)
        state = env.readline()

        while state[0] != 'T':
            policy.write(state)
            action = policy.readline()

            env.write(action)
            state = env.readline()

        print(testName + ' ' + state)
```

## Zadanie

Celem tego zadania jest napisanie współbieżnej implementacji ewaluacji, tj. programu `evaluator`, który dla danej polityki, środowiska i zestawu testów zwróci ten sam wynik co powyższy kod w jak najkrótszym czasie (patrz niżej).

Polityka i środowisko są dane jako pliki wykonywalne, komunikujące się przez standardowe wejście/wyjście.

Implementacja ewaluatora musi spełniać następujące wymagania:

- Liczba współbieżnie pracujących polityk nie może przekraczać `maxConcurrentPolicyCalls`;
  
  proces polityki *pracuje od* momentu przeczytania stanu do momentu wypisania akcji.
- Łączna liczba współbieżnie pracujących polityk lub środowisk nie może przekraczać `maxConcurrentCalls`;

  proces środowiska *pracuje* od rozpoczęcia do wypisania stanu początkowego, oraz od momentu przeczytania akcji do momentu wypisania nowego stanu.
- Liczba procesów środowisk w żadnym momencie nie może przekraczać `maxActiveEnvironments`.
- Całkowita liczba wywołań środowisk (wypisań akcji) powinna być taka, jak dla rozwiązania sekwencyjnego.
- Należy unikać niepotrzebnego tworzenia procesów polityk i środowisk.

Wymagania są motywowane tym, że w praktyce moglibyśmy mieć do czynienia z następującymi ograniczeniami zasobów:

- każda pracująca polityka używa karty GPU do obliczeń, których mamy `maxConcurrentPolicyCalls`;
- każda pracująca polityka i pracujące środowisko intensywnie używają po jednym rdzeniu CPU, których mamy `maxConcurrentCalls`;
- aktywny stan środowiska zużywa dużo pamięci i mieścimy ich co najwyżej `maxActiveEnvironments`.

Testy do zadania nie będą realnie alokować tych zasobów, tylko symulować to przez sprawdzenie opisanych wcześniej wymagań.

Wymaganie wykonania "w jak najkrótszym czasie" interpretujemy zachłannie: jeśli w danym momencie można wywołać politykę lub środowisko (poprzez wypisanie na ich wejście stanu lub akcji) zachowując powyższe wymagania, to należy to zrobić niezwłocznie. Nie oczekujemy wybierania, który proces wywoływać, z którym stanem lub akcją (nawet jeśli odpowiednie priorytety mogłyby przyspieszyć wykonanie).

## Argumenty i wejście/wyjście programów

Program `evaluator` powinien przyjmować następujące argumenty (po `argv[0]`):

1. `char const *policyPath`: ścieżka do wykonywalnego pliku implementującego politykę.
2. `char const *envPath`: ścieżka do wykonywalnego pliku implementującego środowisko.
3. `int maxConcurrentPolicyCalls`: maksymalna liczba współbieżnie pracujących polityk.
4. `int maxConcurrentCalls`: maksymalna łączna liczba współbieżnie pracujących polityk i środowisk.
5. `int maxActiveEnvironments`: maksymalna liczba procesów środowisk. Pozostałe argumenty `extraArgs` (zero lub więcej argumentów, od `argv[6]`) powinny być przekazane jako dodatkowe argumenty przy wołaniu podprocesów polityki i środowiska.

Program `evaluator` ze standardowego wejścia powinien czytać nazwy testów (ciągi alfanumerycznych znaków ASCII długości dokładnie `NAME_SIZE`), oddzielone znakami nowej linii `\n` (po każdej nazwie jeden znak `\n`, wypisany niezwłocznie po ostatnim znaku nazwy). Nazwy te nie muszą być unikalne – powtórzony test należy wykonać ponownie. Nazwy te mogą być wypisywane na standardowe wejście ewaluatora w dowolnym czasie.

Na standardowe wyście powinien wypisywać linie zawierające:

- nazwę testu,
- spację,
- stan końcowy: ciąg `STATE_SIZE` znaków,
- znak nowej linii `\n`.

Rozwiązanie może wypisywać te linie:

- albo niezwłocznie po zakończeniu testu (tj. po otrzymaniu stanu końcowego z programu środowiska) – takie rozwiązanie dostanie max. 8 / 10 punktów.
- albo w kolejności pojawienia się testów na standardowym wejściu – takie rozwiązanie może dostać pełne 10 / 10 punktów. W tym wariancie linia `i` powinna być wypisana niezwłocznie po zakończeniu testów `0, 1, …, i`.

Wybór wariantu należy zadeklarować w pliku `solution/CMakeLists.txt` przez zdefiniowanie odpowiednio `add_compile_definitions(ORDERED_OUTPUT=0)` lub `1`. Nie należy implementować obu wariantów.

Na standardowe wyjście błędów można wypisywać cokolwiek (o ile nie spowalnia to znacząco rozwiązania).

Program `evaluator` po otrzymaniu sygnału `SIGINT` powinien jak najszybciej zakończyć działanie (z uwzględnieniem, że powinien zakończyć i zaczekać na utworzone procesy, w szczególności, polityk i środowisk). Ponownie otrzymane sygnały `SIGINT` można zignorować. Po zakończeniu standardowego wejścia również powinien niezwłocznie zakończyć działanie.

Program `evaluator` powinien zakończyć działanie z kodem:

- `0` – jeśli wszystkie testy zostały wykonane poprawnie,
- `2` – jeśli otrzymał sygnał `SIGINT`,
- `1` – w pozostałych przypadkach.

## Program środowiska

Program środowiska przyjmuje argumenty:

- nazwę testu (z której wyniknie stan początkowy środowiska);
- wszelkie dodatkowe argumenty `extraArgs`.

Po jakimś czasie od rozpoczęcia, wypisuje na standardowe wyjście stan początkowy (ciąg `STATE_SIZE` znaków) i znak końca linii `\n`, a następnie w pętli:

- (natychmiast) czyta akcję (ciąg `ACTION_SIZE` znaków) i znak końca linii `\n` ze standardowego wejścia,
- (po jakimś czasie) wypisuje nowy stan (`STATE_SIZE`) i znak końca linii `\n` na standardowe wyjście.

Po wypisaniu stanu końcowego (stan zaczynający się od znaku `'T'`) program środowiska kończy działanie.

Program środowiska po otrzymaniu sygnału `SIGINT` kończy działanie – może to trochę potrwać i należy zaczekać na zakończenie (bo program ewaluatora musi posprzątać procesy potomne przed zakończeniem). Wymaganie wykonania "w jak najkrótszym czasie" nie dotyczy tego oczekiwania, nie ma potrzeby go zrównoleglać. Ponowne wysłanie SIGINT do środowiska jest dozwolone ale może zostać zignorowane.

## Program polityki

Program polityki przyjmuje argumenty:

- indeks polityki (od `0`, rośnie dla każdego kolejnego tworzonego procesu polityki);
- wszelkie dodatkowe argumenty `extraArgs`.

W pętli:

- (natychmiast) czyta stan (`STATE_SIZE` znaków i `\n`) ze standardowego wejścia;
- (po jakimś czasie) wypisuje akcję (`ACTION_SIZE` znaków i `\n`) dla tego stanu na standardowe wyjście.

Po wypisaniu stanu na standardowe wejście polityki nie należy wypisywać kolejnego póki nie zostanie odczytana akcja.

Program polityki po otrzymaniu sygnału `SIGINT` kończy działanie jak w przypadku programu środowiska (może to trochę potrwać itd.)

## Gwarancje

Można założyć co następuje:

- Programy polityki i środowiska są poprawne i zawsze działają zgodnie z opisem.
- `1 ≤ maxConcurrentPolicyCalls, maxConcurrentCalls, maxActiveEnvironments ≤ 1'000'000`.
- Liczba linii wypisanych na wejście `evaluator` spełnia `1 ≤ n ≤ 1'000'000'000`.
- Nazwy testów są ciągami alfanumerycznych znaków ASCII o długości dokładnie `NAME_SIZE`.
- Stany i akcje wypisywane przez program środowiska i polityki są ciągami alfanumerycznych znaków ASCII długości dokładnie `ACTION_SIZE` i `STATE_SIZE`, odpowiednio.
- `1 ≤ ACTION_SIZE, STATE_SIZE, NAME_SIZE ≤ 1'000'000'000`.
- Testy zadania wysyłają tylko sygnał SIGINT, tylko do głównego procesu `evaluator`.

## Wymagania techniczne

- Program powinien być napisany w języku C lub C++.
- Nie wolno używać funkcji `pthread_*`.
- Wolno używać jedynie wywołań systemowych omówionych na laboratoriach (w szczególności nie wolno używać funkcji `poll`/`select`).
- Wywołania z rodziny `exec` wolno wołać wyłącznie na plikach wykonywalnych pod ścieżkami `policyPath` i `envPath` (podanymi do programu `evaluator`).
- Przed zawołaniem `exec` należy przywrócić pierwotną maskę sygnałów (jeśli się ją zmieniało).
- Nie wolno używać wywołania `setpgid`. Program `evaluator` zostanie uruchomiony w nowej grupie procesów (nie zawierającej początkowo innych procesów).
- Przy uruchomieniu programu polityki i środowiska, wszelkie deskryptory otwarte przez `evaluator` powinny być zamknięte (może w tym pomóc wywołanie `fnctl(fd, F_SETFD, FD_CLOEXEC)`). Deskryptory otwarte już w momencie uruchomienia programu `evaluator` (np. standardowe lub otwarte przez debugger) powinny zostać niezmienione.
- Program `evaluator` przed normalnym zakończeniem (tj. po wykonaniu wszystkich testów) powinien posprzątać wszystkie otwarte przez siebie zasoby (procesy potomne, pamięć, deskryptory plików otwarte przez `evaluator`, pliki, itp.).
- Program `evaluator` przed wyjątkowym zakończeniem (z powodu sygnału) powinien posprzątać procesy potomne i utworzone pliki.
- Nie wolno kończyć programu `evaluator` inaczej niż zawołaniem `exit()` lub wyjściem z `main()` (tj. nie wolno wołać wariantu `_exit()`/`_Exit()` ani wysyłać sygnału `SIGKILL`).
- Stałe `STATE_SIZE`, `ACTION_SIZE` i `NAME_SIZE` są zdefiniowane w nadrzędnym `CMakeLists.txt` i będą zmieniane w testach.

## Format rozwiązania

- Rozwiązaniem powinno być archiwum postaci takiej jak załączony szablon ab12345.zip, czyli archiwum ZIP o nazwie ab12345.zip lub archiwum tar gzip o nazwie ab12345.tgz; zawierający wyłącznie folder o nazwie ab12345, gdzie zamiast ab12345 należy użyć własnych inicjałów i numeru indeksu (czyli nazwy użytkownika na students).
- Rozwiązanie powinno znajdować się w podfolderze `ab12345/solution/`. Należy tam zawrzeć plik `CMakeLists.txt` z celem `add_executable(evaluator, ...)` i wszelkie pliki i foldery potrzebne do kompilacji.
- Kompilacja odbędzie się następująco (na maszynie jak students – Ubuntu z GCC ≥14.2; ale z dostępnym clang-tidy):

```bash
unzip ab12345.zip
# lub:
tar --gunzip -xf ab12345.tgz
# pliki ab12345/CMakeLists.txt oraz foldery ab12345/common/ i ab12345/demo/
# zostaną skopiowane z oryginalnego archiwum, nadpisując wszelkie zmiany.
cmake -S ab12345/ -B build/ -DCMAKE_BUILD_TYPE=Release
cd build/
make evaluator demo_policy demo_env
echo -ne "AAAA\nQQQQ\n" | setsid ./solution/evaluator ./demo/demo_policy ./demo/demo_env 1 1 1 | diff -s - <(echo -ne "AAAA TZZZ\nQQQQ TZZZ\n")
```

(Ostatnia linia to przykładowe użycie; `setsid` uruchamia program w nowej grupie procesów).

W szczególności:

- Zmiany w `/ab12345/CMakeLists.txt` zostaną cofnięte, ale można zmieniać `CMakeLists.txt` w podkatalogu `solution/`.
- Do folderu `common/` i `demo/` nie należy dodawać własnych plików (zostaną usunięte).
- Foldery inne niż `/ab12345/solution/` będą zignorowane przez `/ab12345/CMakeLists.txt`.
- Można zmieniać `.clang-tidy` i `.clang-format` (użycie nie jest wymagane, jedynie zalecane).
- Nie należy zmieniać flag, opcji, itp. kompilatora, linkera i CMake. Własne pliki `CMakeLists.txt` powinny się ograniczać do `add_executable`, `add_library`, `target_link_libraries` i `add_compile_definitions`.
