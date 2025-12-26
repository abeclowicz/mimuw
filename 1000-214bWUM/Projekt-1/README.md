# Projekt 1

Zespół analityków zdecydował się zaproponować metodę redukcji palety barw obrazów opartą na algorytmach klasteryzacji. Osoby tworzące zespół zauważyły, że gdy rozpatruje się zdjęcia piksel po pikselu, otrzymuje się wysoką liczbę unikatowych kodów barw. Dla przeciętnego odbiorcy być może jednak nie jest aż tak istotna różnica o jedną wartość na pojedynczej współrzędnej RGB lub HSL (standardy reprezentacji barw) tak zdefiniowanych barw na niewielu pikselach. Zespół zaproponował, żeby zmniejszyć liczbę barw opisujących piksele obrazka, wykorzystując metody klastrowania.

**Celem** zadania jest ocena jakości działania przedstawionej metody redukcji palety barw.

**Dane**: Otrzymują Państwo zbiór 132 zdjęć w formacie .png. Zbiór zawiera zdjęcia kotów (kotek-%d.png) oraz kwiatów (kwiatek-%d.png). Każde ze zdjęć ma wymiary 100 na 100 px. Identyfikacja dotyczy głównego obiektu na zdjęciu poza trawą.

*Źródło zdjęć:* https://commons.wikimedia.org oraz archiwum prywatne.

**Wynikiem** ma być raport w notatniku jupyter (.ipynb) wraz z zagnieżdzonymi w nim wykresami i wynikami analiz. **Raport i komentarze muszą być wystarczające do zrozumienia i odtworzenia podejmowanych przez Państwa kroków bez konieczności czytania i uruchamiania Państwa kodów.** Każde podjęte działanie modyfikujące w istotny sposób bazę (np. usuwanie rekordów, modyfikacja i wprowadzanie nowych zmiennych) musi być uzasadnione i opisane. W każdym zadaniu można skorzystać z gotowych implementacji. W przypadku konieczności zaokrągleń, przyjąć standardowe zasady. W moodle przedmiotu pojawi się zadanie – miejsce do przesłania raportu. Raport oceniany będzie przez prowadzącego Państwa grupę.

**Zadanie 1:** Przygotuj dwa zbiory danych na potrzeby dalszej analizy (w postaci *data frame*):

- (a) zawierający informacje o barwach i położeniu pikseli w każdym zdjęciu (jeden piksel w każdym wierszu). Informacja o barwach powinna zawierać wszystkie składowe RGB i HSL.

- (b) zawierający zliczenia wystąpień we wszystkich plikach dla każdej unikatowej barwy z pierwszego zbioru w podziale na zdjęcia kotów oraz zdjęcia kwiatów. Dla każdej barwy na podstawie standardu RGB przygotuj trzy zmienne binarne, czy dana składowa dominuje (1, jeśli dominuje, 0 jeśli nie; przy remisach przyjąć 1 dla odpowiednich składowych, np. RGB 128, 128, 0 ma dominującą składową R oraz G, a RGB 60, 30, 65 ma dominującą składową B). Na podstawie standardu HSL stwórz zmienne binarne *czy_zimny* (składowa H pomiędzy 180 a 270) oraz *czy_nasycony* (składowa S powyżej 70%).

Żeby sprawdzić poprawność stworzonej bazy, wybierz zdjęcie kota o numerze równym dwóm ostatnim cyfrom Twojego numeru indeksu podzielonym przez dwa. Wyświetl to zdjęcie na podstawie oryginalnego pliku oraz spisanych kodów barw.

Wskazówka: możesz rozważyć biblioteki `IPython.display` lub `PIL` (Pillow). Do konwersji barw możesz zastosować bibliotekę `colorsys`.

**Zadanie 2:** Podsumuj w dwóch-trzech zdaniach uzyskane zbiory danych i ich zawartość. Zadania pomocnicze:

- Dla każdego zdjęcia oblicz liczbę unikatowych barw, które zawiera. Przedstaw i skomentuj wartość minimalną, maksymalną, średnią, medianę oraz odchylenie standardowe liczby unikatowych barw na zdjęcie. Ile jest zdjęć kotów, a ile kwiatów?
- Przedstaw po pięć barw, które pojawiają się najczęściej w zbiorze zdjęć kotów oraz zbiorze zdjęć kwiatów. Przedstaw i skomentuj wartość minimalną, maksymalną, średnią, medianę oraz odchylenie standardowe jasności oraz nasycenia dla zdjęć kotów oraz zdjęć kwiatów.
- Podsumuj uzyskany zbiór danych przynajmniej trzema różnymi wykresami (skomentuj każdy z wykresów). Podstawowy zestaw wykresów zawiera:
    - Histogram liczby unikatowych barw na zdjęcie w podziale na zdjęcia kodów oraz kwiatów (dwa histogramy)
    - Wykres rozrzutu (*scatterplot*) dla liczby pojawień danej barwy w bazie kotów (współrzędna x) oraz w bazie kwiatów (współrzędna y). Punkty należy pokolorować odpowiadającym kodem RGB
    - Osobno dla bazy zdjęć kotów oraz bazy zdjęć kwiatów po wykresie słupkowym skumulowanym (*stacked bar chart*) da wybranej zmiennej binarnej z bazy barw.

**Zadanie 3:** Osoby z zespołu analityków wahają się pomiędzy użyciem metody k-średnich (np. `KMeans` z pakietu `scikit-learn`) lub k-median (np. `Kmedoids` z pakietu `scikit-learn-extra`) do redukcji palety barw na podstawie wybranego standardu (RGB lub HSL) na zdjęciu. Mają też wątpliwości, jak dobrać dla danej bazy zdjęć zasadne wartości hiperparametrów dla metody.

- Zdecyduj, jakie zmienne ze zbioru danych z Zadania 1 zastosujesz w klasteryzacji. Zdecyduj, czy wymagają dodatkowej transformacji (np. skalowania). Odpowiedzi uzasadnij.
- Dla każdego zdjęcia rozkodowanego na piksele przeprowadź na podstawie wybranych w poprzednim punkcie zmiennych analizę klastrowania metodą k-średnich oraz k-median, dla $k$ należących do przedziału [5, 20]. Dla każdego zdjęcia wywołaj każdy algorytm 5 razy (aby zmniejszyć wpływ losowości). Dla każdego z pięciu wywołań oblicz średnią silhouette oraz wartość Caliński-Harabasz Index (CHI). Zapisz średnie wyniki z otrzymanych wartości w 5 wywołaniach dla danego obrazka.
*Uwaga*: Dla replikowalności rozwiązania ustaw ziarna losowań na wartości {`1104`, `12041`, `130412`, `140413`, `150414`}
- Dla każdego algorytmu przygotuj po dwa wykresy: na jednym dla każdej wartości k narysuj wykres skrzypcowy (*violin plot*) dla uzyskanych średnich Silhouette w pięciu iteracjach na zdjęcie a na drugim dla każdej wartości k wykres skrzypcowy (*violin plot*) dla uzyskanych średnich CHI w pięciu iteracjach na zdjęcie. Na podstawie wykresu zarekomenduj właściwą Twoim zdaniem wartość k dla danego algorytmu. Odpowiedź uzasadnij.
- Stosując wybraną wartość k, przygotuj wersje zdjęć po redukcji palety barw: każdy piksel zamień na kod z wybranego standardu barw środka danego skupienia dla właściwego algorytmu.
- Wybierz losowe 5 zdjęć. Dla każdego z tych zdjęć wyświetl oryginalne zdjęcie, efekt redukcji barw z wykorzystaniem algorytmu k-średnich oraz efekt redukcji z wykorzystaniem algorytmu k-median. Wybierz jeden z tych algorytmów i uzasadnij odpowiedź.

**Zadanie 4:** Zapisz zdjęcia o zredukowanej palecie barw w formacie .png (z domyślną bezstratną kompresją) dla wybranego algorytmu oraz k. Dla każdego zdjęcia oblicz procentowy zysk z kompresji wspomaganej redukcją palety barw, mierzony relatywną różnicą w rozmiarze zdjęcia w kilobajtach:
$$\frac{rozmiar \ oryginalny - rozmiar \ po \ kompresji}{rozmiar \ oryginalny} \cdot 100$$
Oszacuj modelem regresji liniowej zależność pomiędzy uzyskanym procentowym zyskiem a liczbą unikatowych barw na oryginalnym zdjęciu. Rozważ różne formy funkcyjne, postaraj się zmaksymalizować $R^2$.

**Zadanie 5:** Podejście trzeba jeszcze zareklamować. Osoby z zespołu analityków zdecydowały, że sprawdzą, czy zaproponowana metoda redukcji palety barw wpływa pozytywnie na wyniki zadania klasyfikacji binarnej, czy zdjęcie przedstawia kota. Niestety, znają tylko najbardziej podstawową technikę klasyfikacji (kNN).

- Przygotuj dane dla klasyfikacji: dla każdego zdjęcia przygotuj wartość $y$: czy przedstawia kota, czy nie (na podstawie nazwy pliku) oraz wektor opisujący barwy na kolejnych pikselach w wersji a) w oryginalnym zdjęciu, w wersji b) w "skompresowanym"zdjęciu. Połącz uzyskane wektory. Zwróć uwagę na wymiarowość danych: po tym kroku oczekiwane jest uzyskanie trzech zestawów danych: zbiór $Y$ o wymiarach 132 w x 1 k, zbiór $X_{orig}$ o wymiarach 132 w x 10 000 k oraz zbiór $X_{komp}$ o wymiarach 132 w x 10 000 k.
- Podziel bazę zdjęć na część treningową (90%) oraz testową (10%). Pamiętaj, żeby przeprowadzić te same podziały na bazach $X_{orig}$ oraz $X_{komp}$
- Wytrenuj technikę kNN osobno na bazach $X_{orig}$ oraz $X_{komp}$. Z wykorzystaniem 5-krotnej walidacji krzyżowej wybierz właściwą wartość hiperparametru. Wykorzystaj miarę F1.
- Zaraportuj uzyskane wartości miary F1 na zbiorze treningowym, walidacyjnym oraz testowym w podziale na klasyfikację na podstawie oryginalnych danych oraz danych po kompresji. Skomentuj, czy według Ciebie zastosowanie kompresji znacząco pogorszyło jakość dopasowania mierzoną miarą F1? Odpowiedź uzasadnij.
