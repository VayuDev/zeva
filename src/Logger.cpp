#include "Logger.hpp"
#include <iostream>

static void sanitize(std::string& pStr) {
    for(size_t i = 0;i < pStr.size(); ++i) {
        if((pStr.at(i) < '0' || pStr.at(i) > 'z') && pStr.at(i) != ',' && pStr.at(i) != ' ' && pStr.at(i) != '!' && pStr.at(i) != '.') {
            pStr.at(i) = '?';
        }
    }
}

Logger gLog("Global");
Logger& log() {
    return gLog;
}

Logger::Logger(std::string pName)
: mName(std::move(pName)) {
}

void Logger::log(Level level, const char* message) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::string str{message};
    sanitize(str);
    size_t index;
    while((index = str.find("\n")) != std::string::npos) {
        str.replace(index, 1, "");
    }
    switch(level) {
        case Level::Access:
            std::cout << "\033[37;1m";
            break;
        case Level::Debug:
            std::cout << "\033[90;1m";
            break;
        case Level::Info:
            break;
        case Level::Warning:
            std::cout << "\033[93;1m";
            break;
        case Level::Severe:
        case Level::Error:
            std::cout << "\033[31;1m";
            break;
    }
    std::cout << "[" << levelToString(level) << "] [" << mName << "] " << message << "\n";
    std::cout << "\033[0m";
    mLog.emplace_back(std::make_pair(level, str));
}

