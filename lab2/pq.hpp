#pragma once
#include <string>
#include <vector>

struct PQPrivateKey {
	std::string z;
	int st;
	int d;

	PQPrivateKey(std::string z, int st, int d) : z(z), st(st), d(d) {}
};

struct PQPublicKey {
	std::string root;
	int d;

	PQPublicKey(std::string root, int d) : root(root), d(d) {}
};

struct PQSignature {
	std::vector<std::vector<std::string>> path;
	std::vector<std::string> s;
	int st;

	PQSignature() : st(1) {}
	PQSignature(std::vector<std::vector<std::string>> path, std::vector<std::string> s, int st) {
		this->path = path;
		this->s = s;
		this->st = st;
	}
};

void pq_keygen(int d, PQPrivateKey *sk, PQPublicKey *pk);
PQSignature pq_sign(std::string msg, PQPrivateKey sk);
bool pq_verify(std::string msg, PQSignature sig, PQPublicKey pk);
