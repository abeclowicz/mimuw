CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
LDFLAGS = -shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup

.PHONY: clean example all

all: libma.so example

clean:
	rm -f *.o libma.so ma_example.e

example: ma_example.o libma.so
	gcc ma_example.o -o ma_example.e -L. -lma -Wl,-rpath,.

libma.so: ma.o memory_tests.o
	gcc $(LDFLAGS) -o $@ $^

ma.o: ma.c
	gcc $(CFLAGS) -c $< -o $@

memory_tests.o: memory_tests.c
	gcc $(CFLAGS) -c $< -o $@

ma_example.o: ma_example.c
	gcc $(CFLAGS) -c $< -o $@
