#include <string>
#include <vector>
#include "big.h"
#include "packet.hpp"

using std::vector;
using std::string;

Big P; // 2^1024 + 643

void initP() {
	if (length(P) == 0) {
		P = pow(Big(2), 1024) + 643;
	}
}

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
		Big m(1);
		Big ii(i+1);
		Big y(0);
		for (int j = 0; j < l; j++) {
			// m[i,j] = (i+1)**j mod P
			// y = M[i] dot vec mod P
			y += vec[j] * m;
			m = modmult(m, ii, P);
		}
		output.push_back(bigtostring(y));
	}
	return output;
}

void cancel(vector<vector<Big>> &Mk, int k, int i, int l);

// l-out-of-t recovery
// input is an l-length vector
// output is an l-length vector
// indices is the index of each input string
vector<string> recover(const vector<string> &input, vector<int> indices) {
	initP();

	int l = input.size();
	auto M = vector<vector<Big>>(l, vector<Big>(l+1));

	// initialize augmented vandermonde matrix
	for (int i = 0; i < l; i++) {
		for (int j = 0; j < l; j++) {
			int index = indices.at(i);
			M[i][j] = pow(Big(index), j, P);
		}

		// convert input string to bignums
		// and augment M
		M[i][l] = from_binary(input[i].size(), const_cast<char*>(&input[i][0]));
	}

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
	vector<string> output;
	for (int k = 0; k < l; k++) {
		Big v = moddiv(M[k][l], M[k][k], P);
		output.push_back(bigtostring(v));
	}

	return output;
}

// cancel row M[i] using row M[k]
void cancel(vector<vector<Big>> &M, int k, int i, int l) {
	Big d = moddiv(-M[i][k], M[k][k], P);
	for (int j = 0; i < l+1; j++) {
		M[i][j] = modmult(M[i][j] + M[k][j], d, P);
	}
	assert (M[i][k].iszero());
}
