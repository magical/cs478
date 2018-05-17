CXXFLAGS:=-Wall
CXXFLAGS+=-std=c++11 -I../include -ggdb -O1
LDLIBS+=-lzmq ../lib/miracl.a
all: client server
client server: crypto.o compress.o packet.o rabin.o big.o
crypto.o: crypto.cc crypto.hpp
packet.o: packet.cc packet.hpp
rabin.o: rabin.cc rabin.hpp packet.hpp

big_test: big.o
compress_test: compress.o crypto.o
