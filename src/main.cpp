#include "wren.hpp"
#include <cassert>
#include <iostream>
#include "Script.hpp"
#include "ScriptManager.hpp"
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <signal.h>
#include <drogon/HttpAppFramework.h>
#include "PostgreSQLDatabase.hpp"
#include "DatabaseHelper.hpp"

static void sighandler(int) {

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

    DatabaseHelper::attachNotifyTriggerToAllTables(*conn);

    //load scripts
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

    trantor::Logger::setOutputFunction([](const char* str, uint64_t len) {
        std::cout.write(str, len);
    }, []() {
        std::cout << std::flush;
    });


    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGABRT, sighandler);
    drogon::app().addListener("0.0.0.0", 8080);
    drogon::app().loadConfigFile(CONFIG_FILE);

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
    drogon::app().run();
    LOG_INFO << "Quitting";
}
