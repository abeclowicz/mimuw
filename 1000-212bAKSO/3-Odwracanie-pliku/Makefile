.PHONY: all example clean

all: freverse

freverse.o: freverse.asm
	nasm -f elf64 -w+all -w+error -w-unknown-warning -w-reloc-rel -o freverse.o freverse.asm

freverse: freverse.o
	ld --fatal-warnings -o freverse freverse.o

clean:
	rm -f freverse.o freverse
