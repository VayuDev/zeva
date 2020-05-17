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

void sighandler(int) {
    if(gServer) {
        gServer->terminate();
    }
}

int main() {
    auto conn = std::make_shared<PostgreSQLDatabase>("testdb");
    conn->query("CREATE TABLE IF NOT EXISTS scripts (id SERIAL, name TEXT UNIQUE, code TEXT)");


    auto manager = std::make_shared<ScriptManager>();
    //load scripts
    auto scripts = conn->query("SELECT name,code FROM scripts");
    for(size_t i = 0; i < scripts->getRowCount(); ++i) {
        auto name = scripts->getValue(i, 0).stringValue;
        auto code = scripts->getValue(i, 1).stringValue;
        manager->addScript(name, code);
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
    gServer->loop();
}
