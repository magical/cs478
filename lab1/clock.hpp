#include <chrono>
#include <sstream>
typedef std::chrono::high_resolution_clock Clock;

long delta(Clock::time_point a, Clock::time_point b) {
	long d = std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
	return d;
}

std::string deltstr(Clock::time_point a, Clock::time_point b) {
	long d = std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
	std::stringstream ss;
	if (d < 1000) {
		ss << d << "ns";
	} else if (d < 1000000) {
		ss << d/1000 << "µs";
	} else {
		ss << d/1000000 << "ms";
	}
	return ss.str();
}

