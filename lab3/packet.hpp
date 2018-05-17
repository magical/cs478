#pragma once

#include <string>
#include <vector>
#include "zmq.hpp"

const int ChunkSize = 128; // 1024 bits
const int RabinN = 4;
const int RabinT = 8;

// A packet is a portion of a rabin-encoded message
// if you have l packets out of t, you can reconstruct the original message.
struct Packet {
	int i; // this packet's index
	int l; // how many packets we need
	int t; // how many packets were sent
	std::vector<std::string> pieces;
	std::string err;

	Packet() : i(0), l(0), t(0) {}
};

std::vector<std::string> read_string_vector(char* data, size_t size);
Packet read_packet(zmq::message_t &msg);
std::vector<Packet> split_message(const std::string &s, int n, int t);
std::string reconstruct_packets(std::vector<Packet> packets, int n);
