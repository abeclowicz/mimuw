CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -Wpedantic

ifdef DEBUG
	CXXFLAGS += -O0 -g -DDEBUG
else
	CXXFLAGS += -O2
endif

.PHONY: all clean
.DEFAULT_GOAL := all

all: kayles_client kayles_server

clean:
	rm -f *.o kayles_client kayles_server

argparse.o: argparse.cpp argparse.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

messages.o: messages.cpp messages.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

sockets.o: sockets.cpp sockets.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

kayles_client.o: kayles_client.cpp argparse.h messages.h sockets.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

kayles_server.o: kayles_server.cpp argparse.h messages.h sockets.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

kayles_client: kayles_client.o argparse.o messages.o sockets.o
	$(CXX) $(CXXFLAGS) $^ -o $@

kayles_server: kayles_server.o argparse.o messages.o sockets.o
	$(CXX) $(CXXFLAGS) $^ -o $@
