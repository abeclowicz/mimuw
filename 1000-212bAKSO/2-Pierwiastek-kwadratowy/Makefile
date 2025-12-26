.PHONY: all example clean

all: example

example: nsqrt_example.c nsqrt_example.cpp nsqrt.o
	gcc -c -Wall -Wextra -std=c17 -O2 -o nsqrt_example_64.o nsqrt_example.c
	gcc -z noexecstack -o nsqrt_example_64 nsqrt_example_64.o nsqrt.o
	g++ -c -Wall -Wextra -std=c++20 -O2 -o nsqrt_example_cpp.o nsqrt_example.cpp
	g++ -z noexecstack -o nsqrt_example_cpp nsqrt_example_cpp.o nsqrt.o -lgmp

nsqrt.o: nsqrt.asm
	nasm -f elf64 -w+all -w+error -o nsqrt.o nsqrt.asm

clean:
	rm -f nsqrt.o nsqrt_example_64.o nsqrt_example_cpp.o
	rm -f nsqrt_example_64 nsqrt_example_cpp
