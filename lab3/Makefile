CXXFLAGS:=-Wall
CXXFLAGS+=-std=c++11 -I../include -ggdb -O1
LDFLAGS+=-flto
LDLIBS+=-lzmq ../lib/miracl.a
all: client server
client server: crypto.o compress.o
crypto.o: crypto.cc crypto.hpp
