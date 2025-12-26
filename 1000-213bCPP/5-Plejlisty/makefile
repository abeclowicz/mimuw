.PHONY: all clean test

all: playlist.h
	g++ -Wall -Wextra -O2 -std=c++23 *.cpp

clean:
	rm -f *.out

test: all
	valgrind --error-exitcode=123 --leak-check=full --show-leak-kinds=all \
	--errors-for-leak-kinds=all --run-cxx-freeres=yes -q ./a.out 2> a.log
	diff -w playlist_example.log a.log
	rm -f a.log
