#include <string>
#include <vector>
#include <stdexcept>
#include <assert.h>
#include "crypto.hpp"
#include "merkle.hpp"
#include "bitreader.hpp"
#include "pq.hpp"

#include <iostream>

using std::vector;
using std::string;

// "post quantum" signatures using HORS and Merkle trees

const int kappa = 128, k = 32, t = 1024;
const int log2t = 10;

static bool ispoweroftwo(uint64_t x) {
	return x != 0 && (x & (x-1)) == 0;
}

// Returns the root of a merkle tree,
// where the leaves are generated implictly from a seed
static string pq_merkle_create(string z, int len) {
	assert(ispoweroftwo(len));
	vector <string> stack;
	for (int i = 0; i < len; i++) {
		string secret = hash(z + str64(i));
		string leaf = hash(secret);
		stack.push_back(hash(leaf));
		for (unsigned j = i; (j & 1) == 1; j >>= 1) {
			string t = hash(stack[stack.size()-2] + stack.back());
			stack.pop_back();
			stack.pop_back();
			stack.push_back(t);
		}
	}
	assert(stack.size() == 1);
	return stack.at(0); // root
}

// Returns the path to node at index v
// where the leaves of the tree are generated from a seed value
static vector<string> pq_merkle_path(int v, const vector<string> &leaf_hashes, int len) {
	assert(ispoweroftwo(len));
	vector <string> stack;
	vector <string> path;
	int level = 0;
	for (int i = 0; i < len; i++) {
		stack.push_back(leaf_hashes[i]);

		if (v == i) {
			level = stack.size();
		}

		// merge the stack
		for (unsigned j = i; (j & 1) == 1; j >>= 1) {
			auto b = stack.back();
			stack.pop_back();
			auto a = stack.back();
			stack.pop_back();
			stack.push_back(hash(a + b));

			if (i >= v) {
				if (stack.size() == level) {
					path.push_back(b);
				} else if (stack.size() < level) {
					path.push_back(a);
					level = stack.size();
				}
			}
		}
	}
	assert(stack.size() == 1);
	return path;
}


void pq_keygen(int d, PQPrivateKey *sk, PQPublicKey *pk) {
	int st = 1;
	auto z = random_bytes(128/8);
	auto root = pq_merkle_create(z, d*t);
	*sk = PQPrivateKey(z, st, d);
	*pk = PQPublicKey(root, d);
}

// signs msg and updates sk.st
PQSignature pq_sign(string msg, PQPrivateKey &sk) {
	if (sk.st > sk.d){
		throw std::out_of_range("exceeded max number of signatures for this key");
	}
	// split hash of message into pieces
	// for each piece, do HORS on leaves t*(st-1)..t*st of the tree
	// note: hash must be >= 320 bits
	BitReader br(hash384(msg));
	vector<vector<string>> path(k);
	vector<string> s(k);
	auto st = sk.st - 1;

	// precompute hashed merkle leaves to speed up repeated path operations
	vector<string> leaf_hashes;
	const int len = t*sk.d;
	for (int i = 0; i < len; i++) {
		string secret = hash(sk.z + str64(i));
		string leaf = hash(secret);
		leaf_hashes.push_back(hash(leaf));
	}

	for (int j = 0; j < k; j++) {
		auto hj = br.readbits(log2t);
		//std::cout << "sign: " << hj << "\n";
		auto i = st * t + hj;
		s[j] = hash(sk.z + str64(i));
		path[j] = pq_merkle_path(i, leaf_hashes, t*sk.d);
	}
	sk.st++;
	return PQSignature(path, s, st);
}

bool pq_verify(string msg, PQSignature sig, PQPublicKey pk) {
	if (sig.path.size() != k || sig.s.size() != k) {
		return false;
	}
	if (!ispoweroftwo(pk.d)) {
		return false;
	}
	BitReader br(hash384(msg));
	for (int j = 0; j < k; j++) {
		auto hj = br.readbits(log2t);
		//std::cout << "verify: " << hj << "\n";
		auto i = sig.st * t + hj;
		auto v = hash(sig.s.at(j));
		if (!merkle_verify(pk.root, sig.path.at(j), v, i)) {
			return false;
		}
	}
	return true;
}
