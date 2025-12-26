# Odwracanie pliku

Zaimplementuj w asemblerze program `freverse`, który odwraca zawartość pliku. Program uruchamia się poleceniem

```bash
./freverse file
```

gdzie parametr `file` to nazwa odwracanego pliku. Jeśli plik ten jest krótszy niż dwa bajty, to jego odwracanie nie zmienia jego zawartości. Program nie powinien mieć zaszytego ograniczenia na rozmiar pliku, w szczególności program powinien działać na plikach o rozmiarze powyżej 4 GiB. Należy zadbać o efektywne działanie programu.

Program powinien korzystać z funkcji systemowych Linuksa: `sys_read`, `sys_write`, `sys_open`, `sys_close`, `sys_stat`, `sys_fstat`, `sys_lseek`, `sys_mmap`, `sys_munmap`, `sys_msync`, `sys_exit`, i tylko z tych funkcji, ale nie musi oczywiście korzystać ze wszystkich tu wymienionych.

Program powinien sprawdzać poprawność wywołania i poprawność wykonania funkcji systemowych (z wyjątkiem `sys_exit`). Jeśli nie podano parametru programu, podano więcej niż jeden parametr, podany parametr jest niepoprawny lub wywołanie funkcji systemowej zakończyło się błędem, to program powinien zakończyć się kodem `1`. W każdej sytuacji program powinien przed zakończeniem jawnie wywołać funkcję `sys_close` dla pliku, który otworzył.

Program nie powinien niczego wypisywać na terminal.

## Oddawanie rozwiązania

Jako rozwiązanie należy wstawić w Moodle plik o nazwie `freverse.asm`.

## Kompilowanie

Rozwiązanie będzie kompilowane poleceniami:

```bash
nasm -f elf64 -w+all -w+error -w-unknown-warning -w-reloc-rel -o freverse.o freverse.asm
ld --fatal-warnings -o freverse freverse.o
```

Rozwiązanie musi się kompilować i działać na maszynie students i w laboratorium komputerowym.

## Ocenianie

Ocena składa się z dwóch części.

1. Zgodność rozwiązania ze specyfikacją będzie oceniania za pomocą testów automatycznych, za które dostaje się maksymalnie 7 punktów. Przede wszystkim będzie oceniana poprawność wyniku. W tym zadaniu priorytetem jest szybkość działania programu, ale oceniane będą też rozmiary sekcji i wykorzystanie pamięci, w tym stosu. Za błędną nazwę pliku źródłowego odejmiemy jeden punkt.

2. Za formatowanie i jakość kodu dostaje się maksymalnie 3 punkty. Tradycyjne formatowanie programów w asemblerze polega na rozpoczynaniu etykiet od pierwszej kolumny, a mnemoników rozkazów, parametrów rozkazów i komentarzy od wybranych ustalonych kolumn (np. 3, 11 i 29). Nie stosuje się innych wcięć. Taki format mają przykłady pokazywane na zajęciach. Kod powinien być dobrze skomentowany, co oznacza między innymi, że każdy blok kodu powinien być opatrzony informacją, co robi. Należy opisać przeznaczenie rejestrów. Komentarza wymagają wszystkie kluczowe lub nietrywialne linie kodu. W przypadku asemblera nie jest przesadą komentowanie prawie każdej linii kodu, ale należy unikać komentarzy opisujących to, co widać.

Zastrzegamy sobie uzależnienie wystawienia oceny od osobistego wyjaśnienia szczegółów działania programu prowadzącemu zajęcia.
