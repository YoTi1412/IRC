#include "Includes.hpp"
#include "Logger.hpp"
#include <sstream>

Logger::Logger() {}

Logger::Logger(const Logger &other) {
	(void)other;
}

Logger &Logger::operator=(const Logger &other) {
	if (this != &other) {
		*this = other;
	}
	return *this;
}

Logger::~Logger() {}

static inline std::string currentTimestamp() {
    time_t now = time(NULL);
    return Utils::formatTime(now);
}

static inline void printHeader(std::ostream &os, const char *color, const char *level) {
    os << color << "[" << currentTimestamp() << "] [" << level << "] " << RESET;
}

void Logger::warning(const std::string &msg) {
    printHeader(std::cerr, YELLOW, "WARNING");
    std::cerr << msg << std::endl;
}

void Logger::info(const std::string &msg) {
    printHeader(std::cout, GREEN, "INFO");
    std::cout << msg << std::endl;
}

void Logger::error(const std::exception& e) {
    printHeader(std::cerr, RED, "ERROR");
    std::cerr << "Exception: " << e.what() << std::endl;
}
