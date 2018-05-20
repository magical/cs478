#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include <sys/auxv.h>

#include "crypto.hpp"

extern "C" {
#include "miracl.h"
}

using std::string;

static void shs256_process_string(sha256 *sh, const string s) {
	for (char c : s) {
		shs256_process(sh, (int)(unsigned char)c);
	}
}

uint64_t get64(const string &s, size_t off) {
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

string str64(uint64_t t) {
	string s(8,'\0');
	put64(s, t);
	return s;
}

string hash(const string msg) {
	string output(32, '0');
	sha256 sh;
	shs256_init(&sh);
	shs256_process_string(&sh, msg);
	shs256_hash(&sh, &output[0]);
	return output;
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

string hmac(const string &key, const string msg)
{
	const int IPAD = 0x36;
	const int OPAD = 0x5c;
	string tag;
	sha256 outer;
	sha256 inner;
	char x[256/8];
	char K[512/8];

	if (key.size() > sizeof K) {
		throw new std::logic_error("key too large");
	}

	memset(K, 0, sizeof K);
	memmove(K, &key[0], key.size());

	shs256_init(&inner);
	shs256_init(&outer);
	for (char c : K) {
		shs256_process(&inner, c ^ IPAD);
		shs256_process(&outer, c ^ OPAD);
	}

	for (char c : msg) {
		shs256_process(&inner, c);
	}
	shs256_hash(&inner, x);

	for (char c : x) {
		shs256_process(&outer, c);
	}
	shs256_hash(&outer, x);

	return std::string(x, sizeof x);
}

static csprng rng;
int initialized;

void init_rng() {
	unsigned long r = getauxval(AT_RANDOM);
	if (r == 0) {
		throw std::runtime_error("couldn't init rng");
	}
	strong_init(&rng, 16, (char*)r, 0);
	initialized = 1;
}

string random_bytes(int n) {
	if (!initialized) {
		throw std::runtime_error("rng not initialized");
	}
	string x(n, '\0');
	for (int i = 0; i < n; i++) {
		x[i] = strong_rng(&rng);
	}
	return x;
}
