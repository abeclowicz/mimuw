CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lcrypto -lssl

.PHONY: all clean
.DEFAULT_GOAL := all

all: sikradio

clean:
	rm -f *.o sikradio

argparse.o: argparse.cpp argparse.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

connection.o: connection.cpp connection.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

http.o: http.cpp http.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

urlparse.o: urlparse.cpp urlparse.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

sikradio.o: sikradio.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

sikradio: sikradio.o argparse.o connection.o http.o urlparse.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
