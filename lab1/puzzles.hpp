#pragma once

#include <string>

class Puzzle {
public:
	std::string puzzle;
	std::string goal;
	int bits;
	std::string err;
};
void shs256_process_string(sha256 *sh, std::string s);
uint64_t get64(std::string &s);
void put64(std::string &s, uint64_t t);
void setbits(std::string &s, uint64_t x, int bits);
std::string hash(std::string msg);
std::string solve(Puzzle &puz);
Puzzle parse(std::string &msg);
std::string hex(std::string s);
