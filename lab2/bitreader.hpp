#pragma once
#include <string>

// big-endian bit reader
class BitReader {
	std::string data;
	size_t pos;
	uint64_t bits;
	int n;

public:
	BitReader(std::string data) : data(data), pos(0), bits(0), n(0) {}

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

