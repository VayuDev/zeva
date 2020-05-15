#include "wren.hpp"
#include <cassert>
#include <iostream>
#include "Script.hpp"
#include "Logger.hpp"
#include "ScriptManager.hpp"
#include "zegdb.hpp"
#include "common.hpp"
#include <cstring>
#include <cstdlib>
#include <seasocks/Server.h>
#include <seasocks/Logger.h>
#include <seasocks/PrintfLogger.h>
#include <seasocks/ServerImpl.h>
#include "WebHttpRouter.hpp"
#include "ApiHandler.hpp"
#include "HtmlHandler.hpp"

int main() {
    auto conn = std::make_shared<DatabaseNetworkConnection>("localhost", 5120);
    conn->querySync("SELECT * FROM sample_min_csv");

    auto webLogger = std::make_shared<Logger>("Seasocks");
    seasocks::Server server(webLogger);

    auto router = std::make_shared<WebHttpRouter>();
    router->addHandler(std::make_shared<ApiHandler>(conn));
    router->addHandler(std::make_shared<HtmlHandler>());
    server.addPageHandler(router);
    server.startListening(9090);
    server.loop();

    ScriptManager manager;
    auto[data, len] = readWholeFile("../assets/samples/test.wren");
    char cpy[len + 1];
    memcpy(cpy, data, len);
    cpy[len] = '\0';
    free(data);
    manager.addScript("test", cpy);
    auto ret = manager.executeScript("test", "onRunOnce", [](WrenVM* vm) {
        wrenSetSlotDouble(vm, 1, 1.2);
    }).get();
    
    ScriptValue& sRet = std::get<ScriptValue>(ret);
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
}
