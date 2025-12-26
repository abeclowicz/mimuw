Zadanie w tym bloku stawia na Państwa kreatywność i nie posiada sztywnej listy kroków do odtworzenia. [...] Zasadnicze wymagania techniczne i funkcjonalne projektu są następujące:

- **Gra w TypeScript:** Należy stworzyć jednoosobową grę przeglądarkową (np. logiczną lub zręcznościową). Wszelkie autorskie pomysły są mile widziane. W przypadku braku własnej koncepcji, opcją domyślną jest implementacja Sudoku na czas.

- **Mechanika gry:** Rozgrywka musi opierać się na zdobywaniu punktów i składać się z wielu różnych poziomów (może ich być skończona liczba, ale jakieś replayability musi być zapewnione).

- **Aplikacja webowa:** Grę należy osadzić w aplikacji webowej, która musi zapewniać co najmniej obsługę kont użytkowników (rejestracja, logowanie) oraz interfejs do samej rozgrywki.

- **Server-Sent Events (SSE):** Serwer musi na bieżąco przekazywać do klienta informacje o nowych rekordach (Top 5) z wykorzystaniem technologii SSE. Użytkownik w przeglądarce powinien stale widzieć aktualną tabelę najlepszych wyników dla poziomu, na którym obecnie gra.
