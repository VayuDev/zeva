#include "Logger.hpp"
#include <iostream>

bool first = true;

static void sanitize(std::string& pStr) {
    for(size_t i = 0;i < pStr.size(); ++i) {
        if((pStr.at(i) < '0' || pStr.at(i) > 'z') && pStr.at(i) != ',' && pStr.at(i) != ' ' && pStr.at(i) != '!' && pStr.at(i) != '.') {
            pStr.at(i) = '?';
        }
    }
}

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
    sanitize(pStr);
    size_t index;
    while((index = pStr.find("\n")) != std::string::npos) {
        pStr.replace(index, 1, "");
    } 
    std::cout << pStr;
    mLastMsg += pStr;
    return *this;
}

Logger& Logger::error(std::string pStr) {
    sanitize(pStr);
    size_t index;
    while((index = pStr.find("\n")) != std::string::npos) {
        pStr.replace(index, 1, "");
    } 
    std::cerr << pStr;
    mLastMsg += pStr;
    gLog.mLastLogType = LogType::ERROR;
    return *this;
}