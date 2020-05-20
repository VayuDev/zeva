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
#include "ApiHandler.hpp"
#include "HtmlHandler.hpp"
#include "PostgreSQLDatabase.hpp"
#include <fstream>
#include <signal.h>

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
    conn->performCopyToStdout("COPY (SELECT * FROM scripts ORDER BY id ASC) TO STDOUT WITH (DELIMITER ',', FORMAT CSV, HEADER);");
    conn->addListener([](const std::string& pPayload) {
        log().error(pPayload.c_str());
    });

    auto manager = std::make_shared<ScriptManager>();
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
    router->addHandler(std::make_shared<ApiHandler>(conn, manager));
    router->addHandler(std::make_shared<HtmlHandler>());

    auto webLogger = std::make_shared<Logger>("Seasocks");
    gServer = std::make_unique<seasocks::Server>(webLogger);
    gServer->addPageHandler(router);
    gServer->startListening(9090);

    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    while(true) {
        switch(gServer->poll(1)) {
            case seasocks::Server::PollResult::Error:
                return 1;
            case seasocks::Server::PollResult::Terminated:
                return 0;
            case seasocks::Server::PollResult::Continue:
                break;
        }
        conn->awaitNotifications(1);
    }
}
