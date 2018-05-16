#include <unistd.h>
#include <iostream>
#include "zmq.hpp"

#include "crypto.hpp"
#include "compress.hpp"

using std::string;

string decrypt(const string key, const string msg, string iv) {
	return xorkeystream(key, msg, iv);
}

bool authenticated(string key, std::vector<string> messages, string fullsig) {
	string hashchain;
	string sig;
	for (string m : messages) {
		sig = hmac(key, m); // TODO: append message index
		hashchain = hash(hashchain + sig);
		key = hash(key);
	}
	return hashchain == fullsig; // TODO: constant time comparison
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
	//string hashchain; // TODO
};

State state;

struct Bundle {
	string sig;
	std::vector<string> messages;
	string err;
};

void handle_messages(Bundle bundle) {
	if (!authenticated(state.key, bundle.messages, bundle.sig)) {
		std::cerr << "error: could not authenticate\n";
		//return;
		// keep going for debugging purposes
	}
	// TODO: update global hashchain?

	decryptall(state.key, bundle.messages, &state.key);
}

uint64_t get64(char* s, size_t off, size_t size) {
	if (size < 8 || off > size-8) {
		throw "out of bounds";
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

Bundle read_bundle(char* data, size_t size) {
	// 16 byte signature
	// uint64   num messages
	// _
	// | uint64          length
	// | bytes * length  contents
	// -- * num messages
	Bundle b;
	const size_t min_size = 32 + 8;
	if (size < min_size) {
		b.err = "too small";
		return b;
	}
	b.sig = string(data, 32);
	size_t n = get64(data, 32, size);
	size_t pos = min_size;
	if (n > 0) {
		size_t length = get64(data, 40, size);
		pos += 8;
		if (length <= size - pos) {
			string m = string(data+pos, length);
			b.messages.push_back(m);
			pos += 8 + size;
		} else {
			// error
			b.err = "message too long";
		}
	}
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
			std::vector<string> messages = {s};
			Bundle b = read_bundle((char*)msg.data(), msg.size());
			if (!b.err.empty()) {
				std::cerr << "error: " << b.err << "\n";
			}
			//handle_messages(state.key, messages);
			std::cout << hex(b.sig)<<"\n";
			std::cout << hex(b.messages[0])<<"\n";
			handle_messages(b);
			std::cout << hex(messages[0]) << "\n";
		}
		sleep(1);
	}

	return 0;
}
