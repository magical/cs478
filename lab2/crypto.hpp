#pragma once

#include <string>

uint64_t get64(const std::string &s, size_t off = 0);
void put64(std::string &s, uint64_t t);
std::string str64(uint64_t t);
std::string hash(const std::string msg);
std::string hex(std::string s);
std::string hmac(const std::string &key, const std::string msg);
std::string random_bytes(int n);
