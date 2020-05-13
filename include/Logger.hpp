#pragma once
#include <list>
#include <string>

enum class LogType {
    DEBUG,
    INFO,
    WARN,
    ERROR,
};

class Logger {
public:
    Logger& operator<<(std::string pStr);
    Logger& error(std::string pStr);
private:
    friend Logger& log();
    std::string mLastMsg;
    LogType mLastLogType = LogType::INFO;;
    std::list<std::pair<LogType, std::string>> mLog;
};

Logger& log();