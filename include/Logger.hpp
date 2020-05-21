#pragma once
#include <list>
#include <string>
#include <seasocks/Logger.h>
#include <mutex>

class Logger : public seasocks::Logger {
public:
    explicit Logger(std::string pName);
    virtual void log(Level level, const char* message) override;
private:
    friend Logger& log();
    std::mutex mMutex;
    std::list<std::pair<Level, std::string>> mLog;
    std::string mName;
};

Logger& log();