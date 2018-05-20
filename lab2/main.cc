#include <string>
#include <iostream>
#include <sstream>
#include <chrono>

#include "crypto.hpp"
#include "hors.hpp"
#include "pq.hpp"

using std::string;

typedef std::chrono::high_resolution_clock Clock;

string delt(Clock::time_point a, Clock::time_point b) {
	long d = std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
	std::stringstream ss;
	if (d < 1000) {
		ss << d << "ns";
	} else if (d < 1000000) {
		ss << d/1000 << "µs";
	} else {
		ss << d/1000000 << "ms";
	}
	return ss.str();
}

int main() {
	init_rng();
	string msg = "hello there";
	PQPrivateKey sk;
	PQPublicKey pk;
	auto t0 = Clock::now();
	pq_keygen(16, &sk, &pk);
	auto t1 = Clock::now();
	auto sig0 = pq_sign(msg, sk);
	auto t2 = Clock::now();
	auto sig1 = pq_sign(msg, sk);
	auto t3 = Clock::now();

	if (pq_verify(msg, sig0, pk)) {
		std::cout << "verified sig 0\n";
	} else {
		std::cout << "FAILED sig 0\n";
	}
	auto t4 = Clock::now();
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

	std::cout << "keygen took "<<delt(t0, t1)<<"\n";
	std::cout << "signing took "<<delt(t1, t2)<<"\n";
	std::cout << "signing took "<<delt(t2, t3)<<"\n";
	std::cout << "verify took "<<delt(t3,t4)<<"\n";
	return 0;
}
