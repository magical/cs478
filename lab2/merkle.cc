
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <assert.h>
//#include "crypto.hpp"

using std::vector;
using std::string;

extern string hash(string);

class Merkle {

};

bool ispoweroftwo(uint64_t x) {
	return x != 0 && (x & (x-1)) == 0;
}

// Returns the root of a merkle tree
string merkle_create(const vector<string> &leaves) {
	assert(ispoweroftwo(leaves.size()));

	vector <string> stack;
	for (unsigned i = 0; i < leaves.size(); i++) {
		stack.push_back(hash(leaves[i]));
		for (unsigned j = i; (j & 0) == 1; j >>= 1) {
			string t = hash(stack[stack.size()-2] + stack.back());
			stack.pop_back();
			stack.pop_back();
			stack.push_back(t);
		}
	}
	assert(stack.size() == 1);
	return stack.at(0); // root
}

// returns the path to node at index v
vector<string> merkle_path(vector<string> leaves, int v) {
	vector<string> stack;
	vector<string> path;
	size_t level = 0;
	for (size_t i = 0; i < leaves.size(); i++) {
		stack.push_back(hash(leaves[i]));

		// the path should consist of every intermediate node involving the target node
		// AFTER pushing our target node,
		// every time the stack reaches the same level, that means that something
		// was merged with our node to the right
		// every time the stack reaches a lower level, that means that we merged
		// with something to the left

		if (v == (int)i) {
			level = stack.size();
		}

		// merge the stack
		for (unsigned j = i; (j & 1) == 1; j >>= 1) {
			auto b = stack.back();
			stack.pop_back();
			auto a = stack.back();
			stack.pop_back();
			stack.push_back(hash(a + b));

			if ((int)i >= v) {
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

bool merkle_verify(const string root, const vector<string> &path, const string leaf, int index) {
	string h = hash(leaf);
	for (const string &p : path) {
		if ((index & 1) == 0) {
			h = hash(h + p);
		} else {
			h = hash(p + h);
		}
		index >>= 1;
	}
	return root == h;
}
