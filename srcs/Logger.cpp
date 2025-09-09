#include "Logger.hpp"

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

void Logger::debug(const std::string &msg) {
	std::cout << CYAN << "[DEBUG] " << msg << RESET << std::endl;
}

void Logger::warning(const std::string &msg) {
	std::cout << YELLOW << "[WARNING] " << msg << RESET << std::endl;
}

void Logger::info(const std::string &msg) {
	std::cout << GREEN << "[INFO] " << msg << RESET << std::endl;
}

void Logger::error(const std::string &msg) {
	std::cerr << RED << "[ERROR] " << msg << RESET << std::endl;
}

void Logger::error(const std::exception& e) {
	std::cerr << RED << "[ERROR] Exception: " << e.what() << RESET << std::endl;
}
