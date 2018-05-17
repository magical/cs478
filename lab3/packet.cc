
#include <string>
#include <vector>
#include <stdexcept>
#include "packet.hpp"
#include "crypto.hpp" // get64
#include "rabin.hpp"

using std::string;

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

// Decodes a list of strings. Raises std::out_of_range on failure.
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
		if (!(8 <= size - pos)) {
			throw std::out_of_range("length truncated");
		}
		size_t length = get64(data, pos, size);
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

Packet read_packet(zmq::message_t &msg) {
	Packet p;
	std::vector<std::string> pieces;
	try {
		pieces = read_string_vector((char*)msg.data(), msg.size());
	} catch (const std::out_of_range &e) {
		p.err = e.what();
		return p;
	}
	if (pieces.size() < 1) {
		p.err = "no packet header";
		return p;
	}
	string header = pieces.at(0);
	pieces.erase(pieces.begin());
	p.i = get64(header, 0);
	p.l = get64(header, 8);
	p.t = get64(header, 16);
	p.pieces = pieces;
	return p;
}

// Split a string into packets
// using n-out-of-t redundancy
// returns vector of t packets
std::vector<Packet> split_message(const std::string &s, int n, int t) {
	std::vector<Packet> packets(t);
	for (int i = 0; i < t; i++) {
		packets[i].i = i+1;
		packets[i].l = n;
		packets[i].t = t;
	}

	size_t pos = 0;
	while (pos < s.size()) {
		std::vector<string> pieces;
		// get one chunk for each packet
		// pad string with trailing zeros if necessary
		for (int i = 0; i < n; i++) {
			if (pos == s.size()) {
				std::string x(ChunkSize, '\0');
				pieces.push_back(x);
			} else if (s.size() - pos < ChunkSize) {
				std::string x = s.substr(pos);
				x.resize(ChunkSize, '\0');
				pieces.push_back(x);
				pos = s.size();
			} else {
				pieces.push_back(s.substr(pos, ChunkSize));
				pos += ChunkSize;
			}
		}
		pieces = dispersal(pieces, t);
		for (int i = 0; i < n; i++) {
			packets[i].pieces.push_back(pieces.at(i));
		}
	}
	return packets;
}

// reconstruct the original message out of packets
// uses n-out-of-t redundancy
// returns empty string if there are fewer than n packets
// if there are fewer t
string reconstruct_packets(std::vector<Packet> packets, int n) {
	string output;
	if (packets.size() == 0) {
		return output;
	}
	if (n > packets.size()) {
		return output;
	}
	int len = packets[0].pieces.size();
	for (Packet& p : packets) {
		if (p.pieces.size() != len) {
			throw std::out_of_range("packets do not have the same number of pieces");
		}
	}
	std::vector<int> indices;
	for (int i = 0; i < n; i++) {
		indices.push_back(packets[i].i);
	}
	// compine the first piece from each packet
	// then the second piece from each packet
	// and so on
	for (int i = 0; i < n; i++) {
		std::vector<string> pieces;
		for (Packet& p : packets) {
			pieces.push_back(p.pieces[i]);
		}
		output += recover(pieces, indices);
	}
	return output;
}
