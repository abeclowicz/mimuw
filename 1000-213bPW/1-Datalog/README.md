# Datalog

Zadanie polega na implementacji wielowątkowego silnika do wykonywania zapytań w uproszczonej wersji języka *Datalog*, używanego w zastosowaniach bazodanowych. Dany jest parser oraz implementacja jednowątkowa.

Niemałą częścią zadania jest przeczytanie i zrozumienie danego kodu i dokumentacji.

## Spis Treści

- [Datalog i jego semantyka](#datalog-i-jego-semantyka)
    - [Składnia plików wejściowych](#składnia-plików-wejściowych)
        - [Przykład](#przykład)
    - [Semantyka, czyli kiedy stwierdzenie jest wyprowadzalne](#semantyka-czyli-kiedy-stwierdzenie-jest-wyprowadzalne)
        - [Przykład (ponownie)](#przykład-ponownie)
        - [Przykład z pętlą](#przykład-z-pętlą)
- [Rozszerzenie Datalogu o wyrocznię](#rozszerzenie-datalogu-o-wyrocznię)
    - [Przykład z wyrocznią](#przykład-z-wyrocznią)
- [Jednowątkowy silnik ewaluacji zapytań](#jednowątkowy-silnik-ewaluacji-zapytań)
    - [Zbiór `inProgressStatements`](#zbiór-inprogressstatements)
    - [Zbiór dawany w wyniku przez `deriveStatement`](#zbiór-dawany-w-wyniku-przez-derivestatement)
- [Instrukcja](#instrukcja)
- [Zawartość archiwum](#zawartość-archiwum)

## Datalog i jego semantyka

Język *Datalog* pozwala na wyliczanie zapytań odpowiadających na pytania o relacje między elementami na podstawie danych reguł.

W programie w języku Datalog mamy dany (skończony) zbiór *stałych* i zbiór *predykatów*: nazw określających, o jakie relacje między stałymi możemy pytać. *Stwierdzenie* (*statement*) to wyrażenie postaci `predykat(c₁, … , cₙ)`, gdzie `c₁`, `…`, `cₙ` są stałymi (ze zbioru stałych). Stwierdzenia te wyprowadzamy na podstawie *reguł* (*rules*) danych w programie; każde stwierdzenie jest więc albo *wyprowadzalne* (*derivable*), albo *niewyprowadzalne* (*non-derivable*).

Reguły w języku Datalog są postaci:

```
atom₁ :- atom₂, ..., atomₖ.
```

gdzie każdy *atom* jest wyrażeniem postaci `predykat(e₁, … , eₙ)`, w którym `e₁`, `…`, `eₙ` są stałymi (ze zbioru stałych) albo *zmiennymi* (nazwami spoza zbioru stałych). Reguła składa się z *głowy* (przed napisem `:-`) oraz *ciała*, czyli listy *przesłanek* (po tym napisie, oddzielone przecinkami). Reguła zakończona jest kropką.

Intuicyjnie, reguła określa implikację w lewo: dla każdego podstawienia zmiennych, jeśli wszystkie przesłanki są wyprowadzalne, to głowa jest wyprowadzalna. Bardziej szczegółowy opis znajduje się poniżej, w sekcji **Semantyka, czyli kiedy stwierdzenie jest wyprowadzalne**.

Zauważmy, że stwierdzenie jest tym samym, co atom z samymi stałymi (bez zmiennych).

### Składnia plików wejściowych

W naszej wersji plik wejściowy opisujący program to tekst ASCII następującej postaci.

```
Constants: ...

Rules:
    ...

Queries: ...
```

Jak widać, plik wejściowy składa się trzech sekcji, zawierających odpowiednio *stałe*, *reguły* i *zapytania*. Zbiór *predykatów* wnioskujemy z nazw użytych w regułach i zapytaniach – w poprawnym programie wszystkie użycia każdego predykatu `p` powinny mieć tę samą *arność* `ar(p)` (liczbę rozdzielonych przecinkami elementów w nawiasie po nim). W poprawnym programie wszystkie użyte stałe są wymienione w pierwszej sekcji (mogą też tam występować stałe nieużyte nigdzie później).

*Stałe* są oddzielone przecinkami. Nazwy stałych zaczynają się od małej litery i składają się wyłącznie z małych liter, cyfr i znaku podkreślenia `_`.

Nazwy *zmiennych* zaczynają się od wielkiej litery i składają się wyłącznie z wielkich liter, cyfr i znaku podkreślenia `_`.

Każda *reguła* zakończona jest kropką i musi zawierać napis `:-` (w innych implementacjach czasem pomija się ten napis z reguł bez przesłanek).

*Zapytania* (*queries*) to lista stwierdzeń, o których wyprowadzalność pytamy. Na cele tego zadania nie dopuszczamy zapytań ze zmiennymi, w poprawnym programie ich nie będzie.

#### Przykład

```
Constants: a, b

Rules:
    blue(b) :- .
    red(X) :- blue(X), blue(b).

Queries: blue(a), blue(b), red(b)
```

W powyższym przykładzie mamy program w Datalogu, zawierający dwie stałe `a` i `b`, dwie reguły i trzy zapytania. Pierwsza reguła nie ma przesłanek. Druga reguła ma dwie przesłanki; w głowie i w jednej z przesłanek występuje zmienna `X`.

### Semantyka, czyli kiedy stwierdzenie jest wyprowadzalne

**Podstawienie** dla reguły to funkcja ze zbioru zmiennych występujących w tej regule do zbioru stałych. Funkcję podstawienia rozszerzamy na zbiór stałych identycznościowo, tzn. `σ(c) = c` dla każdej stałej `c`.

Stwierdzenie `p(c₁, c₂, … , cₙ)` jest ***wyprowadzalne***, jeśli wśród reguł występuje reguła postaci

```
h(e₁, e₂, …) :- b¹(e¹₁, e¹₂, …), … , bᵏ(eᵏ₁, eᵏ₂, …).
```

oraz istnieje podstawienie `σ`, takie że:

- `h(σ(e₁), σ(e₂), …)` jest tym samym stwierdzeniem co `p(c₁, c₂, …)` (w szczególności `h` jest tym samym predykatem co `p`) oraz
- każde z `k` następujących stwierdzeń jest wyprowadzalne:

```
b¹(σ(e¹₁), σ(e¹₂), …)
…
bᵏ(σ(eᵏ₁), σ(eᵏ₂), …)
```

Mówimy, że `p(c₁, c₂, …)` jest ***bezpośrednio wyprowadzalne*** z tej reguły, tego podstawienia i tych `k` stwierdzeń.

W szczególności, dla reguły bez przesłanek:

```
h(e₁, e₂, …) :- .
```

każde podstawienie zmiennych daje inne wyprowadzalne stwierdzenie (bezpośrednio z pustego zbioru stwierdzeń).

Stwierdzenie, którego nie da się wyprowadzić wielokrotnym użyciem powyższej definicji, jest ***niewyprowadzalne***.

Inaczej mówiąc, powyższą rekurencyjną definicję wyprowadzalności można wyrazić bez rekurencji następująco: stwierdzenie `p(c₁, c₂, …)` jest wyprowadzalne, jeśli istnieje skończone drzewo, które w korzeniu ma `p(c₁, c₂, …)`, a każdy węzeł ma stwierdzenie bezpośrednio wyprowadzalne ze stwierdzeń w potomkach. (W szczególności liście muszą zawierać stwierdzenia wyprowadzalne z reguł bez przesłanek).

#### Przykład (ponownie)

Rozważmy ponownie przedstawiony wyżej program:

```
Constants: a, b

Rules:
    blue(b) :- .
    red(X) :- blue(X), blue(b).

Queries: blue(a), blue(b), red(b)
```

Na zapytanie `blue(b)` odpowiedzią jest "*tak*", bo stwierdzenie to jest wyprowadzalne z reguły `blue(b) :- .`, gdyż po pustym podstawieniu `σ`:

- głowa reguły jest równa zapytaniu;
- każda przesłanka jest wyprowadzalna (bo nie ma przesłanek).

Na zapytanie `red(b)` odpowiedzią jest "*tak*", bo stwierdzenie to jest wyprowadzalne z drugiej reguły, gdyż po podstawieniu w niej `σ(X) = b`:

- głowa reguły staje się równa zapytaniu `red(b)`;
- obie przesłanki są równe stwierdzeniu `blue(b)`, którego wyprowadzalność już uzasadniliśmy.

Na zapytanie `blue(a)` odpowiedzią jest "*nie*", bo jedyną regułą pozwalającą bezpośrednio wyprowadzić stwierdzenia z predykatem `blue` jest pierwsza reguła, a w pierwszej regule żadne podstawienie nie zmieni głowy na `blue(a)`.

Gdyby dodać jeszcze zapytanie `red(a)`, odpowiedzią na nie byłoby "*nie*", bo jedyny sposób, aby je bezpośrednio wyprowadzić, polega na użyciu drugiej reguły z podstawieniem `σ(X) = a`, ale wtedy konieczne byłoby wyprowadzenie stwierdzenia `blue(a)`, co nie jest możliwe.

#### Przykład z pętlą

Rozważmy następujący przykład:

```
Constants: a, b
Rules:
    blue(b) :- .
    blue(X) :- blue(a).
Queries: blue(a), blue(b)
```

Tutaj stwierdzenie `blue(b)` jest wyprowadzalne. Z kolei stwierdzenie `blue(a)` jest niewyprowadzalne: użycie drugiej reguły z podstawieniem `σ(X)=a` wymagałoby wyprowadzenia `blue(a)`, a żadna inna reguła ani podstawienie nie pozwalają tego wyprowadzić.

Jeśli odwołamy się do nierekurencyjnego sposobu wyprowadzania, to można sobie powyższą sytuację wyobrazić tak, że próba stworzenia drzewa derywacji zakończyła się niepowodzeniem, bo próba utworzenia drzewa doprowadziła do powstania pętli. W tym przypadku pętla prowadzi bezpośrednio od założenia reguły do jej głowy. Jednak w praktyce mogą pojawiać się pętle o większej długości i mogą przy próbie stworzenia wyprowadzenia dla różnych podstawień pojawiać się różne pętle.

Więcej przykładów można znaleźć w katalogu `examples` w dołączonym archiwum.

## Rozszerzenie Datalogu o wyrocznię

W tym zadaniu razem z programem w Datalogu dana będzie także *wyrocznia*, czyli obiekt klasy implementującej interfejs `AbstractOracle`, reprezentujący dodatkowe źródło informacji o stwierdzeniach:

```java
public interface AbstractOracle {
    abstract public boolean isCalculatable(Predicate predicate);
    abstract public boolean calculate(Atom statement) throws InterruptedException;
}
```

Wyrocznia jest odpowiedzialna za „kalkulowalne” predykaty. O tym, czy predykat jest kalkulowalny, decyduje metoda `isCalculatable`. Dla stwierdzeń z kalkulowalnym predykatem, zamiast stwierdzać wyprowadzalność na podstawie reguł języka Datalog, należy wywołać `calculate` i uznać odpowiedź za wiążącą. Wszelkie reguły z kalkulowalnym predykatem w głowie stają się więc nieistotne.

Formalnie, dla danego programu z wyrocznią, stwierdzenie `p(c₁, c₂, …)` jest wyprowadzalne, jeśli istnieje skończone drzewo, które:

- w korzeniu ma `p(c₁, c₂, …)`, oraz
- każdy węzeł ma stwierdzenie `s`, takie że,
    - predykat w `s` jest kalkulowalny i `calculate(s)` daje w wyniku `true`, lub
    - predykat w `s` nie jest kalkulowalny i `s` jest bezpośrednio wyprowadzalne ze stwierdzeń w potomkach.

(W szczególności predykaty kalkulowalne występują tylko w liściach).

Obie metody kalkulatora są deterministyczne i nie mają skutków ubocznych. Metoda `isCalculatable` daje wynik natychmiast, natomiast metoda `calculate` może wykonywać długotrwałe obliczenia (ale niezwłocznie kończy działanie, jeśli zostanie przerwana przy pomocy `Thread.interrupt()`). Nie należy wołać `calculate()` na atomie ze zmiennymi lub na stwierdzeniu z niekalkulowalnym predykatem. Zachowanie w takich sytuacjach jest niezdefiniowane.

### Przykład z wyrocznią

Poniższy program modeluje osiągalność w grafie skierowanym: reguły wyrażają, że relacja `reach` jest domknięciem przechodnim relacji `arc`.

```
Constants: a, b, c, d, e
Rules:
    arc(a, b) :- .
    reach(X, Y) :- arc(X, Y).
    reach(X, Z) :- reach(X, Y), reach(Y, Z).
Queries: reach(a, e)
```

Bez wyroczni, jedynie stwierdzenia `arc(a,b)` oraz `reach(a,b)` byłyby wyprowadzalne. Natomiast gdy spojrzymy na ten sam program z następującą wyrocznią

```java
bool isCalculatable(Predicate p) { return p.id.equals("arc"); };
bool calculate(Atom statement) { return statement.toString().equals("arc(c,d)"); }
```

to wyłącznie stwierdzenia `arc(c,d)` oraz `reach(c,d)` są wyprowadzalne (stwierdzenie `arc(a,b)` jest niewyprowadzalne).

## Jednowątkowy silnik ewaluacji zapytań

W tej sekcji opisujemy działanie jednowątkowego silnika wyprowadzania stwierdzeń.

Dołączony kod w archiwum zawiera implementację w `cp2025.engine.SimpleDeriver`, wraz z klasami pomocniczymi. Implementacja ta próbuje wyprowadzić po kolei każde zapytanie (lub stwierdzić niewyprowadzalność, wyczerpując przeszukiwanie wszystkich możliwości) i zapamiętuje przy tym wszystkie stwierdzenia, dla których udało się stwierdzić wyprowadzalność lub niewyprowadzalność.

Użyty algorytm polega na wyprowadzaniu rekurencyjnie "top-down", przeszukiwaniem w głąb:

- Zaczynamy od wywołania `deriveStatement` dla stwierdzenia w zapytaniu.
- Sprawdzamy każdą regułę i przy każdej ustalonej regule każde przypisanie, które zamieni głowę w szukane stwierdzenie (tzw. *unifikacja*), a następnie rekurencyjnie próbujemy wyprowadzić ciało (tj. wszystkie przesłanki, po przypisaniu) wywołaniem `deriveBody(body)`.
- Metoda `deriveBody` rekurencyjnie sprawdza każdą przesłankę metodą `deriveStatement`.
- W `deriveStatement`, gdy tylko znajdziemy regułę i przypisanie, przy którym ciało jest wyprowadzalne, kończymy, stwierdzając wyprowadzalność – dajemy w wyniku `true`. W przeciwnym wypadku (gdy wyczerpiemy wszystkie przypisania i reguły), dajemy w wyniku `false`.
- W `deriveBody`, gdy tylko znajdziemy w ciele przesłankę, której nie udaje się wyprowadzić, stwierdzamy, że tego ciała nie udało nam się wyprowadzić – dajemy w wyniku `false`. W przeciwnym wypadku (gdy udało się wyprowadzić wszystkie przesłanki), dajemy w wyniku `true`.
- Gdy rekurencja się zapętli, tj. gdy rozpoczniemy wywołanie `deriveStatement` dla stwierdzenia już w trakcie wyprowadzania, to uznajemy, że tego stwierdzenia nie udało nam się w ten sposób wyprowadzić (ale być może da się wyprowadzić inaczej) – dajemy w wyniku `false`.

Wynik `false` metody `deriveStatement` nie oznacza więc zawsze niewyprowadzalności, bo może (bezpośrednio lub pośrednio) wynikać z dojścia do pętli w wyprowadzeniu. Uściślimy to, wprowadzając następujący zbiór.

### Zbiór `inProgressStatements`

W trakcie rekurencji utrzymujemy zbiór `inProgressStatements` stwierdzeń, dla których trwa wyprowadzanie (tj. są obecnie na stosie rekurencyjnych wywołań `deriveStatement`). Ściśle rzecz biorąc, metoda `deriveStatement` podaje wartość "czy `statement` jest wyprowadzalne bez użycia `inProgressStatements`". W ten sposób unikamy zapętleń w rekurencji. Kiedy więc `deriveStatement` daje w wyniku `false`, nie świadczy to w ogólności o niewyprowadzalności `statement`, tylko pozwala nam powiedzieć, że:

```
(*) Jeśli wszystkie stwierdzenia w `inProgressStatements` są niewyprowadzalne,
to `statement` jest niewyprowadzalne.
```

Zaczynamy od zapytania z pustym `inProgressStatements`, więc dla tego zapytania (tj. dla korzenia rekurencji) jest to równoznaczne z powiedzeniem, że jest ono niewyprowadzalne (bezwarunkowo).

### Zbiór dawany w wyniku przez `deriveStatement`

Powyższą własność `(*)` można użyć do wywnioskowania niewyprowadzalności nie tylko dla korzenia rekurencji. Rozpatrzmy sytuację, w której w korzeniu rekurencji mamy wywołanie `deriveStatement(s1)`, które wywołało (między innymi) `deriveStatement(s2)`, które z kolei wywołało (między innymi) `deriveStatement(s3)`. Jeśli każde z tych trzech zagnieżdżonych wywołań da w wyniku `false`, to:

- `deriveStatement(s1)` było wołane z pustym `inProgressStatements`, więc `s1` jest niewyprowadzalne;
- `deriveStatement(s2)` było wołane z `inProgressStatements={s1}`, więc `s2` jest niewyprowadzalne;
- `deriveStatement(s3)` było wołane z `inProgressStatements={s1,s2}`, więc `s3` jest niewyprowadzalne.

Możemy w ten sposób wywnioskować niewyprowadzalność dla stwierdzeń w rekurencyjnych wywołaniach, które dały w wyniku `false` i dla których wszystkie nadrzędne wywołania (aż do korzenia włącznie) dały w wyniku `false`.

Zbiór takich stwierdzeń możemy wyliczyć rekurencyjnie, dając w wyniku `deriveStatement(s)` nie tylko wartość `boolean`, ale też zbiór stwierdzeń:

- jeśli `deriveStatement(s)` daje w wyniku `true`, to jednocześnie daje zbiór pusty;
- jeśli `deriveStatement(s)` daje w wyniku `false`, to jednocześnie daje sumę: `{s}` i wszystkich zbiorów z rekurencyjnych wywołań.

Dopiero na koniec rekurencji możemy stwierdzić niewyprowadzalność całego zbioru podanego w korzeniu rekurencji.

## Instrukcja

Zadanie polega na zaimplementowaniu klasy `cp2025.engine.ParallelDeriver`, implementującej interfejs `AbstractDeriver` (z jedną metodą, która dla danego programu i wyroczni zwraca słownik wyników: `boolean` dla każdego zapytania), dającej takie same wyniki jak `SimpleDeriver`.

W rozwiązaniu:

- Konstruktor klasy `ParallelDeriver` powinien przyjmować jeden argument typu `int`: liczbę wątków pomocniczych. Będzie ona zawsze co najmniej 1.
- Wywołanie `derive(program, oracle)` powinno utworzyć co najwyżej tyle wątków pomocniczych.
- W wątku wykonującym `derive(program, oracle)` nie należy wykonywać ciężkich obliczeń (np. wywołań `calculate`).
- Rozwiązanie powinno zrównoleglać pracę wyłącznie na poziomie zapytań: nie oczekujemy dzielenia pracy związanej z jednym zapytaniem między wiele wątków, nie należy tego w tym zadaniu robić.
- W szczególności, jeśli program zawiera tylko jedno zapytanie, rozwiązanie działa bez równoległych obliczeń.
- Rozwiązanie powinno korzystać ze wspólnej (dla jednego wywołania `derive(program, oracle)`) bazy stwierdzeń wyprowadzalnych i niewyprowadzalnych: jeśli jeden wątek zacznie wyprowadzać stwierdzenie, dla którego drugi wątek wyliczył już odpowiedź, pierwszy wątek powinien z tego skorzystać i wcześniej zakończyć.
- Wątki powinny powiadamiać się o interesujących ich stwierdzeniach: jesli jeden wątek stwierdzi wyprowadzalność lub niewyprowadzalność stwierdzenia `s`, które jest obecnie przetwarzane przez drugi wątek (jest w `inProgressStatements`), to drugi wątek powinien o tym niezwłocznie dostać informację i użyć jej. To znaczy, jeśli w drugim wątku trwa wyprowadzanie `s`, to powinno ono zostać bezzwłocznie przerwane (bez wycieków pamięci), a poznany status wyprowadzalności `s` powinien zostać użyty do określenia wyprowadzalności innych stwierdzeń tak, jakby drugi wątek sam ten status określił. Należy również przerwać wyprowadzanie tych stwierdzeń, dla których ustalenie wyprowadzalności nie jest już potrzebne (bo były wyprowadzane tylko w celu wyprowadzenia `s`).
- W szczególności, jeśli w powyższej sytuacji drugi wątek jest w trakcie wywołania `calculate()`, powinien zostać przerwany wywołaniem `Thread.interrupt()`.
- Jeśli wątek wykonujący `derive(program, oracle)` zostanie przerwany, powinien niezwłocznie zakończyć, wyjątkiem `InterruptedException`.
- Pomocnicze zasoby powinny być zwolnione wraz z zakończeniem działania `derive(program, oracle)`, to znaczy wątki pomocnicze powinny zakończyć pracę i nie powinno być wycieków pamięci.
- W szczególności wątki pomocnicze muszą odróżnić następujące dwie sytuacje:
    - przerwanie z powodu otrzymania informacji o przetwarzanym stwierdzeniu (wtedy powinny niezwłocznie zakończyć przetwarzanie tego stwierdzenia, ale niekoniecznie przetwarzanie całego zapytania), od
    - przerwania z powodu przerwania `derive(program, oracle)` (wtedy powinny niezwłocznie zakończyć całą pracę).

Można założyć, że:

- `derive(program, oracle)` dostaje poprawny program (tzn. `program.validate()` nie rzuca wyjątku).
- W każdym teście wykonywane jest co najwyżej jedno wywołanie `derive(program, oracle)`.

Format rozwiązania:

- Na moodle należy wysłać archiwum o nazwie postaci `ab12345.tgz` (tar gzip) lub `ab12345.zip`, gdzie `ab12345` należy zastąpić swoimi inicjałami i numerem indeksu.
- Archiwum powinno zawierać jeden folder o nazwie postaci `ab12345` i nic poza zawartością tego folderu.
- W tym folderze powinien się znajdować projekt, tzn. co najmniej `src/main/java/cp2025/engine/ParallelDeriver.java`.
- Własny kod można dodawać jedynie do pliku `ParallelDeriver.java` oraz nowych plików pomocniczych w `src/main/java/cp2025/engine/`.
- Nie należy modyfikować żadnych istniejących plików poza `ParallelDeriver.java` – ich zawartość zostanie nadpisana.
- Po rozpakowaniu na students powinno działać `cd ab12345; make test`.

Pytania do zadania należy zadawać na forum w Moodle do 19 listopada 23:59. (Jest to termin istotnie wcześniejszy od terminu oddawania zadania, aby ewentualne ujednoznacznienia i wyjaśnienia nie pojawiały się w ostatniej chwili).

## Zawartość archiwum

Do treści zadania załączone jest archiwum z kodem parsera i silnika jednowątkowego. Parser i testy wymagają zewnętrznych bibliotek, dlatego w projekcie używany jest system budowania: Maven. Otwarcie projektu w IDE powinno (np. VS Code z zainstalowanym Extension Pack for Java) umożliwić proste budowanie i wykonywanie. Alternatywnie, można też w konsoli używać załączonego `Makefile`. Projekt stosuje się do Maven Standard Directory Layout.

Na kod źródłowy składają się:

- `src/main/java/cp2025/datalog/`: automatycznie wygenerowany kod parsera.
    - Można go wygenerować ponownie na podstawie gramatyki określonej w pliku `/Datalog.cf` poleceniem `make datalog`.
- `src/main/java/cp2025/engine/`:
    - `Datalog.java`: podstawowe struktury opisujące program w Datalogu (atomy, reguły itp.), z funkcjami pomocniczymi:
        - `toString`
        - `Datalog.Program.validate()`,
        - `List<Variable> getVariables(List<Atom>)`.
    - `Parser.java`: wrapper dla wygenerowanego parsera w postaci statycznych metod, które przyjmują kod programu (jako ścieżkę do pliku, otwarty strumień znaków `Reader`, lub bezpośrednio jako `String`) i zwracają zwalidowaną strukturę `Datalog.Program`.
    - `AbstractDeriver.java` i `AbstractOracle.java`: interfejs silnika i wyroczni.
    - `NullOracle.java`: trywialna wyrocznia (w której żaden predykat nie jest kalkulowalny).
    - `Main.java`: przykład użycia.
    - `SimpleDeriver.java`: opisany wyżej silnik jednowątkowy.
    - `ParallelDeriver.java`: pusty szablon do wypełnienia.
    - `FunctionGenerator.java`: klasa pomocnicza pozwalająca iterować po wszystkich przypisaniach.
    - `Unifier.java`: statyczne metody pomocnicze do unifikacji reguły z celem (wyprowadzanym stwierdzeniem).
- `test/java/cp2025/`: przykładowe podstawowe testy (ich powodzenie nie gwarantuje żadnych punktów).

Poza tym w archiwum znajdują się:

- `examples/`: przykłady programów w języku Datalog z wynikami (przy użyciu `NullOracle`).
- `lib/`: zależności.
- `.vscode/java-formatter.xml`: konfiguracja automatycznego formatowania kodu (`Ctrl+Shift+I` w VS Code).
- `Makefile`: ułatwia użycie Maven z konsoli.
- `mvnw`, `mvnw.cmd`: pliki wykonywalne Maven Wrapper, które powinny pozwolić na zbudowanie projektu na dowolnym systemie (zalecamy mimo wszystko pracę w środowisku `students`).
- `pom.xml`: specyfikacja projektu do zbudowania przez Maven (wersja Javy, zależności).
- `bnfc`: plik wykonywalny BNFC do generowania parserów.
