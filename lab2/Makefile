CXXFLAGS:=-Wall
CXXFLAGS+=-std=c++11 -I../include -ggdb -O1
LDLIBS+=-lzmq ../lib/miracl.a
all: merkle.o hors.o
merkle.o: merkle.cc merkle.hpp
hors.o: hors.cc
