CXXFLAGS:=-Wall
CXXFLAGS+=-std=c++11 -I../include
LDFLAGS+=-lzmq
LDLIBS+=../lib/miracl.a
all: client server
client server: puzzles.o
