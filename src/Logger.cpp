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
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME_COARSE, &ts);
    //delete the oldest two messages if they are older than one day
    //we only delete at most two to increase the performance of the logger.
    //Old messages get deleted slowly one after the other when new ones
    //come in.
    size_t iterations = 0;
    for(auto it = mLog.cbegin(); it != mLog.cend() && iterations++ < 2;) {
        auto timestamp = std::get<time_t>(*mLog.cbegin());
        if(ts.tv_sec - 60 * 60 * 24 >= timestamp) {
            it = mLog.erase(it);
        } else {
            it++;
        }
    }

    const auto& inserted = mLog.emplace_back(std::forward_as_tuple(level, ts.tv_sec, str));
    for(auto& handler: mHandlers) {
        handler.second(inserted);
    }
}

std::shared_ptr<Logger> Logger::getLog(const std::string &pName) {
    std::shared_lock<std::shared_mutex> lock{sLogRegistryMutex};
    return sLogRegistry.at(pName);
}

Logger::~Logger() {
    //TODO find a way to delete a logger upon destruction
    //std::lock_guard<std::shared_mutex> lock{sLogRegistryMutex};
    //sLogRegistry.erase(mName);
}

std::shared_ptr<Logger> Logger::create(const std::string &pName) {
    auto logger = std::shared_ptr<Logger>(new Logger{pName});
    std::lock_guard<std::shared_mutex> lock{sLogRegistryMutex};
    sLogRegistry.emplace(pName, logger);
    return logger;
}

std::forward_list<std::string> Logger::getAllLoggerNames() {
    std::forward_list<std::string> ret;
    std::shared_lock<std::shared_mutex> lock{sLogRegistryMutex};
    for(const auto& logger: sLogRegistry) {
        ret.push_front(logger.first);
    }
    return ret;
}
