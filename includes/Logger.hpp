#pragma once

#include "Includes.hpp"

#define CYAN   "\033[36m"
#define GREEN    "\033[32m"
#define YELLOW "\033[33m"
#define RED   "\033[31m"
#define RESET   "\033[0m"

class Logger {
	private:
		Logger();
		Logger(const Logger &other);
		Logger &operator=(const Logger &other);
		~Logger();
	public:
		static void debug(const std::string &msg);
		static void warning(const std::string &msg);
		static void info(const std::string &msg);
		static void error(const std::string &msg);
		static void error(const std::exception& e);
};
