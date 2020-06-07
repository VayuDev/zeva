#include "ScriptBindings.hpp"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <json/json.h>
#include "Util.hpp"
#include <cassert>
#include <stdio.h>
#include <limits.h>
#include <iostream>
#include <drogon/HttpAppFramework.h>

ScriptBindings::ScriptBindings(const std::string &pModule, const std::string &pCode)
: mCode(pCode), mModule(pModule) {
    std::array<int, 2> outputFd, inputFd;
    if(pipe(outputFd.data()) != 0) {
        perror("pipe()");
    }
    if(pipe(inputFd.data()) != 0) {
        perror("pipe()");
    }

    auto inputStr = std::to_string(outputFd.at(0));
    auto outputStr = std::to_string(inputFd.at(1));
    setenv("INPUT_FD",  inputStr.c_str(), 1);
    setenv("OUTPUT_FD", outputStr.c_str(), 1);
    setenv("SCRIPT_NAME", pModule.c_str(), 1);
    pid_t pid = fork();
    if(pid == 0) {
        //child
        int fdlimit = (int)sysconf(_SC_OPEN_MAX);
        for (int i = STDERR_FILENO + 1; i < fdlimit; i++) {
            if(i != outputFd.at(0) && i != inputFd.at(1)) [[likely]] {
                close(i);
            }
        }
        char cwd[PATH_MAX];
        if(getcwd(cwd, sizeof(cwd)) == nullptr) {
            perror("getcwd()");
        }
        std::string path{cwd};
        path += "/assets/ZeVaScript";
        std::string name{"ZeVaScript: "};
        name.append(pModule);
        execl(path.c_str(), name.c_str(), "", nullptr);
        perror("execl");
        exit(1);
    }
    //parent
    close(outputFd.at(0));
    close(inputFd.at(1));
    mOutputFd = outputFd.at(1);
    mInputFd = inputFd.at(0);
    unsetenv("INPUT_FD");
    unsetenv("OUTPUT_FD");
    unsetenv("SCRIPT_NAME");
    mPid = pid;

    //send code
    safeWrite('C', mCode.c_str(), mCode.size());

    char cmd;
    auto err = safeRead(cmd);
    if(cmd == 'O') {

    } else if(cmd == 'E') {
        throw std::runtime_error(err);
    } else {
        assert(false);
    }
}

ScriptBindings::~ScriptBindings() {
    if(kill(mPid, SIGTERM)) {
        perror("kill()");
    }
    int status;
    waitpid(mPid, &status, 0);
    close(mOutputFd);
    close(mInputFd);
}

const std::string &ScriptBindings::getCode() {
    return mCode;
}

std::future<std::variant<std::string, Json::Value>> ScriptBindings::execute(const std::string &pFunctionName, const std::vector<ScriptValue>& params) {
    std::unique_lock<std::mutex> lock(mFdMutex);
    //TODO check for errors and incomplete messages

    Json::Value msg;
    msg["function"] = pFunctionName;
    Json::Value jsonParams;
    for(const auto& pVal: params) {
        jsonParams.append(scriptValueToJson(pVal));
    }
    msg["params"] = std::move(jsonParams);
    auto str = msg.toStyledString();
    auto length = str.size();
    safeWrite('E', str.c_str(), str.size());
    return std::async(std::launch::deferred, [this, lock=std::move(lock)] () -> std::variant<std::string, Json::Value> {

        char cmd;
        std::string response = safeRead(cmd);
        switch(cmd) {
            case 'J': {
                std::stringstream reader{response};
                Json::Value responseJson;
                reader >> responseJson;

                if(responseJson.type() != Json::nullValue) {
                    //remove the new line at the end
                    response.at(response.size() - 1) = '\0';
                    LOG_INFO << "[Script] " << mModule << " return json: " << response;
                }

                return responseJson;
            }
            case 'R': {
                //printf("Received raw string of size: %i\n", (int)length);
                if(isValidAscii(reinterpret_cast<const signed char *>(response.c_str()), response.size())) {
                    LOG_INFO << "[Script] " << mModule << " return string: " << response;
                } else {
                    LOG_INFO << "[Script] " << mModule << " invalid ascii return";
                }

                return response;
            }
            case 'E': {
                LOG_INFO << "[Script] " << mModule << " error " << response;
                throw std::runtime_error(response);
            }
            default:
                assert(false);
        }
    });
}

std::string ScriptBindings::safeRead(char& cmd) {
    auto safeRead = [this](void* dest, size_t length) {
        size_t bytesRead = 0;
        while(bytesRead < length) {
            auto status = read(mInputFd, (char*)dest + bytesRead, length - bytesRead);
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

void ScriptBindings::safeWrite(char cmd, const void *buffer, size_t length) {
    auto safeWrite = [this](const void* data, size_t length) {
        size_t bytesWritten = 0;
        while(bytesWritten < length) {
            auto status = write(mOutputFd, (char*)data + bytesWritten, length - bytesWritten);
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
