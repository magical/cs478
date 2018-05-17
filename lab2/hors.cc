#include <string>
#include <vector>

using std::string;
using std::vector;

extern string random_bytes(size_t);
extern string hash(string);

// big-endian bit reader
class BitReader {
	string data;
	size_t pos;
	uint64_t bits;
	int n;

public:
	BitReader(string data) : data(data), pos(0), bits(0), n(0) {}

	uint64_t readbits(int howmany) {
		uint64_t output;
		while (n < howmany) {
			bits <<= 8;
			if (pos < data.size()) {
				bits += (uint64_t)(unsigned char)data[pos];
				pos++;
			}
		}
		output = bits >> (n - howmany);
		n -= howmany;
		return output;
	}
};


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
