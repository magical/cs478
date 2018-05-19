#include <string>
#include <iostream>

#include "crypto.hpp"
#include "hors.hpp"
#include "pq.hpp"


using std::string;
int main() {
	init_rng();
	PQPrivateKey sk;
	PQPublicKey pk;
	pq_keygen(8, &sk, &pk);
	string msg = "hello there";
	auto sig0 = pq_sign(msg, sk);
	auto sig1 = pq_sign(msg, sk);

	if (pq_verify(msg, sig0, pk)) {
		std::cout << "verified sig 0\n";
	} else {
		std::cout << "FAILED sig 0\n";
	}
	if (pq_verify(msg, sig1, pk)) {
		std::cout << "verified sig 1\n";
	} else {
		std::cout << "FAILED sig 1\n";
	}

	auto badmsg = "goodbye";
	if (!pq_verify(badmsg, sig0, pk)) {
		std::cout << "did not verify bad msg with sig 0\n";
	} else {
		std::cout << "FAILED verified bad msg with sig 0\n";
	}

}
