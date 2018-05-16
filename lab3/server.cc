#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include "zmq.hpp"

#include "crypto.hpp"
#include "compress.hpp"

using std::string;

string decrypt(const string key, const string msg, string iv) {
	return xorkeystream(key, msg, iv);
}

string authenticate(string key, std::vector<string> messages, string hashchain) {
	string sig;
	for (string m : messages) {
		sig = hmac(key, m); // TODO: append message index
		hashchain = hash(hashchain + sig);
		key = hash(key);
	}
	return hashchain;
}

void decryptall(string key, std::vector<string> &messages, string *key_out) {
	for (string &m_i : messages) {
		std::cout << "key:" << hex(key) << "\n";
		auto iv = m_i.substr(0, 16);
		auto ciphertext = m_i.substr(16);
		auto compressed = decrypt(key, ciphertext, iv);
		auto plaintext = decompress(compressed);
		std::cout << "ct:" << hex(ciphertext) << "\n";
		std::cout << "cx:" << hex(compressed) << "\n";
		std::cout << "pt:" << hex(plaintext) << "\n";
		m_i = plaintext;
		key = hash(key);
	}
	*key_out = key;
}

struct State {
	string key;
	string hashchain;
};

State state;

struct Bundle {
	string sig;
	std::vector<string> messages;
	string err;
};

void handle_messages(Bundle bundle) {
	string hashchain = authenticate(state.key, bundle.messages, state.hashchain);
	if (hashchain != bundle.sig) { // TODO: constant time comparison
		std::cerr << "error: could not authenticate\n";
		//return;
		// keep going for debugging purposes
	} else {
		state.hashchain = hashchain;
	}

	decryptall(state.key, bundle.messages, &state.key);
}

uint64_t get64(char* s, size_t off, size_t size) {
	if (size < 8 || off > size-8) {
		throw std::out_of_range("get64: out of bounds");
	}
	return ((((uint64_t)(unsigned char)s[off+0])) |
		(((uint64_t)(unsigned char)s[off+1]) << 8) |
		(((uint64_t)(unsigned char)s[off+2]) << 16) |
		(((uint64_t)(unsigned char)s[off+3]) << 24) |
		(((uint64_t)(unsigned char)s[off+4]) << 32) |
		(((uint64_t)(unsigned char)s[off+5]) << 40) |
		(((uint64_t)(unsigned char)s[off+6]) << 48) |
		(((uint64_t)(unsigned char)s[off+7]) << 56));
}

// Decodes a list of strings. Raises std::out_of_rane on failure.
// Data is transfered between client and server as a list of strings.
// uint64   count
// __
// | uint64          length
// | bytes[length]   contents
// -- * count
std::vector<string> read_string_vector(char* data, size_t size) {
	std::vector<string> messages;
	if (size < 8) {
		throw std::out_of_range("too small");
	}
	uint64_t n = get64(data, 0, size);
	size_t pos = 8;
	while (n -- > 0) {
		std::cout << n << " " << pos << "\n";
		if (!(8 <= size - pos)) {
			throw std::out_of_range("length truncated");
		}
		size_t length = get64(data, pos, size);
		std::cout << n << " " << pos << " " << length << "\n";
		pos += 8;
		if (length <= size - pos) {
			string m = string(data+pos, length);
			messages.push_back(m);
			pos += length;
		} else {
			throw std::out_of_range("message too long");
		}
	}
	return messages;
}

Bundle read_bundle(char* data, size_t size) {
	Bundle b;
	std::vector<string> messages;
	try {
		messages = read_string_vector(data, size);
	} catch (const std::out_of_range &e) {
		b.err = e.what();
		return b;
	}
	if (messages.size() < 1) {
		b.err = "no signature";
		return b;
	}
	b.sig = messages.at(0);
	messages.erase(messages.begin());
	b.messages = messages;
	return b;
}

unsigned char key_bytes[] = {197, 48, 233, 58, 115, 6, 244, 205, 123, 253, 215, 37, 8, 36, 216, 170};

int main() {
	state.key.assign(reinterpret_cast<char*>(key_bytes), sizeof key_bytes);

	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://localhost:7000");

	for (;;) {
		// send a request to the client
		zmq::message_t msg;
		socket.send(msg);
		socket.recv(&msg);
		if (msg.size() > 0) {
			auto s = string(reinterpret_cast<char*>(msg.data()), msg.size());
			std::cout << hex(s) << "\n";
			Bundle b = read_bundle((char*)msg.data(), msg.size());
			if (!b.err.empty()) {
				std::cerr << "error: " << b.err << "\n";
			} else {
				//handle_messages(state.key, messages);
				std::cout << hex(b.sig)<<"\n";
				std::cout << hex(b.messages[0])<<"\n";
				handle_messages(b);
				std::cout << hex(b.messages[0]) << "\n";
			}
		}
		sleep(1);
	}

	return 0;
}
