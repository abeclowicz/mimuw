.PHONY: all clean test test_valgrind

named_poset_collections_32.o: named_poset_collections.cpp
	g++ -c -Wall -Wextra -O2 -std=c++23 $^ -o $@

named_poset_collections_64.o: named_poset_collections.cpp
	g++ -c -Wall -Wextra -O2 -std=c++23 -DN=64 $^ -o $@

named_poset_collections_example_1.o: named_poset_collections_example_1.c
	gcc -c -Wall -Wextra -O2 -std=c23 -I. $^ -o $@

named_poset_collections_example_2.o: named_poset_collections_example_2.cpp
	g++ -c -Wall -Wextra -O2 -std=c++23 -I. $^ -o $@

named_poset_collections_example_1: named_poset_collections_example_1.o named_poset_collections_32.o
	g++ $^ -o $@

named_poset_collections_example_2_a: named_poset_collections_example_2.o named_poset_collections_64.o
	g++ $^ -o $@

named_poset_collections_example_2_b: named_poset_collections_64.o named_poset_collections_example_2.o
	g++ $^ -o $@

all: named_poset_collections_example_1 named_poset_collections_example_2_a named_poset_collections_example_2_b

clean:
	rm -f *.o named_poset_collections_example_1 named_poset_collections_example_2_a named_poset_collections_example_2_b

test: all
	./named_poset_collections_example_1
	./named_poset_collections_example_2_a
	./named_poset_collections_example_2_b

test_valgrind: all
	valgrind --tool=memcheck --leak-check=full ./named_poset_collections_example_1
	valgrind --tool=memcheck --leak-check=full ./named_poset_collections_example_2_a
	valgrind --tool=memcheck --leak-check=full ./named_poset_collections_example_2_b
