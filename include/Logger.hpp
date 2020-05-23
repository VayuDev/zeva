#pragma once
#include <list>
#include <string>
#include <seasocks/Logger.h>
#include <mutex>
#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <shared_mutex>
#include <atomic>
#include <forward_list>

using LogHandler = std::function<void(const std::pair<seasocks::Logger::Level, std::string>&)>;

class Logger final : public seasocks::Logger {
public:
    ~Logger() override;
    void log(Level level, const char* message) override;

    static std::shared_ptr<Logger> create(const std::string& pName);
    static std::shared_ptr<Logger> getLog(const std::string& pName);
    static std::forward_list<std::string> getAllLoggerNames();
private:
    static std::map<std::string, std::shared_ptr<Logger>> sLogRegistry;
    static std::shared_mutex sLogRegistryMutex;

    explicit Logger(std::string pName);

    friend class ModuleLogWebsocket;
    std::mutex mMutex;
    std::list<std::pair<Level, std::string>> mLog;
    std::string mName;

    std::atomic<int> mHandlerIdCounter = 0;
    std::map<int, LogHandler> mHandlers;

    inline int appendHandler(LogHandler pHandler) {
        int id = mHandlerIdCounter++;
        mHandlers.emplace(id, std::move(pHandler));
        return id;
    }
    inline void deleteHandler(int pId) {
        mHandlers.erase(pId);
    }
};

Logger& log();