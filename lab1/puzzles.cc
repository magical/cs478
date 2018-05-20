#include <cstring>
#include <string>
#include <sstream>

extern "C" {
#include "miracl.h"
}

#include "puzzles.hpp"

using std::string;

void shs256_process_string(sha256 *sh, string s) {
	for (char c : s) {
		shs256_process(sh, (int)(unsigned char)c);
	}
}


uint64_t get64(string &s, size_t off) {
	s.at(off+7); // bounds check
	return ((((uint64_t)(unsigned char)s[off+0])) |
		(((uint64_t)(unsigned char)s[off+1]) << 8) |
		(((uint64_t)(unsigned char)s[off+2]) << 16) |
		(((uint64_t)(unsigned char)s[off+3]) << 24) |
		(((uint64_t)(unsigned char)s[off+4]) << 32) |
		(((uint64_t)(unsigned char)s[off+5]) << 40) |
		(((uint64_t)(unsigned char)s[off+6]) << 48) |
		(((uint64_t)(unsigned char)s[off+7]) << 56));
}

void put64(string &s, uint64_t t) {
	s.at(7); // bounds check
	s[0] = (unsigned char)(t & 0xff);
	s[1] = (unsigned char)((t>>8) & 0xff);
	s[2] = (unsigned char)((t>>16) & 0xff);
	s[3] = (unsigned char)((t>>24) & 0xff);
	s[4] = (unsigned char)((t>>32) & 0xff);
	s[5] = (unsigned char)((t>>40) & 0xff);
	s[6] = (unsigned char)((t>>48) & 0xff);
	s[7] = (unsigned char)((t>>56) & 0xff);
}

// sets the first few bits of s to x
void setbits(string &s, uint64_t x, int bits) {
	uint64_t mask = (~0LLU) << bits;
	uint64_t y = get64(s);
	y = (y & mask) | (x & (~mask));
	put64(s, y);
}

string hash(string msg) {
	string output(32, '0');
	sha256 sh;
	shs256_init(&sh);
	shs256_process_string(&sh, msg);
	shs256_hash(&sh, &output[0]);
	return output;
}

string solve(Puzzle &puz) {
	// brute-force the puzzle
	if (puz.bits > 24) {
		// give up
		return "";
	}
	uint64_t upper = 1LLU<<puz.bits;
	string scratch = puz.puzzle;
	string output;
	for (uint64_t i = 0; i < upper; i++) {
		setbits(scratch, i, puz.bits);
		output = hash(scratch);
		if (output == puz.goal) {
			return scratch;
		}
	}
	return "";
}

Puzzle parse(string &msg) {
	Puzzle puz;
	if (msg.size() >= MESSAGE_LENGTH) {
		puz.puzzle = msg.substr(1,32);
		puz.goal = msg.substr(33,32);
		puz.t = get64(msg, 65);
		puz.bits = (int)get64(msg, 73);
	} else {
		puz.err = "invalid message length";
	}
	return puz;
}

void encode(Puzzle puz, void* p) {
	char* msg = reinterpret_cast<char*>(p);
	std::memmove(&msg[1], &puz.puzzle[0], puz.puzzle.size());
	std::memmove(&msg[33], &puz.goal[0], puz.goal.size());
	string x(8, '0');
	put64(x, puz.t);
	std::memmove(&msg[65], &x[0], x.size());
	put64(x, puz.bits);
	std::memmove(&msg[73], &x[0], x.size());
}

string hex(string s) {
	std::stringstream out;
	const char* hexdigits = "0123456789abcdef";
	for (char c : s) {
		out << hexdigits[(c >> 4) & 0xF];
		out << hexdigits[c & 0xF];
	}
	return out.str();
}
