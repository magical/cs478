#pragma once

#include <string>

extern "C" {
#include "miracl.h"
}

class Message { };

void shs256_process_string(sha256 *sh, std::string s);
uint64_t get64(const std::string &s, size_t off = 0);
void put64(std::string &s, uint64_t t);
void setbits(std::string &s, uint64_t x, int bits);
std::string hash(const std::string msg);
std::string hex(std::string s);
std::string hmac(const std::string &key, const std::string msg);
std::string xorkeystream(const std::string &key, const std::string &msg, std::string counter);
