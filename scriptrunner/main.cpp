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
#include <sys/prctl.h>
#include <csignal>

static std::string safeRead(int pInputFd, char& cmd) {
    auto safeRead = [pInputFd](void* dest, size_t length) {
        size_t bytesRead = 0;
        while(bytesRead < length) {
            auto status = read(pInputFd, (char*)dest + bytesRead, length - bytesRead);
            if(status == -1) {
                throwError("read()");
            }
            bytesRead += status;
        }
    };
    safeRead(&cmd, 1);
    size_t length;
    safeRead(&length, sizeof(size_t));
    char buffer[length];
    safeRead(buffer, length);
    return std::string{buffer, length};
}

void safeWrite(int pOutputFd, char cmd, const void *buffer, size_t length) {
    auto safeWrite = [pOutputFd](const void* data, size_t length) {
        size_t bytesWritten = 0;
        while(bytesWritten < length) {
            auto status = write(pOutputFd, (char*)data + bytesWritten, length - bytesWritten);
            if(status == -1) {
                throwError("write()");
            }
            bytesWritten += status;
        }
    };
    safeWrite(&cmd, 1);
    safeWrite(&length, sizeof(size_t));
    safeWrite(buffer, length);
}

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

    char cmd;
    auto code = safeRead(input, cmd);
    assert(cmd == 'C');

    auto sendStr = [&] (char cmd, std::string_view pStr) {
        safeWrite(output, cmd, pStr.data(), pStr.size());
    };

    try {
        Script script{name, code};
        sendStr('O', "ok");
        while(true) {
            char cmd;
            auto buffer = safeRead(input, cmd);
            switch(cmd) {
                case 'E': {
                    std::stringstream inputStream{buffer};
                    Json::Value msg;
                    inputStream >> msg;
                    auto params = msg["param"];
                    try {
                        auto ret = script.execute(msg["function"].asString(), msg["params"]);
                        auto *rawString = std::get_if<std::string>(&ret);
                        if(rawString) {
                            //printf("Sending raw string off length %i\n", (int)rawString->size());
                            sendStr('R', *rawString);
                        } else {
                            const auto& json = std::get<Json::Value>(ret);
                            auto formatted = json.toStyledString();
                            //printf("Received command %c with parameter %i", cmd, (int) length);
                            //printf("Sending json string: '%s'\n", formatted.c_str());
                            sendStr('J', formatted);
                        }
                    } catch(std::exception& e) {
                        sendStr('E', e.what());
                    }

                    break;
                }
                default:
                    assert(false);
            }
        }
    } catch(std::exception& e) {
        sendStr('E', e.what());
    }



}