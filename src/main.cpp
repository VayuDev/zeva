#include "wren.hpp"
#include <cassert>
#include <iostream>
#include "Script.hpp"
#include "ScriptManager.hpp"
#include <cstring>
#include <cstdlib>
#include <fstream>

#include <drogon/HttpAppFramework.h>
#include "PostgreSQLDatabase.hpp"
#include "DatabaseHelper.hpp"
#include "LogWebsocket.hpp"

#include <csignal>
#include "Util.hpp"
#include <regex>

static void sighandler(int) {
    drogon::app().quit();
}

const char* CONFIG_FILE = "assets/config.json";

class LogNotificationReceiver : private pqxx::notification_receiver {
public:
    LogNotificationReceiver(pqxx::connection& pConn, std::shared_ptr<LogWebsocket> pLog)
            : pqxx::notification_receiver(pConn, "newlog"), mLog(std::move(pLog)) {}

    void operator()(const std::string &payload, int) override {
        auto firstPercentageIndex = payload.find('%');
        auto level = std::stoll(payload.substr(0, firstPercentageIndex));
        auto msg = payload.substr(firstPercentageIndex + 1, std::string::npos);
        mLog->newLogMessage(level, msg);
    }
private:
    std::shared_ptr<LogWebsocket> mLog;
};

int main() {
    //init database
    auto conn = std::make_shared<PostgreSQLDatabase>(CONFIG_FILE);
    DatabaseHelper::createDb(*conn);

    srand(time(0));
    //event trigger
    conn->addListener([](const std::string& pPayload) {
        auto splitterLocation = pPayload.find('%');
        if(splitterLocation == std::string::npos) {
            LOG_ERROR << "Database notify isn't formatted correctly!" << pPayload;;
        } else {
            ScriptManager::the().onTableChanged(
                    pPayload.substr(0, splitterLocation),
                    pPayload.substr(splitterLocation + 1, pPayload.length() - splitterLocation - 1));
        }

    });

    auto logWebsocketController = std::make_shared<LogWebsocket>();
    LogNotificationReceiver recv{conn->getConnection(), logWebsocketController};

    long passedTime = std::numeric_limits<long>::max();
    //handle notifications
    drogon::app().getLoop()->runEvery(0.1, [conn, &passedTime] () mutable {
        conn->awaitNotifications(1);
        //delete the old log about every hour
        if(passedTime++ > 60 * 600) {
            conn->query("DELETE FROM log WHERE created < (CURRENT_TIMESTAMP - '7 days' :: interval)");
            passedTime = 0;
        }
    });

    //setup log
    std::regex levelFinder{R"([a-zA-Z]+)"};
    trantor::Logger::setOutputFunction([&levelFinder, conn, logWebsocketController](const char* str, uint64_t len) {
        if(isValidAscii(reinterpret_cast<const signed char *>(str), len)) {
            std::cout.write(str, len);
            if(drogon::app().isRunning()) {
                std::string msg{str, len - 1};
                std::smatch levelMatch;
                if (!std::regex_search(msg, levelMatch, levelFinder)) {
                    assert(false);
                }
                std::string restString = levelMatch.suffix();
                if (!std::regex_search(restString, levelMatch, levelFinder)) {
                    assert(false);
                }
                const std::string &logLevel = levelMatch[0];
                int64_t level = stringToLogLevel(logLevel);
                drogon::app().getDbClient()->execSqlAsync(
                        "INSERT INTO log (level, created, msg) VALUES ($1, CURRENT_TIMESTAMP, $2)",
                        [](const drogon::orm::Result &) {},
                        [](const drogon::orm::DrogonDbException &) {},
                        level, std::move(msg));
            }
        } else {
            std::cout << "INVALID ASCII\n";
        }
    }, []() {
        std::cout << std::flush;
    });

    //setup webserver
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGABRT, sighandler);
    drogon::app().addListener("0.0.0.0", 8080);
    drogon::app().loadConfigFile(CONFIG_FILE);
    drogon::app().setLogLevel(trantor::Logger::LogLevel::kDebug);
    drogon::app().registerController(logWebsocketController);

    auto resp404 = drogon::HttpResponse::newHttpResponse();
    resp404->setStatusCode(drogon::HttpStatusCode::k404NotFound);
    resp404->setBody("Not Implemented");
    drogon::app().setCustom404Page(resp404);

    drogon::app().setCustomErrorHandler([](drogon::HttpStatusCode pStatus) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(pStatus);
        resp->setBody(std::to_string((int) pStatus));
        return resp;
    });

    drogon::app().registerHandler("/", [](const drogon::HttpRequestPtr&, std::function<void (const drogon::HttpResponsePtr &)> && callback) {
        callback(drogon::HttpResponse::newRedirectionResponse("/hub/hub.html"));
    });

    //load scripts
    drogon::app().getLoop()->queueInLoop([] {
        auto client = std::make_shared<PostgreSQLDatabase>(CONFIG_FILE);
        auto res = client->query("SELECT name,code FROM scripts");
        for(size_t i = 0; i < res->getRowCount(); ++i) {
            auto name = res->getValue(i, 0).stringValue;
            auto code = res->getValue(i, 1).stringValue;
            try {
                ScriptManager::the().addScript(name, code);
            } catch(std::exception& e){
                LOG_ERROR << "Failed to load script '" << name << "' with exception: " << e.what();
            }
        }
        LOG_WARN << "Server completely started";
    });
    drogon::app().run();
    LOG_WARN << "Quitting server";
}
