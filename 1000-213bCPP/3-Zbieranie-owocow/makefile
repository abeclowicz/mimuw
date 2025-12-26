.PHONY: example test all clean

example: fruit_picking.h fruit_picking_example.cpp
	g++ -Wall -Wextra -O2 -std=c++23 fruit_picking_example.cpp -o fruit_picking_example

test: example
	./fruit_picking_example > tmp.out
	diff tmp.out fruit_picking_example.out
	rm tmp.out

all: test

clean:
	rm fruit_picking_example
