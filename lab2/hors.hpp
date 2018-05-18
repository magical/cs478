#include <vector>
#include <string>

void hors_keygen(unsigned keysize, int t);
std::vector<std::string> hors_sign(const std::string msg, const std::vector<std::string> &xs);
bool hors_verify(const std::string msg, const std::vector<std::string> sig, const std::vector<std::string> &ys);
