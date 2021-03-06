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

string decode_message(Node* tree, string msg);

std::vector<Node*> read_node_list(const string &msg);
string encode_node_list(const std::vector<Node*> &nodes);
Node* build_tree(std::vector<Node*> nodes);
codemap_t convert_tree(Node* tree);
void walk_node(Node* node, string prefix, codemap_t &codes);
void free_tree(Node* node);
void writebits(string& s, const string &bits);

unsigned char uch(char c) { return static_cast<unsigned char>(c); }

string decompress(const string &msg) {
	size_t tree_length = get64(msg, 0);
	auto nodes = read_node_list(msg.substr(8, tree_length));
	Node *tree = build_tree(nodes);
	string output = decode_message(tree, msg.substr(8 + tree_length));
	free_tree(tree);
	return output;
}

std::vector<Node*> read_node_list(const string &msg) {
	std::vector<Node*> nodes;
	int state = 0;
	// the tree is stored as a list of char, count pairs
	// which provide a histogram of the original input.
	// from these we can reconstruct the tree.
	// the count is stored as a variable-length encoding
	// 1xxxxxxxx          =          0xxxxxxx
	// 0xxxxxxxx 1yyyyyyy = 00xxxxxx xyyyyyyy
	// etc
	char value = 0;
	int count = 0;
	unsigned shift = 0;

	for (char c : msg) {
		if (state == 0) {
			value = c;
			count = 0;
			shift = 0;
			state++;
		} else if (state == 1) {
			unsigned x = uch(c);
			count = count + ((x & 0x7f) << shift);
			shift += 7;
			if (x >> 7) {
				state = 0;

				Node* n = new Node();
				n->leaf = true;
				n->value = value;
				n->count = count;
				nodes.push_back(n);
			}
		}
	}

	// add implicit eof symbol
	Node *eof = new Node();
	eof->leaf = true;
	eof->eof = true;
	eof->count = 1;
	nodes.push_back(eof);

	return nodes;
}

string decode_message(Node* tree, string msg) {
	Node *cur = tree;
	string output = "";
	if (cur->leaf) {
		// must be eof
		return output;
	}
	for (char c : msg) {
		for (int i = 0; i < 8; i++) {
			unsigned bit = (uch(c)>>i) & 1U;
			if (bit == 0) {
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

class BitWriter {
	string *buffer;
	uint64_t bits;
	unsigned n;

public:
	BitWriter(string* buf) {
		this->buffer = buf;
		this->bits = 0;
		this->n = 0;
	}

	void writebits(const string& bits) {
		for (char c : bits) {
			unsigned bit = (c == '1');
			this->bits += (bit&1) << n;
			n++;
			flush();
		}
	}

	void flush() {
		while (n >= 8) {
			buffer->push_back(this->bits & 0xFF);
			this->bits >>= 8;
			n -= 8;
		}
	}

	void close() {
		flush();
		if (n > 0) {
			buffer->push_back(this->bits);
		}
		n = 0;
	}
};


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

	string encoded_nodes = encode_node_list(nodes);

	// add eof symbol
	Node *eof = new Node();
	eof->leaf = true;
	eof->eof = true;
	eof->count = 1;
	nodes.push_back(eof);

	// build tree
	Node *tree = build_tree(nodes);

	// convert tree to codes
	codemap_t codes = convert_tree(tree);

	free_tree(tree);

	// compress
	string output;
	output.resize(8);
	put64(output, encoded_nodes.size());
	output += encoded_nodes;

	BitWriter bw(&output);
	for (char c : msg) {
		bw.writebits(codes.at(c));
	}
	bw.writebits(codes.at('\0')); // eof
	bw.close();

	return output;
}

string encode_node_list(const std::vector<Node*> &nodes) {
	string output;

	for (const Node* n : nodes) {
		output.push_back(n->value);
		auto count = n->count;
		while (count > 0x7F) {
			output.push_back(count & 0x7F);
			count >>= 7;
		}
		output.push_back(count | 0x80);
	}
	return output;
}

bool compare_node_count(const Node *a, const Node *b) {
	return a->count > b->count;
}

// build a tree from a node list, destrying it in the process
Node* build_tree(std::vector<Node*> nodes) {
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

	return nodes.at(0);
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

void free_tree(Node* node) {
	if (!node->leaf) {
		free_tree(node->left);
		free_tree(node->right);
	}
	delete node;
}
