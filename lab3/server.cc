#include <unistd.h>
#include <iostream>
#include "zmq.hpp"

#include "crypto.hpp"
#include "compress.hpp"

using std::string;

string decrypt(const string key, const string msg, string iv) {
	return xorkeystream(key, msg, iv);
}

bool authenticate(string key, std::vector<string> messages, string fullsig) {
	string hashchain;
	string sig;
	for (string m : messages) {
		sig = hmac(key, m); // TODO: append message index
		hashchain = hash(hashchain + sig);
		key = hash(key);
	}
	return hashchain == fullsig; // TODO: constant time comparison
	// TODO: update global hashchain
}

void decryptall(string key, std::vector<string> messages) {
	for (string &m_i : messages) {
		auto iv = m_i.substr(0, 16);
		auto ciphertext = m_i.substr(16);
		auto compressed = decrypt(key, ciphertext, iv);
		auto plaintext = decompress(compressed);
		m_i = plaintext;
		key = hash(key);
	}
}

string key;
unsigned char key_bytes[] = {197, 48, 233, 58, 115, 6, 244, 205, 123, 253, 215, 37, 8, 36, 216, 170};

int main() {
	key.assign(reinterpret_cast<char*>(key_bytes), sizeof key_bytes);

	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://localhost:7000");

	for (;;) {
		// send a request to the client
		zmq::message_t msg;
		socket.send(msg);
		socket.recv(&msg);
		auto s = std::string(reinterpret_cast<char*>(msg.data()), msg.size());
		std::cout << hex(s) << "\n";
		sleep(1);
	}

	return 0;
}
