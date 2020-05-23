#include "wren.hpp"
#include <cassert>
#include <iostream>
#include "Script.hpp"
#include "Logger.hpp"
#include "ScriptManager.hpp"
#include <cstring>
#include <cstdlib>
#include <seasocks/Server.h>
#include <seasocks/Logger.h>
#include <seasocks/PrintfLogger.h>
#include <seasocks/ServerImpl.h>
#include "WebHttpRouter.hpp"
#include "ScriptsApiHandler.hpp"
#include "HtmlHandler.hpp"
#include "PostgreSQLDatabase.hpp"
#include <fstream>
#include <signal.h>
#include "DatabaseHelper.hpp"
#include <ModuleLog.hpp>
#include <DatabaseApiHandler.hpp>
#include <LogApiHandler.hpp>

std::unique_ptr<seasocks::Server> gServer;

static void sighandler(int) {
    if(gServer) {
        gServer->terminate();
    }
}

int main() {
    auto conn = std::make_shared<PostgreSQLDatabase>("testdb");
    conn->query("CREATE TABLE IF NOT EXISTS scripts (id SERIAL, name TEXT UNIQUE, code TEXT)");
    try {
        conn->query("CREATE TABLE protected (id SERIAL, name TEXT)");
        conn->query("INSERT INTO protected (name) VALUES ('scripts'), ('protected')");
    } catch(...) {

    }
    DatabaseHelper::attachNotifyTriggerToAllTables(*conn);
    auto manager = std::make_shared<ScriptManager>(Logger::create("Scripts"));
    conn->addListener([manager](const std::string& pPayload) {
        auto splitterLocation = pPayload.find('%');
        if(splitterLocation == std::string::npos) {
            log().error("Database notify isn't formatted correctly! %s", pPayload.c_str());
        } else {
            manager->onTableChanged(
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
            manager->addScript(name, code);
        } catch(std::exception& e){
            log().error("Failed to handle script exception: %s", e.what());
        }
    }

    auto router = std::make_shared<WebHttpRouter>();
    router->addHandler("api", "log", std::make_shared<LogApiHandler>());
    router->addHandler("api", "scripts", std::make_shared<ScriptsApiHandler>(conn, manager));
    router->addHandler("api", "db", std::make_shared<DatabaseApiHandler>(conn, manager));
    router->addHandler("", "", std::make_shared<HtmlHandler>());

    auto webLogger = Logger::create("Seasocks");
    gServer = std::make_unique<seasocks::Server>(webLogger);
    gServer->addPageHandler(router);
    gServer->addWebSocketHandler("/api/log/ws_log", std::make_shared<ModuleLogWebsocket>(*gServer));
    gServer->startListening(9090);

    log().info("Started server");

    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    while(true) {
        switch(gServer->poll(1)) {
            case seasocks::Server::PollResult::Error:
                gServer.reset();
                return 1;
            case seasocks::Server::PollResult::Terminated:
                gServer.reset();
                return 0;
            case seasocks::Server::PollResult::Continue:
                break;
        }
        conn->awaitNotifications(1);
    }
}
