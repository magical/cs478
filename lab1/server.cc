#include <iostream>
#include <string>
#include "zmq.hpp"

extern "C" {
#include "miracl.h"
}

#include "puzzles.hpp"

using std::string;

const int NBITS = 8;
zmq::message_t empty;

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

	// clear low bits
	setbits(puz.puzzle, 0, NBITS);

	puz.t = t;
	puz.bits = NBITS;

	return puz;
}

bool valid(string secret, time_t t, string msg, string puzzle) {
	string output(32, '0');
	sha256 sh;
	shs256_init(&sh);
	shs256_process_string(&sh, secret);
	shs256_process_string(&sh, tostring(t));
	shs256_process_string(&sh, msg);
	shs256_hash(&sh, &output[0]);

	shs256_init(&sh);
	shs256_process_string(&sh, output);
	shs256_hash(&sh, &output[0]);

	string puzhash = hash(puzzle);
	return output == puzhash;
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
		zmq::message_t req;
		socket.recv(&req);

		// read message
		auto s = std::string(reinterpret_cast<char*>(req.data()), req.size());
		//std::cout << s << "\n";

		// does it contain a response to a puzzle?
		// if so, check
		// otherwise, generate a new puzzle
		if (s.at(0) == 2) {
			// 2 | solution | goal | t | nbits | message
			Puzzle puz = parse(s);
			if (!puz.err.empty()) {
				// invalid message size
				goto error;
			}
			string message = s.substr(MESSAGE_LENGTH);
			if (valid(secret, puz.t, message, puz.puzzle)) {
				std::cout << message << "\n";
			} else {
				std::cout << "invalid\n";
			}
			socket.send(empty);
		} else if (s.at(0) == 0) {
			// 0 | message

			// generate puzzle
			time_t t = time(NULL);
			string message = s.substr(1);
			Puzzle puz = genpuzzle(secret, t, message);

			// 1 | puzzle | goal | t | nbits
			zmq::message_t resp(MESSAGE_LENGTH);

			reinterpret_cast<char*>(resp.data())[0] = 1;
			encode(puz, resp.data());

			socket.send(resp);
		} else {
			goto error;
		}
		continue;
	error:
		std::cerr << "error\n";
		socket.send(empty);
	}

	return 0;
}
