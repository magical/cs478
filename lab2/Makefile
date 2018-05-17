CXXFLAGS:=-Wall
CXXFLAGS+=-std=c++11 -I../include -ggdb -O1
LDLIBS+=-lzmq ../lib/miracl.a
all: merkle.o
merkle.o: merkle.cc merkle.hpp
