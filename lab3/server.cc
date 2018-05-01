#include <unistd.h>
#include <iostream>
#include "zmq.hpp"

int main() {
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://localhost:7000");

	for (;;) {
		// send a request to the client
		zmq::message_t msg;
		socket.send(msg);
		socket.recv(&msg);
		auto s = std::string(reinterpret_cast<char*>(msg.data()), msg.size());
		std::cout << s << "\n";
		sleep(1);
	}

	return 0;
}
