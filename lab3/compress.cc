#include <algorithm>
#include <string>
#include <map>
#include "crypto.hpp"

using std::string;

// huffman compression

class Node {
public:
	bool leaf;
	int count;

	char value; // for leaf nodes
	bool eof;

	Node* left; // for non-leaf nodes
	Node* right;

	Node() : leaf(false), count(0), value(0), eof(false), left(NULL), right(NULL) {}
};

typedef std::map<char, Node*> histogram_t;
typedef std::map<char, string> codemap_t;

Node* decode_tree(string msg);
string decode_message(Node* tree, string msg);

codemap_t convert_tree(Node* tree);
void walk_node(Node* node, string prefix, codemap_t &codes);
void writebits(string& s, const string &bits);

string decompress(const string &msg) {
	size_t tree_length = get64(msg, 0);
	Node *tree = decode_tree(msg.substr(8, tree_length));
	return decode_message(tree, msg.substr(8 + tree_length));
}

Node* decode_tree(string msg) {
	return NULL;
}

string decode_message(Node* tree, string msg) {
	Node *cur = tree;
	string output = "";
	for (char c : msg) {
		for (int i = 0; i < 8; i++) {
			unsigned bit = (((unsigned char)c)>>i) & 1U;
			if (bit) {
				cur = cur->left;
			} else {
				cur = cur->right;
			}
			if (cur->leaf) {
				if (cur->eof) {
					goto done;
				}
				output.push_back(cur->value);
				cur = tree;
			}
		}
	}
done:
	return output;
}

bool compare_node_count(const Node *a, const Node *b) {
	return a->count > b->count;
}

string compress(const string &msg) {
	// Initialize list of nodes
	std::vector<Node*> nodes;

	// build histogram of input characters
	histogram_t hist;
	for (char c : msg) {
		if (hist.count(c) == 0) {
			hist[c] = new Node();
			hist[c]->leaf = true;
			hist[c]->value = c;
			nodes.push_back(hist[c]);
		}
		hist[c]->count++;
	}

	// add eof symbol
	Node *eof = new Node();
	eof->leaf = true;
	eof->eof = true;
	eof->count = 1;
	nodes.push_back(eof);

	// build tree
	std::make_heap(nodes.begin(), nodes.end(), compare_node_count);
	while (nodes.size() > 1) {
		// pop the two lowest nodes to form a new node
		Node *a = nodes.at(0);
		std::pop_heap(nodes.begin(), nodes.end(), compare_node_count);
		nodes.pop_back();

		Node *b = nodes.at(0);
		std::pop_heap(nodes.begin(), nodes.end(), compare_node_count);
		nodes.pop_back();

		Node *parent = new Node();
		parent->left = a;
		parent->right = b;
		parent->count = a->count + b->count;
		nodes.push_back(parent);
		std::push_heap(nodes.begin(), nodes.end(), compare_node_count);
	}

	Node *tree = nodes.at(0);

	// convert tree to codes
	codemap_t codes = convert_tree(tree);;

	// compress
	string output;
	for (char c : msg) {
		writebits(output, codes.at(c));
	}
	writebits(output, codes.at('\0'));
	return output;
}


codemap_t convert_tree(Node* tree) {
	codemap_t codes;
	walk_node(tree, "", codes);
	if (tree->leaf) {
		walk_node(tree, "0", codes);
	}
	return codes;
}

void walk_node(Node* node, string prefix, codemap_t &codes) {
	if (node->leaf) {
		codes[node->value] = prefix;
	} else {
		walk_node(node->left, prefix + "0", codes);
		walk_node(node->right, prefix + "1", codes);
	}
}

void writebits(string& output, const string& bits) {
	output += bits;
}

void test_decompress() {
	decompress("");
}
