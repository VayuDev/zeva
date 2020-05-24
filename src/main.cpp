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


static void sighandler(int) {

}

int main() {
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGABRT, sighandler);
    drogon::app().addListener("0.0.0.0", 8080);
    drogon::app().loadConfigFile("assets/config.json");

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
