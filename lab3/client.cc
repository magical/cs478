#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "zmq.hpp"
#include "compress.hpp"
#include "crypto.hpp"

using std::string;
using std::vector;

csprng rng;

string encrypt(const string key, const string msg, string *iv_out) {
	// AES CTR
	string output;
	std::string counter(16, '\0');
	char keystream[16];

	for (int i = 0; i < 16; i++) {
		counter[i] = strong_rng(&rng);
	}
	*iv_out = counter;

	output.resize(msg.size());
	aes a;
	aes_init(&a, MR_ECB, key.size(), const_cast<char*>(&key[0]), NULL);
	for (size_t i = 0; i < msg.size(); i++) {
		if (i%16 == 0) {
			memmove(keystream, &counter[0], sizeof keystream);
			put64(counter, get64(counter)+1);
			aes_encrypt(&a, keystream);
		}
		output[i] = msg.at(i) ^ keystream[i%16];
	}
	aes_end(&a);
	return output;
}

string hmac(const string &key, const string msg)
{
	const int IPAD = 0x36;
	const int OPAD = 0x5c;
	string tag;
	sha256 outer;
	sha256 inner;
	char x[256/8];
	char K[512/8];

	if (key.size() > sizeof K) {
		throw new std::logic_error("key too large");
	}

	// deviation from standard: always hash the message
	string msghash = hash(msg);

	memset(K, 0, sizeof K);
	memmove(K, &key[0], key.size());

	shs256_init(&inner);
	shs256_init(&outer);
	for (char c : K) {
		shs256_process(&inner, c ^ IPAD);
		shs256_process(&outer, c ^ OPAD);
	}

	for (char c : msghash) {
		shs256_process(&inner, c);
	}
	shs256_hash(&inner, x);

	for (char c : x) {
		shs256_process(&inner, c);
	}
	shs256_hash(&outer, x);

	return std::string(x, sizeof x);
}


string key;
string hashchain;
vector<string> messages;

void save_message(std::string msg) {
	std::string iv;
	auto msg_compressed = compress(msg);
	auto msg_encrypted = encrypt(key, msg_compressed, &iv);
	auto sig = hmac(key, msg_encrypted);
	hashchain = hash(hashchain + sig);
	key = hash(key);
	messages.push_back(msg_encrypted);
}

int main() {
	char buf[1];
	strong_init(&rng, 0, buf, 0);

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
