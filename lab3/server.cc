#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include "zmq.hpp"

#include "crypto.hpp"
#include "compress.hpp"
#include "packet.hpp"

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

void handle_messages(Bundle &bundle) {
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

string reconstruct_packets(std::vector<Packet> packets) {
	string output;
	// TODO: apply RID
	// take the first piece from each packet and smash them together
	// and so on
	if (packets.size() == 0) {
		return output;
	}
	int n = packets[0].pieces.size();
	for (Packet& p : packets) {
		if (p.pieces.size() != n) {
			throw std::out_of_range("packets do not have the same number of pieces");
		}
	}
	for (int i = 0; i < n; i++) {
		for (Packet& p : packets) {
			output += p.pieces[0];
		}
	}
	return output;
}

unsigned char key_bytes[] = {197, 48, 233, 58, 115, 6, 244, 205, 123, 253, 215, 37, 8, 36, 216, 170};

int main() {
	state.key.assign(reinterpret_cast<char*>(key_bytes), sizeof key_bytes);

	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://localhost:7000");

	zmq::socket_t sub(context, zmq::socket_type::sub);
	sub.connect("tcp://127.0.0.1:37813");
	sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);

	zmq::message_t empty;
	zmq::message_t ignore;

	for (;;) {
		zmq::message_t response;

		// send a request to the client
		socket.send(empty);
		socket.recv(&response);

		std::cout << "got response\n";

		// wait for messages pieces to come in
		if (response.size() > 0) {
			std::vector<Packet> packets;
			// TODO: timeout
			for (int i = 0; i < 4; i++) {
				zmq::message_t msg;
				sub.recv(&msg);
				std::cout << "got packet\n";
				Packet p = read_packet(msg);
				if (!p.err.empty()) {
					std::cerr << "error: " << p.err << "\n";
				} else {
					packets.push_back(p);
				}
			}

			if (packets.size() >= 4) {
				string reconstructed = reconstruct_packets(packets);
				std::cout << hex(reconstructed) << "\n";
				Bundle b = read_bundle(&reconstructed[0], reconstructed.size());
				if (!b.err.empty()) {
					std::cerr << "error: " << b.err << "\n";
				} else {
					//handle_messages(state.key, messages);
					//std::cout << hex(b.sig)<<"\n";
					//std::cout << hex(b.messages[0])<<"\n";
					handle_messages(b);
					//std::cout << hex(b.messages[0])<<"\n";
					for (const string &r : b.messages) {
						std::cout << r << "\n";
					}
				}
			}
		}
		sleep(1);
	}

	return 0;
}
