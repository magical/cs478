#include <iostream>
#include <string>
#include <vector>
#include "zmq.hpp"

using std::string;
using std::vector;

string encrypt(string key, string msg) { return msg; }
string compress(string msg) { return msg; }
string hmac(string key, string msg) { return msg; }
string hash(string msg) { return msg; }

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

	for (;;) {
		// when a messae arrives,
		// queue the message
		//
		// when the collector arrives,
		// send a batch of messages
	}

	return 0;
}
