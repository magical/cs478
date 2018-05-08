#include <iostream>
#include <string>
#include "zmq.hpp"

extern "C" {
#include "miracl.h"
}

using std::string;

void shs256_process_string(sha256 *sh, string s) {
	for (char c : s) {
		shs_process(sh, (int)(unsigned char)c);
	}
}

std::string tostring(uint64_t t) {
	std::string s(8, '0');
	s[0] = (char)(t & 0xff);
	s[1] = (char)((t>>8) & 0xff);
	s[2] = (char)((t>>16) & 0xff);
	s[3] = (char)((t>>24) & 0xff);
	s[4] = (char)((t>>32) & 0xff);
	s[5] = (char)((t>>40) & 0xff);
	s[6] = (char)((t>>48) & 0xff);
	s[7] = (char)((t>>56) & 0xff);
	return s;
}

string genpuzzle(string secret, time_t t, string msg) {
	string output(32, '0');
	sha256 sh;
	shs256_init(&sh);
	shs256_process_string(&sh, secret);
	shs256_process_string(&sh, tostring(t));
	shs256_process_string(&sh, msg);
	shs256_hash(&sh, &output[0]);
	return output;
}

string hash(string msg) {
	string output(32, '0');
	sha256 sh;
	shs256_init(&sh);
	shs256_process_string(&sh, msg);
	shs256_hash(&sh, &output[0]);
	return output;
}

string ztostring(zmq::message_t &zmsg) {
	string s(zmsg.size(), '\0');
	memmove(&s[0], zmsg.data(), zmsg.size());
	return s;
}


int main() {
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::rep);
	socket.bind("tcp://127.0.0.1:7000");

	string secret = "client secret";

	for (;;) {
		zmq::message_t msg;
		socket.recv(&msg);

		// generate puzzle
		time_t t = time(NULL);
		string puzzle = genpuzzle(secret, t, ztostring(msg));
		string puzzle_hash = hash(puzzle);

		auto s = std::string(reinterpret_cast<char*>(msg.data()), msg.size());
		std::cout << s << "\n";

		zmq::message_t empty;
		socket.send(empty);
	}

	return 0;
}
