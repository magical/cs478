#include <string>
#include <vector>
#include "big.h"
#include "packet.hpp"

using std::vector;
using std::string;

Big *P = nullptr; // 2^1024 + 643

void initP() {
	if (P == nullptr) {
		P = new Big();
		*P = pow(Big(2), 1024) + 643;
	}
}

void printmatrix(const vector<vector<Big>> &M);

string bigtostring(const Big& v) {
	string output(ChunkSize, '\0');
	to_binary(v, output.size(), &output[0], FALSE);
	return output;
}

// l-out-of-t dispersal
// input is an l-length vector
// output is a t-length vector
vector<string> dispersal(const vector<string> &input, int t) {
	initP();

	int l = input.size();
	assert(2 < l && l < t);

	// convert strings to bignums
	vector<Big> vec(l);
	for (int i = 0; i < input.size(); i++) {
		vec[i] = from_binary(input[i].size(), const_cast<char*>(&input[i][0]));
	}

	// generate vandermonde matrix on the fly and do a matrix mutiply with vec
	// we assume the message has already been broken into independent chunks of t elements
	vector<string> output;
	for (int i = 0; i < t; i++) {
		Big v = 1;
		Big ii = i+1;
		Big y = 0;
		for (int j = 0; j < l; j++) {
			// m[i,j] = (i+1)**j mod P
			// y = M[i] dot vec mod P
			y = modmultadd(v, vec[j], y, *P);
			v = modmult(v, ii, *P);
		}
		output.push_back(bigtostring(y));
	}
	return output;
}

void cancel(vector<vector<Big>> &Mk, int k, int i, int l);

// l-out-of-t recovery
// input is an l-length vector
// output is the concatenation of all l output pieces
// indices is the index of each input string
string recover(const vector<string> &input, vector<int> indices) {
	initP();

	int l = input.size();
	auto M = vector<vector<Big>>(l, vector<Big>(l+1));

	// initialize augmented vandermonde matrix
	for (int i = 0; i < l; i++) {
		int index = indices.at(i);
		for (int j = 0; j < l; j++) {
			M[i][j] = pow(Big(index), j, *P);
		}

		// convert input string to bignums
		// and augment M
		M[i][l] = from_binary(input[i].size(), const_cast<char*>(&input[i][0]));
	}

	//printmatrix(M);

	// solve with gauss-jordan elimination
	// make upper triangular
	for (int k = 0; k < l; k++) {
		for (int i = k+1; i < l; i++) {
			cancel(M, k, i, l);
		}
	}

	// make diagonal
	for (int k = l-1; k >= 0; k--) {
		for (int i = k-1; i >= 0; i--) {
			cancel(M, k, i, l);
		}
	}

	// convert solution to bytes
	string output;
	for (int k = 0; k < l; k++) {
		Big v = moddiv(M[k][l], M[k][k], *P);
		output += bigtostring(v);
	}

	return output;
}

// cancel row M[i] using row M[k]
void cancel(vector<vector<Big>> &M, int k, int i, int l) {
	Big d = moddiv(*P-M[i][k], M[k][k], *P);
	for (int j = 0; j < l+1; j++) {
		// M_ij += d * M_kj  mod p
		M[i][j] = modmultadd(d, M[k][j], M[i][j], *P);
	}
	//printmatrix(M);
	assert (M[i][k].iszero());
}

void printmatrix(const vector<vector<Big>> &M) {
	get_mip()->IOBASE = 10;
	for (size_t i = 0; i < M.size(); i++) {
		for (size_t j = 0; j < M[i].size(); j++) {
			if (j != 0) { std::cerr << " "; }
			std::cerr << M.at(i).at(j);
		}
		std::cerr << "\n";
	}
	std::cerr << "\n";
}
