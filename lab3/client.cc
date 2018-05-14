#include <iostream>
#include <string>
#include <vector>
#include "zmq.hpp"
#include "compress.hpp"
#include "crypto.hpp"

using std::string;
using std::vector;

string encrypt(string key, string msg) { return msg; }
string hmac(string key, string msg) { return msg; }

string key;
string hashchain;
vector<string> messages;

void save_message(std::string msg) {
	auto msg_compressed = compress(msg);
	auto msg_encrypted = encrypt(key, msg_compressed);
	auto sig = hmac(key, msg_encrypted);
	hashchain = hash(hashchain + sig);
	key = hash(key);
	messages.push_back(msg_encrypted);
}

int main() {
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::rep);
	socket.bind("tcp://127.0.0.1:7000");

	zmq_pollitem_t items[2] = {};

	items[0].socket = (void*)socket;
	items[0].events = ZMQ_POLLIN;

	items[1].fd = 0;
	items[1].events = ZMQ_POLLIN;

	std::cout << compress("") << "\n";
	std::cout << compress("abc") << "\n";
	std::cout << compress("aaaaaaaabbbbccd") << "\n";

	for (;;) {
		int r = zmq::poll(items, 2, 0);

		// when a messae arrives,
		// queue the message
		if (items[1].revents & ZMQ_POLLIN) {
			if (!std::cin) {
				break;
			}
			std::cout << "got a message\n";
			std::string line;
			std::getline(std::cin, line);
			save_message(line);
		}

		// when the collector arrives,
		// send a batch of messages
		if (items[0].revents & ZMQ_POLLIN) {
			std::cout << "got a collection request\n";

			zmq::message_t ignore;
			socket.recv(&ignore);

			if (!messages.empty()) {
				string s = messages.back();
				zmq::message_t msg(s.size());
				memmove(msg.data(), s.data(), s.size());
				socket.send(msg);
				messages.pop_back();
			} else {
				zmq::message_t msg;
				socket.send(msg);
			}
		}
	}

	return 0;
}
