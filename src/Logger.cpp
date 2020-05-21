#include "Logger.hpp"
#include <iostream>
#include <memory>
#include <utility>

std::shared_ptr<Logger> gLog;

Logger &log() {
    if(!gLog)
        gLog = Logger::create("Global");
    return *gLog;
}

std::map<std::string, std::shared_ptr<Logger>> Logger::sLogRegistry;
std::shared_mutex Logger::sLogRegistryMutex;

static void sanitize(std::string& pStr) {
    for(char i : pStr) {
        if(!isprint(i)) {
            i = '?';
        }
    }
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
    const auto& inserted = mLog.emplace_back(std::make_pair(level, str));
    for(auto& handler: mHandlers) {
        handler.second(inserted);
    }
}

std::shared_ptr<Logger> Logger::getLog(const std::string &pName) {
    std::shared_lock<std::shared_mutex> lock{sLogRegistryMutex};
    return sLogRegistry.at(pName);
}

Logger::~Logger() {
    std::lock_guard<std::shared_mutex> lock{sLogRegistryMutex};
    sLogRegistry.erase(mName);
}

std::shared_ptr<Logger> Logger::create(const std::string &pName) {
    auto logger = std::shared_ptr<Logger>(new Logger{pName});
    std::lock_guard<std::shared_mutex> lock{sLogRegistryMutex};
    sLogRegistry.emplace(pName, logger);
    return logger;
}
