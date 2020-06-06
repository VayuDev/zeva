#include <iostream>
#include <json/json.h>
#include <string>
#include <chrono>
#include <fstream>
#include <zconf.h>
#include "Base64.hpp"
#include "Script.hpp"
#include "Util.hpp"
#include <cassert>

int main() {
    auto inputFdStr = getenv("INPUT_FD");
    auto outputFdStr = getenv("OUTPUT_FD");
    auto name = getenv("SCRIPT_NAME");
    if(inputFdStr == nullptr || outputFdStr == nullptr || name == nullptr) {
        fprintf(stderr, "Missing env variables\n");
        exit(1);
    }
    auto input = atoi(inputFdStr);
    auto output = atoi(outputFdStr);
    size_t length;
    read(input, &length, sizeof(length));
    char code[length + 1];
    read(input, code, length);
    code[length] = '\0';

    auto sendStr = [&] (char pType, const std::string& pStr) {
        write(output, &pType, 1);
        size_t length = pStr.size();
        write(output, &length, sizeof(length));
        write(output, pStr.c_str(), pStr.size());
    };

    try {
        Script script{name, code};

        sendStr('O', "ok");

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
        while(true) {
            char cmd;
            read(input, &cmd, 1);
            size_t length;
            read(input, &length, sizeof(length));
            char buffer[length + 1];
            read(input, buffer, length);
            buffer[length] = '\0';
            switch(cmd) {
                case 'E': {
                    std::stringstream inputStream;
                    inputStream.write(buffer, length + 1);
                    Json::Value msg;
                    inputStream >> msg;
                    auto params = msg["param"];
                    auto ret = script.execute(msg["function"].asString(), msg["params"]);
                    auto *rawString = std::get_if<std::string>(&ret);
                    if(rawString) {
                        //std::cout << "Sending raw string: " << *rawString << std::endl;
                        sendStr('R', *rawString);
                    } else {
                        const auto& json = std::get<Json::Value>(ret);
                        auto formatted = json.toStyledString();
                        //printf("Received command %c with parameter %i", cmd, (int) length);
                        //printf("Sending json string: '%s'\n", formatted.c_str());
                        sendStr('J', formatted);
                    }
                    break;
                }
                default:
                    assert(false);
            }
        }
#pragma clang diagnostic pop
    } catch(std::exception& e) {
        sendStr('E', e.what());
    }



}