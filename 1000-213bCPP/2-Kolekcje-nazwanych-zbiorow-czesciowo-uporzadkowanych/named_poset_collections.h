#ifndef NAMED_POSET_COLLECTIONS_H
#define NAMED_POSET_COLLECTIONS_H

#ifndef __cplusplus
    #include <stdbool.h>
    #include <stddef.h>
#else
    #include <cstddef>
#endif

#ifdef __cplusplus
    namespace cxx {
        extern "C" {
#endif

/**
 * Tworzy nową, pustą kolekcję nazwanych zbiorów częściowo uporządkowanych.
 *
 * @returns Wynikiem jest identyfikator tej kolekcji. Każde wywołanie zwraca
 * nowy identyfikator, począwszy od <code>0</code>, a skończywszy na
 * <code>LONG_MAX</code>. Jeśli wyczerpią się identyfikatory, zwraca
 * <code>-1</code>.
 */
long npc_new_collection(void);

/**
 * Jeżeli istnieje kolekcja nazwanych zbiorów częściowo uporządkowanych o
 * identyfikatorze <code>id</code>, usuwa ją, a w przeciwnym przypadku niczego
 * nie robi.
 */
void npc_delete_collection(long id);

/**
 * Jeśli istnieje kolekcja o identyfikatorze <code>id</code>, a
 * <code>name</code> jest poprawną nazwą i nie ma w tej kolekcji zbioru
 * częściowo uporządkowanego o nazwie <code>name</code>, tworzy w tej kolekcji
 * nowy zbiór częściowo uporządkowany o podanej nazwie i relacji zawierającej
 * jedynie pary <code>(x, x)</code> dla <code>x = 0, 1, …, N - 1</code>.
 *
 * @returns Wynikiem jest <code>true</code>, jeśli zbiór częściowo uporządkowany
 * został utworzony, a <code>false</code> w przeciwnym przypadku.
 */
bool npc_new_poset(long id, char const *name);

/**
 * Jeśli istnieje kolekcja o identyfikatorze <code>id</code> i jest w niej zbiór
 * częściowo uporządkowany o nazwie <code>name</code>, usuwa go, a w przeciwnym
 * przypadku niczego nie robi.
 */
void npc_delete_poset(long id, char const *name);

/**
 * Jeśli istnieje kolekcja o identyfikatorze <code>id</code>, a
 * <code>name_dst</code> jest poprawną nazwą i jest w tej kolekcji zbiór
 * częściowo uporządkowany o nazwie <code>name_src</code>, kopiuje go na zbiór
 * częściowo uporządkowany o nazwie <code>name_dst</code>.
 *
 * @returns Wynikiem jest <code>true</code>, jeśli zbiór został skopiowany, a
 * <code>false</code> w przeciwnym przypadku.
 */
bool npc_copy_poset(long id, char const *name_dst, char const *name_src);

/**
 * @returns Jeśli istnieje niepusta kolekcja o identyfikatorze <code>id</code>,
 * wynikiem jest wskaźnik na nazwę pierwszego zbioru częściowo uporządkowanego w
 * tej kolekcji, a <code>NULL</code> w przeciwnym przypadku.
 */
char const *npc_first_poset(long id);

/**
 * @returns Jeśli istnieje kolekcja o identyfikatorze <code>id</code>, a w niej
 * istnieje zbiór częściowo uporządkowany następny w kolejności po zbiorze
 * częściowo uporządkowanym o nazwie <code>name</code>, wynikiem jest wskaźnik
 * na nazwę tego następnego zbioru częściowo uporządkowanego, a
 * <code>NULL</code> w przeciwnym przypadku.
 */
char const *npc_next_poset(long id, char const *name);

/**
 * Jeśli istnieje kolekcja o identyfikatorze <code>id</code>, a w niej istnieje
 * zbiór częściowo uporządkowany o nazwie <code>name</code>, elementy
 * <code>x</code> i <code>y</code> należą do tego zbioru, ale nie są w relacji,
 * dodaje parę <code>(x, y)</code> do relacji i domyka relację przechodnio.
 *
 * @returns Wynikiem jest <code>true</code>, jeśli relacja została
 * zmodyfikowana, a <code>false</code> w przeciwnym przypadku.
 */
bool npc_add_relation(long id, char const *name, size_t x, size_t y);

/**
 * @returns Jeśli istnieje kolekcja o identyfikatorze <code>id</code>, a w niej
 * istnieje zbiór częściowo uporządkowany o nazwie <code>name</code> oraz para
 * <code>(x, y)</code> należy do relacji tego zbioru, wynikiem jest
 * <code>true</code>, a <code>false</code> w przeciwnym przypadku.
 */
bool npc_is_relation(long id, char const *name, size_t x, size_t y);

/**
 * Jeśli istnieje kolekcja o identyfikatorze <code>id</code>, a w niej istnieje
 * zbiór częściowo uporządkowany o nazwie <code>name</code> oraz para
 * <code>(x, y)</code> różnych elementów należy do relacji tego zbioru i nie
 * istnieje element <code>z</code> różny od <code>x</code> i <code>y</code>,
 * taki że pary <code>(x, z)</code> i <code>(z, y)</code> należą do relacji,
 * usuwa parę <code>(x, y)</code> z relacji.
 *
 * @returns Wynikiem jest <code>true</code>, jeśli relacja została
 * zmodyfikowana, a <code>false</code> w przeciwnym przypadku.
 */
bool npc_remove_relation(long id, char const *name, size_t x, size_t y);

/**
 * @returns Wynikiem jest liczba aktualnie istniejących kolekcji.
 */
size_t npc_size(void);

/**
 * @returns Wynikiem jest liczba elementów zbioru częściowo uporządkowanego.
 */
size_t npc_poset_size(void);

/**
 * @returns Jeśli istnieje kolekcja o identyfikatorze <code>id</code>, wynikiem
 * jest liczba zbiorów częściowo uporządkowanych w tej kolekcji, a
 * <code>0</code> w przeciwnym przypadku.
 */
size_t npc_collection_size(long id);

#ifdef __cplusplus
        } /* extern "C" */
    } /* namespace cxx */
#endif

#endif /* NAMED_POSET_COLLECTIONS_H */
