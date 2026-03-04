#include <iostream>
#include <string>

class Logger {
public:
	enum class Level {
		L_DEBUG,
		L_INFO,
		L_WARNING,
		L_ERROR
	};
	static void log(Level level, const std::string& message) {
		std::string prefix;
		switch (level) {
		case Level::L_DEBUG:   prefix = "[DEBUG]: "; break;
		case Level::L_INFO:    prefix = "[INFO]: "; break;
		case Level::L_WARNING: prefix = "[WARNING]: "; break;
		case Level::L_ERROR:   prefix = "[ERROR]: "; break;
		}
		time_t now = time(0);
		tm* timeinfo = localtime(&now);
		char timestamp[20];
		strftime(timestamp, sizeof(timestamp),
			"%Y-%m-%d %H:%M:%S", timeinfo);
		std::cout << "[" << timestamp << "] " << prefix << message << std::endl;
	}
};