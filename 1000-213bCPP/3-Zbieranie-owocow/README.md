# Zbieranie owoców

> [!NOTE]
> Made in collaboration with [@FilipBaciak](https://github.com/FilipBaciak).

## Wstęp

Rozwiązując to zadanie, studenci powinni poznać:

- rodzaje i dostępność konstruktorów,
- tworzenie operatorów jako metod klasowych i funkcji globalnych,
- realizację semantyki przenoszenia,
- operatory porównujące,
- jawne i niejawne konwersje typów,
- listy inicjujące,
- użycie słów kluczowych `const`, `constexpr` i `constinit`,
- użycie słowa kluczowego `inline`.

## Polecenie

Zadanie polega na zaimplementowaniu modułu `fruit_picking` pozwalającego na tworzenie rankingów osób zbierających owoce. W ramach tego modułu powinny być udostępnione następujące klasy:

- `Fruit` – reprezentująca pojedynczy owoc, który opisać można za pomocą jego smaku, wielkości i jakości;

- `Picker` – reprezentująca osobę, która zbiera owoce;

- `Ranking` – reprezentująca ranking zbieraczy owoców.

Atrybuty opisujące owoc powinny uwzględniać jedynie następujące możliwości:

- smak (typu `Taste`) – słodki (`SWEET`) lub kwaśny (`SOUR`);

- wielkość (typu `Size`) – duży (`LARGE`), średni (`MEDIUM`) lub mały (`SMALL`);

- jakość (typu `Quality`) – zdrowy (`HEALTHY`), nadgniły (`ROTTEN`) lub robaczywy (`WORMY`).

Nie powinno być możliwości porównywania ani niejawnej konwersji pomiędzy różnymi atrybutami owocu. W szczególności, nie powinno być możliwości porównywania smaku z wielkością czy jakości ze smakiem. Dostęp do atrybutów owocu powinny realizować jego metody `taste`, `size` i `quality`. Oprócz tego klasa `Fruit` powinna udostępniać:

- jawne tworzenie obiektu (owocu) z trzema parametrami (kolejno smakiem, wielkością i jakością); nie powinno być możliwości tworzenia obiektu bez podania wszystkich tych parametrów;

- tworzenie obiektu za pomocą domyślnego kopiowania oraz przenoszenia;

- jawne konwersje z typu `std::tuple<Taste, Size, Quality>` na `Fruit` i w przeciwną stronę;

- domyślne operacje przypisania w wersji kopiującej i przenoszącej;

- możliwość sprawiania, żeby zdrowy owoc stał się nadgniły;

- możliwość zalęgania się robaków w zdrowym owocu;

- sprawdzanie, czy dwa owoce są równe (operator `==`), co powinno zachodzić tylko wtedy, gdy mają one identyczne atrybuty; sprawdzanie natomiast relacji `<`, `<=`, `>` oraz `>=` pomiędzy dwoma owocami nie powinno być dostępne;

- wypisywanie na standardowe wyjście informacji o owocu w postaci `[smak wielkość jakość]`, zachowując słownictwo z przykładu `fruit_picking_example.out`.

Należy zapewnić, żeby obiekt klasy `Fruit` mógł być używany w wyrażeniach stałych `constexpr`. Ponadto w wyrażeniach stałych powinny być dostępne wszystkie metody, które nie zmieniają stanu obiektu tej klasy. Oprócz tego powinny być dostępne globalne stałe `YUMMY_ONE` i `ROTTY_ONE`, inicjowane tylko podczas kompilowania, które reprezentują odpowiednio zdrowy, słodki, duży owoc oraz kwaśny, mały, nadgniły owoc.

W drugiej wymaganej w zadaniu klasie `Picker` powinna być możliwość:

- przechowywania zebranych owoców – owoce powinny być przechowywane w takiej samej kolejności w jakiej były zbierane.

- tworzenia obiektu osoby o zadanym imieniu zbierającej owoce; w przypadku podania pustego imienia lub nie podania go wcale, tworzona osoba powinna przyjąć imię "Anonim";

- utworzenia obiektu za pomocą domyślnego kopiowania oraz przenoszenia;

- realizacji domyślnych operacji przypisania w wersji kopiującej i przenoszącej;

- pozyskania nazwy osoby zbierającej;

- zebrania pojedynczego owocu operator `+=`; jeśli zebrany owoc jest

  - zdrowy, to staje się on nadgniły, gdy poprzedni zebrany owoc był nadgniły;

  - nadgniły i jeśli poprzedni zebrany owoc był zdrowy, to ten poprzedni staje się nadgniły;

  - robaczywy, to wszystkie zdrowe i słodkie owoce zebrane do tej pory stają się robaczywe;

- zabrania od innego zbieracza jego najwcześniej zebranego owocu za pomocą operatora `+=`;

- oddania swojego pierwszego owocu innemu zbieraczowi, operator `-=`;

- podania liczby wszystkich owoców zebranych przez zbieracza;

- podania liczby zebranych owoców, mających dany smak;

- podania liczby zebranych owoców, mających daną wielkość;

- podania liczby zebranych owoców, które mają daną jakość;

- porównywania osób zbierających (operator `<=>`) dające informację, który zbieracz powinien być wyżej w rankingu; przy porównywaniu sprawdzana jest najpierw liczba zebranych zdrowych owoców; kto ma ich więcej znajdzie się wyżej w rankingu; przy równej liczbie sprawdzana jest liczba posiadanych słodkich owoców; czym ktoś ma ich więcej, tym będzie lepszy w rankingu; jeśli i ta liczba okaże się taka sama, to w analogiczny sposób badane powinny być kolejno: liczby dużych owoców, liczby średnich owoców, liczby małych owoców i na końcu liczby wszystkich zebranych owoców; jeśli żadne z kolejnych kryteriów nie rozstrzyga zwycięzcy, to jest remis; przy remisie, wyżej w rankingu powinna się znaleźć ta osoba, która wcześniej do tego rankingu została wprowadzona;

- sprawdzania równości zbieraczy (operator `==`); równość osób zbierających ma miejsce tylko wtedy, gdy mają to samo imię i zebrali takie same owoce, w takiej samej kolejności;

- wypisania na standardowe wyjście imienia zbieracza i aktualnej listy kolejnych zebranych przez niego owoców; wydruk informacji o osobie zbierającej i jej zbiorach powinien być taki jak w przykładzie `fruit_picking_example.out`; w szczególności, najpierw powinno być wypisane imię i dwukropek, a potem w osobnych wierszach kolejne owoce, każdy poprzedzony znakiem tabulacji.

W ostatniej klasie `Ranking` powinna być możliwość:

- przechowywania osób zbierających owoce, w porządku zgodnym z ich pozycją w rankingu, zaczynając od najlepszego zbieracza;

- utworzenia pustego rankingu lub w oparciu o listę osób zbierających, przekazaną poprzez parametr;

- utworzenia obiektu za pomocą domyślnego kopiowania oraz przenoszenia;

- realizacji domyślnych operacji przypisania w wersji kopiującej i przenoszącej;

- dodania jednej osoby do rankingu, korzystając z operatora `+=`;

- usunięcia z rankingu najwyżej notowanego zbieracza równego podanemu w parametrze, o ile taki występuje w rankingu; operację tę należy realizować operatorem `-=`, który powinien korzystać z operatora `==` z klasy `Picker`;

- łączenia rankingu z innym rankingiem za pomocą operatora `+=`;

- łączenia dwóch rankingów w nowy ranking, za pomocą operatora `+`;

- dostępu do zbieracza znajdującego się na podanej pozycji w rankingu, przy założeniu, że pozycje są numerowane od 0. Pozycja poza zakresem powinna dać w wyniku ostatniego zbieracza w rankingu. Zmiana rankingu poprzez dostęp do zbieraczy uzyskany w ten sposób, nie powinna być możliwa. Tę operację powinien realizować operator `[]`;

- podania liczby wszystkich osób zbierających owoce, które zostały ujęte w rankingu;

- wypisania rankingu na standardowe wyjście; wydruk powinien obejmować kolejnych zbieraczy, zaczynając od pierwszego w rankingu.

## Wymagania formalne

Oczekiwane rozwiązanie powinno korzystać z kontenerów i metod udostępnianych przez standardową bibliotekę C++. Co więcej, powinno ono zapewniać odpowiednią realizację semantyki przenoszenia obiektów tymczasowych.

Należy ukryć przed światem zewnętrznym wszystkie zmienne globalne i funkcje pomocnicze nienależące do wyspecyfikowanego interfejsu modułu.

Rozwiązanie powinno zawierać jedynie plik nagłówkowy `fruit_picking.h`. Pliki ten należy umieścić w Moodle. Rozwiązanie będzie kompilowane na maszynie *students* poleceniem

```bash
g++ -Wall -Wextra -O2 -std=c++23 *.cpp
```

## Ocenianie rozwiązania

### Ocena automatyczna

Za testy automatyczne zostanie przyznana ocena z przedziału od 0 do 6 punktów. Za błędną nazwę pliku zostanie odjęty 1 punkt. Za ostrzeżenia wypisywane przez kompilator zostanie odjęty 1 punkt. Nie ma punktów ułamkowych.
Ocena jakości kodu

Ocena jakości kodu jest z przedziału od 0 do 4 punktów. Nie ma punktów ułamkowych. Odejmujemy punkty za:

- brzydkie formatowanie kodu (niepoprawne wstawianie spacji, wcięć, odstępów, brak komentarzy, magiczne stałe itd.);

- nieprawidłowe, niedostateczne używanie `const`, o ile testy tego nie wykryły;

- zły dobór typu lub kontenera, brak nazw typów, niewiele mówiące nazwy typów i zmiennych;

- rozwlekłą lub nieelegancką strukturę programu, rozpatrywanie zbyt wielu warunków brzegowych, powtarzanie kodu, przechowywanie liczb jako napisów;

- korzystanie z wejścia-wyjścia dostarczanego przez bibliotekę C zamiast ze strumieni;

- nieuzasadnione przekazywanie funkcjom przez wartość dużych obiektów;

- deklarowanie składowych lub metod jako `public`, gdy wystarczyłoby `private` lub `protected`, nadużywanie relacji `friend`;

- niezgodność publicznego interfejsu klas z treścią zadania;

- brak użycia tam gdzie jest to wymagane operatora `<=>`, definiowanie zbyt wielu operatorów, zbyt wielu lub zbyt małej liczby wersji operatorów;

- brak użycia listy inicjującej w konstruktorze;

- wprowadzanie sztucznych ograniczeń na rozmiar danych;

- nieusuwanie lub nieefektywne usuwanie niepotrzebnych już danych;

- brak *header guard*;

- nieukrycie przed światem zewnętrznym wszystkich zmiennych globalnych i funkcji pomocniczych nienależących do wyspecyfikowanego interfejsu modułu;

- użycie `typedef` zamiast `using`;

- inne znalezione i niewymienione w powyższych kryteriach błędy i niezgodności z treścią zadania lub odpowiedziami udzielonymi na forum, a niewykryte przez testy automatyczne;

- niezastosowanie się do uwag udzielonych w poprzednich zadaniach.

Nie wymagamy jeszcze prawidłowego oznaczania metod niezgłaszających wyjątków.

## Przykłady użycia

Przykłady użycia modułu `fruit_picking` znajdują się w pliku `fruit_picking_example.cpp`. Przykłady wydruków wypisywanych przez program `fruit_picking_example` znajdują się w pliku `fruit_picking_example.out`. Przykłady te są integralną częścią specyfikacji zadania.