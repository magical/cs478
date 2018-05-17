#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/auxv.h>
#include "zmq.hpp"
#include "big.h"

#include "compress.hpp"
#include "crypto.hpp"
#include "packet.hpp"

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

// do not use this function directly
void _write_string_vector(char* buf, size_t size, const std::vector<string> messages) {
	string x(8, '\0');
	put64(x, messages.size());

	memmove(buf, &x[0], x.size());
	size_t pos = 8;
	for (const string &s : messages) {
		put64(x, s.size());
		memmove(&buf[pos+0], &x[0], x.size());
		memmove(&buf[pos+8], &s[0], s.size());
		pos += 8 + s.size();
	}
}

void write_string_vector(zmq::message_t *buf, const std::vector<string> messages) {
	size_t size = 8;
	for (const string &s : messages) {
		size += 8 + s.size();
	}
	buf->rebuild(size);
	_write_string_vector((char*)buf->data(), size, messages);
}

void write_string_vector(std::string *buf, const std::vector<string> messages) {
	size_t size = 8;
	for (const string &s : messages) {
		size += 8 + s.size();
	}
	buf->resize(size);
	_write_string_vector(&(*buf)[0], size, messages);
}

string str64(uint64_t t) {
	string output(8, '\0');
	put64(output, t);
	return output;
}

void write_packet(zmq::message_t *buf, const Packet& packet) {
	std::vector<string> pieces;
	string header = str64(packet.i) + str64(packet.l) + str64(packet.t);
	pieces.push_back(header);
	for (const string& p : packet.pieces) {
		pieces.push_back(p);
	}
	write_string_vector(buf, pieces);
}

int main() {
	Miracl precision = 100;
	unsigned long auxrandom = getauxval(AT_RANDOM);
	if (auxrandom == 0) {
		throw "couldn't seed rng";
	}
	strong_init(&rng, 16, (char*)auxrandom, 0);

	key.assign(reinterpret_cast<char*>(key_bytes), sizeof key_bytes);

	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::rep);
	socket.bind("tcp://127.0.0.1:7000");

	zmq::socket_t pub(context, zmq::socket_type::pub);
	pub.bind("tcp://127.0.0.1:37813");

	zmq_pollitem_t items[2] = {};

	items[0].socket = (void*)socket;
	items[0].events = ZMQ_POLLIN;

	items[1].fd = 0;
	items[1].events = ZMQ_POLLIN;

	//std::cout << hex(hmac(key, "hi")) << "\n";

	for (;;) {
		int r = zmq::poll(items, 2, 0);
		if (r < 0) {
			perror("poll");
		}

		// when a message arrives,
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
			zmq::message_t empty;
			socket.recv(&ignore);

			if (!messages.empty()) {
				zmq::message_t response(1);
				socket.send(response);

				vector<string> bundle;
				bundle.push_back(hashchain);
				for (string s : messages) {
					bundle.push_back(s);
				}
				std::string full_message;
				write_string_vector(&full_message, bundle);

				std::vector<Packet> packets = split_message(full_message, RabinN, RabinT);
				zmq::message_t msg;
				for (const Packet &p : packets) {
					write_packet(&msg, p);
					pub.send(msg);
					std::cout << "sent a packet\n";
				}

				messages.clear();
			} else {
				socket.send(empty);
			}
		}
	}

	return 0;
}
