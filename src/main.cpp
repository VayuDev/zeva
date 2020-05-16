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
int main() {
    auto conn = std::make_shared<PostgreSQLDatabase>("testdb");
    conn->query("CREATE TABLE IF NOT EXISTS scripts (id SERIAL, name TEXT, code TEXT)");
    auto webLogger = std::make_shared<Logger>("Seasocks");
    seasocks::Server server(webLogger);



    ScriptManager manager;
    std::ifstream t("../assets/samples/test.wren");
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    manager.addScript("test", str);
    auto ret = manager.executeScript("test", "onRunOnce", [](WrenVM* vm) {
        wrenSetSlotDouble(vm, 1, 1.2);
    }).get();
    
    auto& sRet = std::get<ScriptValue>(ret);
    switch(sRet.type) {
    case WrenType::WREN_TYPE_NUM:
        log().info("%f", sRet.doubleValue);
        break;
    case WrenType::WREN_TYPE_STRING: 
        log().info("%s", sRet.stringValue);
        free(sRet.stringValue);
        break;
    default:
        break;
    }


    auto router = std::make_shared<WebHttpRouter>();
    router->addHandler(std::make_shared<ApiHandler>(conn));
    router->addHandler(std::make_shared<HtmlHandler>());
    server.addPageHandler(router);
    server.startListening(9090);
    server.loop();
}
