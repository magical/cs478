#include <string>
#include <vector>
#include "crypto.hpp"
#include "bitreader.hpp"

using std::string;
using std::vector;

void hors_keygen(unsigned keysize, int t) {
	vector<string> xs;
	vector<string> ys;
	for (int i = 0; i < t; i++) {
		string x = random_bytes(keysize/8);
		string y = hash(x);
		xs.push_back(x);
		ys.push_back(y);
	}
}

vector<string> hors_sign(const string msg, const vector<string> &xs) {
	vector<string> sig;
	auto h = hash(msg);
	BitReader br(h);
	const int l = 10; // FIXME: log2 t
	for (int i = 0; i < l; i++) {
		int k = br.readbits(l);
		sig.push_back(xs.at(k));
	}
	return sig;
}

bool hors_verify(const string msg, const vector<string> sig, const vector<string> &ys) {
	const int l = 10; // FIXME: log2 t
	auto h = hash(msg);
	BitReader br(h);
	unsigned valid = 1;
	for (int i = 0; i < l; i++) {
		auto y = hash(sig[i]);
		int k = br.readbits(l);
		valid &= (y == ys[k]);
	}
	return valid;
}
