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
    size_t length = mCode.size();
    write(mOutputFd, &length, sizeof(length));
    write(mOutputFd, mCode.c_str(), length);

    //check for errors
    char cmd;
    read(mInputFd, &cmd, 1);
    read(mInputFd, &length, sizeof(length));
    char err[length + 1];
    read(mInputFd, err, sizeof(err));
    err[length] = '\0';
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
    write(mOutputFd, "E", 1);
    Json::Value msg;
    msg["function"] = pFunctionName;
    Json::Value jsonParams;
    for(const auto& pVal: params) {
        jsonParams.append(scriptValueToJson(pVal));
    }
    msg["params"] = std::move(jsonParams);
    auto str = msg.toStyledString();
    auto length = str.size();
    write(mOutputFd, &length, sizeof(length));
    write(mOutputFd, str.c_str(), str.size());
    return std::async(std::launch::deferred, [this, lock=std::move(lock)] () -> std::variant<std::string, Json::Value> {
        char cmd;
        read(mInputFd, &cmd, 1);
        size_t length;
        read(mInputFd, &length, sizeof(length));
        char response[length + 1];
        read(mInputFd, response, length);
        response[length] = '\0';
        switch(cmd) {
            case 'J': {
                std::stringstream reader;
                reader.write(response, length + 1);
                Json::Value responseJson;
                reader >> responseJson;

                if(responseJson.type() != Json::nullValue) {
                    //remove the new line at the end
                    //we can't just pass the buffer and replace the \n with \0
                    //because this collides with the template-magic of trantor's log
                    std::string responseStr{response, length - 1};
                    LOG_INFO << "[Script] " << mModule << " return json: " << responseStr;
                }

                return responseJson;
            }
            case 'R': {
                std::string responseStr{response, length};
                if(isValidAscii(reinterpret_cast<const signed char *>(responseStr.c_str()), responseStr.size())) {
                    LOG_INFO << "[Script] " << mModule << " return string: " << response;
                } else {
                    LOG_INFO << "[Script] " << mModule << " invalid ascii return";
                }

                return responseStr;
            }
            case 'E': {
                std::string errorString{response, length};
                LOG_INFO << "[Script] " << mModule << " error " << errorString;
                throw std::runtime_error(errorString);
            }
            default:
                assert(false);
        }
    });
}
