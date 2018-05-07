#include <iostream>
#include <unistd.h>
#include <string>
#include "zmq.hpp"

int main() {
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://127.0.0.1:7000");

	for (;;) {
		std::string s;
		std::getline(std::cin, s);

		// send a request to the server
		zmq::message_t msg(s.size());
		memmove(msg.data(), &s[0], s.size());
		socket.send(msg);

		zmq::message_t ignore;
		socket.recv(&msg);

		sleep(1);
	}

	return 0;
}
