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

#include <csignal>
#include "Util.hpp"
#include <regex>

static void sighandler(int) {
    drogon::app().quit();
}

const char* CONFIG_FILE = "assets/config.json";

int main() {
    //init database
    auto conn = std::make_shared<PostgreSQLDatabase>(CONFIG_FILE);
    conn->query("CREATE TABLE IF NOT EXISTS scripts (id SERIAL PRIMARY KEY, name TEXT UNIQUE, code TEXT)");
    try {
        conn->query("CREATE TABLE protected (id SERIAL PRIMARY KEY, name TEXT UNIQUE)");
        conn->query("INSERT INTO protected (name) VALUES ('scripts'), ('protected')");
    } catch(...) {}
    //timelog table
    conn->query(R"(
CREATE TABLE IF NOT EXISTS timelog (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL
))");
    conn->query(R"(
CREATE TABLE IF NOT EXISTS timelog_activity (
    id SERIAL PRIMARY KEY,
    timelogid BIGINT NOT NULL REFERENCES timelog(id),
    name TEXT NOT NULL
))");
    conn->query(
            R"(
CREATE TABLE IF NOT EXISTS timelog_entry (
    timelogid BIGINT NOT NULL REFERENCES timelog(id),
    activityid BIGINT NOT NULL REFERENCES timelog_activity(id),
    duration INTERVAL NOT NULL,
    PRIMARY KEY(timelogid,activityid)
))");
    //log table
    try {
        conn->query("CREATE TABLE log_level (id SERIAL PRIMARY KEY, name TEXT UNIQUE)");
        conn->query(R"(
INSERT INTO log_level (id, name) VALUES
    (0, 'trace'),
    (1, 'debug'),
    (2, 'info'),
    (3, 'warn'),
    (4, 'error'),
    (5, 'fatal'),
    (6, 'syserr');
)");
    } catch(std::exception& e) {
    }


    conn->query(R"(
CREATE TABLE IF NOT EXISTS log (
    id SERIAL PRIMARY KEY,
    created TIMESTAMP WITH TIME ZONE NOT NULL,
    level BIGINT NOT NULL REFERENCES log_level(id),
    msg TEXT NOT NULL
))");

    DatabaseHelper::attachNotifyTriggerToAllTables(*conn);

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

    //setup log
    std::regex levelFinder{R"([a-zA-Z]+)"};
    trantor::Logger::setOutputFunction([&levelFinder, conn](const char* str, uint64_t len) {
        if(isValidAscii(reinterpret_cast<const signed char *>(str), len)) {
            std::cout.write(str, len);
            std::string msg{str, len - 1};
            int64_t level = 0;
            std::smatch levelMatch;
            if(!std::regex_search(msg, levelMatch, levelFinder)) {
                assert(false);
            }
            std::string restString = levelMatch.suffix();
            if(!std::regex_search(restString, levelMatch, levelFinder)) {
                assert(false);
            }
            const std::string& logLevel = levelMatch[0];
            if(logLevel == "TRACE") {
                level = 0;
            } else if(logLevel == "DEBUG") {
                level = 1;
            } else if(logLevel == "INFO") {
                level = 2;
            } else if(logLevel == "WARN") {
                level = 3;
            } else if(logLevel == "ERROR") {
                level = 4;
            } else if(logLevel == "FATAL") {
                level = 5;
            } else if(logLevel == "SYSERR") {
                level = 6;
            }
            drogon::app().getDbClient()->execSqlAsync("INSERT INTO log (level, created, msg) VALUES ($1, CURRENT_TIMESTAMP, $2)",
                                                      [](const drogon::orm::Result&) {},
                                                      [](const drogon::orm::DrogonDbException&) {},
                                                      level, std::move(msg));
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
    drogon::app().getLoop()->queueInLoop([conn] {
        //load scripts
        auto scripts = conn->query("SELECT name,code FROM scripts");
        for(size_t i = 0; i < scripts->getRowCount(); ++i) {
            auto name = scripts->getValue(i, 0).stringValue;
            auto code = scripts->getValue(i, 1).stringValue;
            try {
                ScriptManager::the().addScript(name, code);
            } catch(std::exception& e){
                LOG_ERROR << "Failed to load script '" << name << "' with exception: " << e.what();
            }
        }
        LOG_INFO << "Server completely started";
    });
    drogon::app().run();
    LOG_INFO << "Quitting server";
}
