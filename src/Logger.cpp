#include "Logger.hpp"
#include <iostream>

bool first = true;

Logger gLog;
Logger& log() {
    if(first) {
        first = false;
    } else {
        std::cout << "\n";
    }
    gLog.mLog.emplace_back(std::make_pair(gLog.mLastLogType, std::move(gLog.mLastMsg)));
    gLog.mLastLogType = LogType::INFO;
    return gLog;
}

Logger& Logger::operator<<(std::string pStr) {
    std::cout << pStr;
    mLastMsg += pStr;
}

Logger& Logger::error(std::string pStr) {
    std::cerr << pStr << std::flush;
    mLastMsg += pStr;
    gLog.mLastLogType = LogType::ERROR;
}