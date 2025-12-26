# Fabryka

Fabryka zatrudnia pracowników i przyjmuje zlecenia przetwarzające liczby. Wykonanie zlecenia może zostać zaplanowane na przyszłość.

### Zlecenie

Zlecenie jest reprezentowane przez strukturę

```c
typedef struct task_t {  
    int id; time_t start;  
    int (*task_function) (int);  
    int capacity;  
    int* data;  
    int* results;  
} task_t;
```

gdzie `id` to unikalny kod zadania, `capacity` to liczba pracowników, która ma obsłużyć zadanie, `task_function` to funkcja opisująca zadanie, `data` to tablica o rozmiarze capacity przechowująca dane dla pracowników, `result` to tablica do przechowania wyników wywołania `task_function` przez pracowników.  

### Fabryka

Fabryka ma `S` stanowisk. Każde stanowisko (`0`, …, `S-1`) ma pojemność `s_i > 0`, która wyraża ilu maksymalnie pracowników może pracować przy stanowisku. Każde zadanie `task` jest wykonywane nie wcześniej niż `task.start`, na jednym stanowisku, mającym miejsce dla nie mniej niż `task.capacity` pracowników, tj. `capacity <= s_i`, przez dokładnie `task.capacity` pracowników. Fabryka jest inicjowana poleceniem `int init_plant(int* stations, int n_stations, int n_workers)`, które przekazuje tablicę stanowisk wraz z pojemnościami oraz liczbę pracowników, którzy zgłoszą się do pracy. Fabryka jest sprzątana poleceniem `int destroy_plant()`. Funkcja wywołana przed zakończeniem pracy powinna poczekać na zakończenie działania fabryki. Wszelkie zlecenia otrzymane po wywołaniu `destroy_plant()` powinny być ignorowane i zwracać `-1`.

### Pracownik

W fabryce pracuje nie więcej niż `P` pracowników. Pracownik jest reprezentowany przez strukturę

```c
typedef struct worker_t {  
    int id;  
    time_t start;  
    time_t end;  
    int (*work) (worker_t* w, task_t* t, int i);  
} worker_t;  
```

gdzie `id` to unikalny identyfikator pracownika, `start` i `end` to, odpowiednio, początek i koniec pracy, `work` to funkcja przetwarzająca zlecenia. Pierwszy argument funkcji `work`, `worker_t* w`, to samoodwołanie. Każdy pracownik `w` zgłasza się do pracy raz, instrukcją `int add_worker(worker_t*)`, i może zostać przydzielany do (rozpoczęcia) zadań tylko w czasie pomiędzy `w.start` i `w.end`. Zadania do których zostanie przydzielony przed `w.end` mogą zostać dokończone. W danym momencie, pracownik może pracować tylko przy jednym zadaniu, i nie może zostać przydzielony do zadania, dopóki nie zakończy **swojej** pracy nad poprzednim zadaniem. Przy czym samo zadanie nie musi być zakończone przed podjęciem nowego zadania. Fabryka wie ilu pracowników `P` zgłosi się do pracy.

## Zlecanie zadań

Zadanie zostaje zlecone przez `int add_task(task_t*)`, a wynik może zostać odebrany przez `int collect_task(task_t*)`. Wywołanie `collect_task(&task)` powinno wstrzymywać wątek do czasu obliczenia zadania i zwrócić `0` w razie powodzenia oraz `-1` w przypadku błędu. Wywołanie `add_task(&task)` powinno zwrócić `0` w przypadku powodzenia oraz `-1` w przypadku błędu. Zadania mogą pozostać nieodebrane.

## Wykonywanie zadań

Zadanie `task` może być wykonane tylko gdy mamy dostępnych `task.capacity` pracowników, **wolne** stanowisko o pojemności większej bądź równej `task.capacity` i czas jest nie wcześniejszy niż `task.start`. Stanowisko i wybrani pracownicy są uznani za zajętych (nie są **wolni**) i nie mogą zostać wybrani do innych zadań do czasu uzyskania statusu **wolny**. Po przydzieleniu `task.capacity` pracowników, każdy z pracowników `w_i`, `0 ≤ i < capacity`, wykonuje `w_i.work(w_i, &task, i)` i umieszcza wynik w odpowiednim polu `task.result[i]`, po czym staje się **wolny**. Kiedy ostatni pracownik umieści wynik, stanowisko staje się **wolne**.
Zadania, które nie będą mogły być wykonane ze względu na brak pracowników powinny natychmiast zakończyć się z błędem (`-1`) w momencie, kiedy system jest w stanie rozpoznać ten stan.

## Polecenie

W katalogu `plant/` należy zaimplementować wyżej wymienione funkcje (`init_plant`, `destroy_plant`, `add_worker`, `add_task`, `collect_task`) niezbędne do operacji fabryki, która przyjmuje zlecenia w środowisku wielowątkowym, tj. zlecenia mogą być kierowane współbieżnie przez wiele wątków i powinny być obliczane wydajnie i współbieżnie.

## Założenia

Można założyć, że:

- `S > 0`, `P ≥ 0`;
- wszyscy `P` zadeklarowani pracownicy zgłoszą się do pracy;
- każdy pracownik zgłosi się dokładnie raz (kolejne zgłoszenia z wcześniej użytym `id` należy ignorować);
- każde zlecenie zostanie wywołane (`add_task`) dokładnie raz (kolejne zgłoszenia z wcześniej użytym `id` należy ignorować);
- zgłoszone zadania `work` i `task` wykonają się poprawnie i nie zakończą programu;
- dla każdego zgłoszonego pracownika, struktura `worker_t` nie zostanie zmodyfikowana ani zwolniona co najmniej do czasu wywołania `destroy_plant`; odpowiedzialny za zwolnienie tej struktury jest wołający `add_worker` (tzn. testy automatyczne).
- dla każdego zgłoszonego zadania, struktura `task_t` nie zostanie zmodyfikowana ani zwolniona co najmniej do czasu wywołania `destroy_plant`; odpowiedzialny za zwolnienie tej struktury jest wołający `add_task` (tzn. testy automatyczne).
- istnieje jedna globalna fabryka; po `destroy_plant` powinny być zwolnione wszystkie związane z nią zasoby, poza ewentualnie pamięcią stałego rozmiaru (niezależną od `n_stations=S` i `n_workers=P` i liczby zgłoszonych zadań);
- funkcje `add_worker`, `add_task`, `collect_task` będą wołane pomiędzy `init_plant` i `destroy_plant`, te wywołane poza tym przedziałem powinny zostać zignorowane i zwrócić `-1`; W rozwiązaniu można wykonywać wielomianowe obliczenia, np. liniowe przeszukiwanie w tablicy pracowników.

## Szczegóły

- Rozwiązaniem powinno być archiwum postaci takiej jak załączony szablon `ab12345.zip`, czyli archiwum ZIP o nazwie `ab12345.zip` lub archiwum `tar gzip` o nazwie `ab12345.tgz`; zawierający wyłącznie folder o nazwie `ab12345`, gdzie zamiast `ab12345` należy użyć własnych inicjałów i numeru indeksu (czyli nazwy użytkownika na students).
- Rozwiązanie współbieżne powinno znajdować się w podfolderze `plant/` i definiować funkcje zadeklarowane w `common/plant.h`.
- Można w rozwiązaniu załączyć dodatkowe pliki i foldery, o ile będzie się dało skompilować i wykonać następująco na maszynie students:

```bash
bash unzip ab12345.zip
# lub:
tar --gunzip -xf ab12345.tgz
# pliki ab12345/CMakeLists.txt oraz foldery ab12345/common/ i ab12345/demo/
# zostaną skopiowane z oryginalnego archiwum, nadpisując wszelkie zmiany.
cmake -S ab12345/ -B build/ -DCMAKE_BUILD_TYPE=Release
cd build/
make ./demo/demo 
```

W szczególności:

- Zmiany `/ab12345/CMakeLists.txt` zostaną cofnięte, ale można zmieniać `CMakeLists.txt` w podkatalogu `plant/`.
- Do folderu `common/` i `demo/` nie należy dodawać własnych plików (zostaną usunięte).
- Foldery inne niż `/ab12345/plant/` będą zignorowane przez `/ab12345/CMakeLists.txt`.
- Można zmieniać `.clang-tidy` i `.clang-format` (użycie nie jest wymagane, jedynie zalecane).
- Nie należy zmieniać flag, opcji, itp. kompilatora, linkera i CMake. Własne pliki `CMakeLists.txt` powinny się ograniczać do `add_executable`, `add_library`, `target_link_libraries`.
