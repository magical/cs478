#include <iostream>
#include <string>
#include <vector>
#include "zmq.hpp"
#include "compress.hpp"
#include "crypto.hpp"

#include <sys/auxv.h>

using std::string;
using std::vector;

csprng rng;

string encrypt(const string key, const string msg, string *iv_out) {
	// random initial counter
	std::string counter(16, '\0');
	for (int i = 0; i < 16; i++) {
		counter[i] = strong_rng(&rng);
	}
	*iv_out = counter;

	return xorkeystream(key, msg, counter);
}

unsigned char key_bytes[] = {197, 48, 233, 58, 115, 6, 244, 205, 123, 253, 215, 37, 8, 36, 216, 170};

string key;
string hashchain;
vector<string> messages;

void save_message(std::string msg) {
	std::string iv;
	auto msg_compressed = compress(msg);
	auto msg_encrypted = encrypt(key, msg_compressed, &iv);
	std::cout << "key:" << hex(key) << "\n";
	std::cout << "ct:" << hex(msg_encrypted) << "\n";
	std::cout << "cx:" << hex(msg_compressed) << "\n";
	std::cout << "pt:" << hex(msg) << "\n";
	std::cout << "de:" << hex(decompress(msg_compressed)) << "\n";
	msg_encrypted = iv + msg_encrypted;
	auto sig = hmac(key, msg_encrypted);
	hashchain = hash(hashchain + sig);
	key = hash(key);
	messages.push_back(msg_encrypted);
}

void write_string_vector(zmq::message_t &buf, const std::vector<string> messages) {
	size_t size = 8;
	for (const string &s : messages) {
		size += 8 + s.size();
	}
	buf.rebuild(size);

	string x(8, '\0');
	put64(x, messages.size());

	memmove(buf.data(), &x[0], x.size());
	size_t pos = 8;
	for (const string &s : messages) {
		put64(x, s.size());
		memmove((char*)buf.data()+pos+0, &x[0], x.size());
		memmove((char*)buf.data()+pos+8, &s[0], s.size());
		pos += 8 + s.size();
	}
}

int main() {
	unsigned long auxrandom = getauxval(AT_RANDOM);
	if (auxrandom == 0) {
		throw "couldn't seed rng";
	}
	strong_init(&rng, 16, (char*)auxrandom, 0);

	key.assign(reinterpret_cast<char*>(key_bytes), sizeof key_bytes);

	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::rep);
	socket.bind("tcp://127.0.0.1:7000");

	zmq_pollitem_t items[2] = {};

	items[0].socket = (void*)socket;
	items[0].events = ZMQ_POLLIN;

	items[1].fd = 0;
	items[1].events = ZMQ_POLLIN;

	std::cout << hex(compress("")) << "\n";
	std::cout << hex(compress("abc")) << "\n";
	std::cout << hex(compress("aaaaaaaabbbbccd")) << "\n";
	std::cout << hex(compress("aaaaaaaabbbcc")) << "\n";
	std::cout << decompress(compress("aaaaaaaabbbcc")) << "\n";
	std::cout << hex(hmac(key, "hi")) << "\n";

	for (;;) {
		int r = zmq::poll(items, 2, 0);
		if (r < 0) {
			perror("poll");
		}

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
				vector<string> bundle;
				bundle.push_back(hashchain);
				for (string s : messages) {
					bundle.push_back(s);
				}
				zmq::message_t msg;
				write_string_vector(msg, bundle);
				socket.send(msg);
				messages.clear();
			} else {
				zmq::message_t msg;
				socket.send(msg);
			}
		}
	}

	return 0;
}
