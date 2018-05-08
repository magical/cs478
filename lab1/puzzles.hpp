#pragma once

#include <string>

class Puzzle {
public:
	std::string puzzle;
	std::string goal;
	uint64_t t;
	int bits;
	std::string err;
};

const int MESSAGE_LENGTH = 1 + 32 + 32 + 8 + 8;

void shs256_process_string(sha256 *sh, std::string s);
uint64_t get64(std::string &s, size_t off = 0);
void put64(std::string &s, uint64_t t);
void setbits(std::string &s, uint64_t x, int bits);
std::string hash(std::string msg);
std::string solve(Puzzle &puz);
Puzzle parse(std::string &msg);
void encode(Puzzle puz, void* p);
std::string hex(std::string s);
