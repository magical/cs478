#include <iostream>
#include <unistd.h>
#include <string>
#include "zmq.hpp"

extern "C" {
#include "miracl.h"
}

#include "puzzles.hpp"
#include "clock.hpp"

using std::string;

string ztostring(zmq::message_t &zmsg) {
	string s(zmsg.size(), '\0');
	memmove(&s[0], zmsg.data(), zmsg.size());
	return s;
}

int main() {
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://127.0.0.1:7000");

	int nrequests = 0;

	auto t0 = Clock::now();
	for (;;) {
		string line;
		std::getline(std::cin, line);
		if (!std::cin) {
			break;
		}

		// send a request to the server
		zmq::message_t msg(line.size()+1);
		((char*)msg.data())[0] = 0;
		memmove((char*)msg.data()+1, &line[0], line.size());
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

		auto t2 = Clock::now();
		string solution = solve(puz);
		auto t3 = Clock::now();
		std::cout << hex(solve(puz)) << "\n";
		std::cout << "solving took "<<deltstr(t2, t3)<<"\n";

		puz.puzzle = solution;

		msg.rebuild(MESSAGE_LENGTH + line.size());
		((char*)msg.data())[0] = 2;
		encode(puz, msg.data());
		memmove((char*)msg.data()+MESSAGE_LENGTH, &line[0], line.size());

		string debug((char*)msg.data(), msg.size());
		std::cout << hex(debug) << "\n";

		socket.send(msg);
		socket.recv(&resp);

		nrequests++;
	}

	auto t1 = Clock::now();
	printf("made %d requests in %s\n", nrequests, deltstr(t0, t1).c_str());
	printf("requests per second = %f", nrequests * 1e9 / delta(t0, t1));

	return 0;
}
