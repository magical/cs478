CXXFLAGS:=-Wall
CXXFLAGS+=-std=c++11 -I../include -ggdb -O1
LDLIBS+=-lzmq ../lib/miracl.a
all: main
main: main.cc merkle.o hors.o pq.o crypto.o
crypto.o: crypto.cc crypto.hpp
merkle.o: merkle.cc merkle.hpp bitreader.hpp
hors.o: hors.cc hors.hpp
pq.o: pq.cc pq.hpp merkle.hpp bitreader.hpp
