#include <iostream>
#include <string>
#include "zmq.hpp"

extern "C" {
#include "miracl.h"
}

#include "puzzles.hpp"

using std::string;

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

Puzzle genpuzzle(string secret, time_t t, string msg) {
	Puzzle puz;

	// generate puzzle
	string output(32, '0');
	sha256 sh;
	shs256_init(&sh);
	shs256_process_string(&sh, secret);
	shs256_process_string(&sh, tostring(t));
	shs256_process_string(&sh, msg);
	shs256_hash(&sh, &output[0]);
	puz.puzzle = output;

	// generate goal
	puz.goal = hash(output);

	puz.bits = 4;

	return puz;
}

string check(string secret, time_t t, string msg, string puzzle) {
	string output(32, '0');
	sha256 sh;
	shs256_init(&sh);
	shs256_process_string(&sh, secret);
	shs256_process_string(&sh, tostring(t));
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

		// read message
		auto s = std::string(reinterpret_cast<char*>(msg.data()), msg.size());
		std::cout << s << "\n";

		// does it contain a response to a puzzle?
		// if so, check
		// otherwise, generate a new puzzle
		if (s.at(0) == 1) {
			// 1 | solution | goal | nbits | message

		} else {
			// generate puzzle
			time_t t = time(NULL);
			Puzzle puz = genpuzzle(secret, t, ztostring(msg));
			// clear low bits
			puz.puzzle[0] = 0;

			// 1 | puzzle | goal | nbits
			zmq::message_t resp(1+32+32+8);

			reinterpret_cast<char*>(resp.data())[0] = 1;
			memmove(resp.data()+1, &puz.puzzle[0], puz.puzzle.size());
			memmove(resp.data()+33, &puz.goal[0], puz.goal.size());
			string x(8, '0');
			put64(x, puz.bits);
			memmove(resp.data()+65, &x[0], x.size());

			socket.send(resp);
		}
	}

	return 0;
}
