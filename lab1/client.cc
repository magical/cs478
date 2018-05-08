#include <iostream>
#include <unistd.h>
#include <string>
#include "zmq.hpp"

extern "C" {
#include "miracl.h"
}

using std::string;

class Puzzle {
public:
	string puzzle;
	string goal;
	int bits;
	string err;
};

uint64_t get64(string &s) {
	s.at(7); // bounds check
	return (((uint64_t)(s[0])) |
		((uint64_t)(s[1]) << 8) |
		((uint64_t)(s[2]) << 16) |
		((uint64_t)(s[3]) << 24) |
		((uint64_t)(s[4]) << 32) |
		((uint64_t)(s[5]) << 40) |
		((uint64_t)(s[6]) << 48) |
		((uint64_t)(s[7]) << 56));
}

void put64(string &s, uint64_t t) {
	s.at(7); // bounds check
	s[0] = (char)(t & 0xff);
	s[1] = (char)((t>>8) & 0xff);
	s[2] = (char)((t>>16) & 0xff);
	s[3] = (char)((t>>24) & 0xff);
	s[4] = (char)((t>>32) & 0xff);
	s[5] = (char)((t>>40) & 0xff);
	s[6] = (char)((t>>48) & 0xff);
	s[7] = (char)((t>>56) & 0xff);
}

// sets the first few bits of s to x
void setbits(string &s, uint64_t x, int bits) {
	uint64_t mask = (~0LLU) << bits;
	uint64_t y = get64(s);
	y = (y & mask) | x;
	put64(s, y);
}

string hash(string msg) {
	string output(32, '0');
	sha256 sh;
	shs256_init(&sh);
	for (char c : msg) {
		shs_process(&sh, (int)(unsigned char)c);
	}
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

string ztostring(zmq::message_t &zmsg) {
	string s(zmsg.size(), '\0');
	memmove(&s[0], zmsg.data(), zmsg.size());
	return s;
}

Puzzle parse(string &msg) {
	Puzzle puz;
	// TODO: don't abort
	if (msg.size() == 1 + 32 + 32 + 8) {
		puz.puzzle = msg.substr(1,32);
		puz.goal = msg.substr(33,32);
		string bitstr = msg.substr(65,8);
		puz.bits = (int)get64(bitstr);
	} else {
		puz.err = "invalid message length";
	}
	return puz;
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

int main() {
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://127.0.0.1:7000");

	for (;;) {
		string s;
		std::getline(std::cin, s);
		if (!std::cin) {
			break;
		}

		// send a request to the server
		zmq::message_t msg(s.size());
		memmove(msg.data(), &s[0], s.size());
		socket.send(msg);

		zmq::message_t resp;
		socket.recv(&resp);

		if (resp.size() < 0) {
			std::cerr << "error: received empty reply\n";
			continue;
		}

		// first byte is 0 if the request was successful,
		// 1 if we need to solve a puzzle
		char first_byte = reinterpret_cast<char*>(resp.data())[0];
		if (first_byte == 0) {
			std::cout << "success\n";
			continue;
		}

		// TODO: solve puzzle
		std::cout << "got puzzle\n";
		string resp_str = ztostring(resp);
		Puzzle puz = parse(resp_str);
		if (!puz.err.empty()) {
			std::cout << puz.err << "\n";
			continue;
		}

		std::cout << hex(puz.puzzle) << "\n";
		std::cout << hex(puz.goal) << "\n";
		std::cout << puz.bits << "\n";
	}

	return 0;
}
