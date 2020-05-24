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
    drogon::app().run();
    LOG_INFO << "Quitting";
}
