#include <string>
#include <iostream>
#include <cctype>
#include "crypto.hpp"
#include "compress.hpp"

bool printable(std::string s) {
	for (char c : s) {
		if (!isprint(c)) {
			return false;
		}
	}
	return true;
}

int main() {
	std::cout << hex(compress("")) << "\n";
	std::cout << hex(compress("abc")) << "\n";
	std::cout << hex(compress("aaaaaaaabbbbccd")) << "\n";
	std::cout << hex(compress("aaaaaaaabbbcc")) << "\n";
	std::cout << decompress(compress("aaaaaaaabbbcc")) << "\n";

	for (;;) {
		std::string line;
		if (!std::getline(std::cin, line, '\n')) {
			break;
		}
		std::string s = compress(line);
		std::cout << line << "\n";
		std::cout << hex(s) << "\n";
		std::string d = decompress(s);
		std::cout << hex(d) << "\n";
		if (printable(d)) {
			std::cout << d << "\n";
		} else {
			std::cout << "<unprintable>\n";
		}
	}
}
