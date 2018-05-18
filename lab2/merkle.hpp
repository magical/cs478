#pragma once
#include <string>
#include <vector>

std::string merkle_create(const std::vector<std::string> &leaves);
std::vector<std::string> merkle_path(std::vector<std::string> leaves, int v);
bool merkle_verify(const std::string root, const std::vector<std::string> &path, const std::string leaf, int index);
