#include <iostream>
#include "zmq.hpp"

int main() {
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::rep);
	socket.bind("tcp://127.0.0.1:7000");

	for (;;) {
		zmq::message_t msg;
		socket.recv(&msg);

		auto s = std::string(reinterpret_cast<char*>(msg.data()), msg.size());
		std::cout << s << "\n";

		zmq::message_t empty;
		socket.send(empty);
	}

	return 0;
}
