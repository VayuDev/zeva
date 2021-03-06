#include "ScriptBindings.hpp"
#include "Util.hpp"
#include <ScriptManager.hpp>
#include <cassert>
#include <drogon/HttpAppFramework.h>
#include <iostream>
#include <json/json.h>
#include <limits.h>
#include <linux/prctl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

ScriptBindings::ScriptBindings(const std::string &pModule,
                               const std::string &pCode, uint32_t timeoutMs)
    : mCode(pCode), mModule(pModule), mTimeout(timeoutMs) {
  signal(SIGPIPE, SIG_IGN);
  spawnChild();
}

ScriptBindings::~ScriptBindings() {
  killChild();
  if (mTimeoutId)
    drogon::app().getLoop()->invalidateTimer(*mTimeoutId);
  *mTimeoutShouldRun = false;
}

const std::string &ScriptBindings::getCode() { return mCode; }

std::string ScriptBindings::safeRead(char &cmd) {
  auto safeRead = [this](void *dest, size_t length) {
    size_t bytesRead = 0;
    while (bytesRead < length) {
      auto status =
          read(mInputFd, (char *)dest + bytesRead, length - bytesRead);
      if (status <= 0) {
        if (errno == EPIPE || status == 0) {
          throw ProcessDiedException();
        }
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
  auto safeWrite = [this](const void *data, size_t length) {
    size_t bytesWritten = 0;
    while (bytesWritten < length) {
      auto status =
          write(mOutputFd, (char *)data + bytesWritten, length - bytesWritten);
      if (status == -1) {
        if (errno == EPIPE) {
          throw ProcessDiedException();
        }
        throwError("write()");
      }
      bytesWritten += status;
    }
  };
  safeWrite(&cmd, 1);
  safeWrite(&length, sizeof(size_t));
  safeWrite(buffer, length);
}

void ScriptBindings::spawnChild() {
  LOG_INFO << "[Script] " << mModule << " spawned";
  std::array<int, 2> outputFd, inputFd;
  if (pipe(outputFd.data()) != 0) {
    throwError("pipe()");
  }
  if (pipe(inputFd.data()) != 0) {
    throwError("pipe()");
  }

  auto inputStr = std::to_string(outputFd.at(0));
  auto outputStr = std::to_string(inputFd.at(1));
  setenv("INPUT_FD", inputStr.c_str(), 1);
  setenv("OUTPUT_FD", outputStr.c_str(), 1);
  setenv("SCRIPT_NAME", mModule.c_str(), 1);
  pid_t ppid_before_fork = getpid();
  pid_t pid = fork();
  if (pid == 0) {
    // child
    int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
    if (r == -1) {
      perror("prctl()");
      exit(1);
    }
    if (getppid() != ppid_before_fork)
      exit(1);

    int fdlimit = (int)sysconf(_SC_OPEN_MAX);
    for (int i = STDERR_FILENO + 1; i < fdlimit; i++) {
      if (i != outputFd.at(0) && i != inputFd.at(1))
        [[likely]] { close(i); }
    }
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
      throwError("getcwd()");
    }
    std::string path{cwd};
    path += "/assets/ZeVaScript";
    std::string name{"ZeVaScript: "};
    name.append(mModule);
    execl(path.c_str(), name.c_str(), "", nullptr);
    perror("execl");
    exit(1);
  }
  // parent
  close(outputFd.at(0));
  close(inputFd.at(1));
  mOutputFd = outputFd.at(1);
  mInputFd = inputFd.at(0);
  unsetenv("INPUT_FD");
  unsetenv("OUTPUT_FD");
  unsetenv("SCRIPT_NAME");
  mPid = pid;

  // send code
  safeWrite('C', mCode.c_str(), mCode.size());

  char cmd;
  auto err = safeRead(cmd);
  if (cmd == 'O') {

  } else if (cmd == 'E') {
    throw std::runtime_error(err);
  } else {
    assert(false);
  }
}

void ScriptBindings::killChild() {
  *mTimeoutShouldRun = false;
  if (kill(mPid, SIGTERM)) {
    perror("kill()");
  }
  int status;
  waitpid(mPid, &status, 0);
  if (close(mOutputFd)) {
    perror("close()");
  }
  if (close(mInputFd)) {
    perror("close()");
  }
}

void ScriptBindings::execute(const std::string &pFunctionName,
                             const std::vector<ScriptValue> &params,
                             ScriptCallback &&pCallback,
                             ErrorCallback &&pErrorCallback, int64_t pId) {
  std::unique_lock<std::recursive_mutex> lock(mFdMutex);
  Json::Value msg;
  msg["function"] = pFunctionName;
  Json::Value jsonParams;
  for (const auto &pVal : params) {
    jsonParams.append(scriptValueToJson(pVal));
  }
  msg["params"] = std::move(jsonParams);
  auto str = msg.toStyledString();
  try {
    safeWrite('E', str.c_str(), str.size());
  } catch (ProcessDiedException &e) {
    LOG_WARN << "[Script] " << mModule << " died :c";
    killChild();
    spawnChild();
    return execute(pFunctionName, params, std::move(pCallback),
                   std::move(pErrorCallback));
  } catch (...) {
    std::rethrow_exception(std::current_exception());
  }
  auto id = pId == -1 ? mIdCounter++ : pId;
  mToCallWhenDone.emplace(std::make_tuple(pFunctionName, params,
                                          std::move(pCallback),
                                          std::move(pErrorCallback), id));
  *mTimeoutShouldRun = true;
  mTimeoutId = drogon::app().getLoop()->runAfter(
      mTimeout / 1000.0,
      [id, mTimeoutShouldRun = this->mTimeoutShouldRun, this] {
        if (!mTimeoutShouldRun)
          return;
        std::unique_lock<std::recursive_mutex> lock(mFdMutex);
        if (!mToCallWhenDone.empty() &&
            id == std::get<int64_t>(mToCallWhenDone.front())) {
          LOG_WARN << "[Script] " << mModule << " received a timeout!";
          auto [functionName, params, callback, errorCallback, id] =
              mToCallWhenDone.front();

          std::runtime_error e{"Script timeout!"};
          errorCallback(e);
          mToCallWhenDone.pop();
          restartAndRequeue();
        }
      });
}

void ScriptBindings::checkForNewMessages() {
  std::unique_lock<std::recursive_mutex> lock(mFdMutex);

  struct pollfd poller = {.fd = mInputFd, .events = POLLIN};
  if (!poll(&poller, 1, 0)) {
    return;
  }
  if (mToCallWhenDone.empty())
    return;
  auto [functionName, params, callback, errorCallback, id] =
      mToCallWhenDone.front();

  char cmd;
  std::string response;
  try {
    response = safeRead(cmd);
  } catch (ProcessDiedException &) {
    LOG_WARN << "[Script] " << mModule << " died :c";
    restartAndRequeue();
    return;
  } catch (...) {
    std::rethrow_exception(std::current_exception());
  }
  *mTimeoutShouldRun = false;
  switch (cmd) {
  case 'J': {
    std::stringstream reader{response};
    Json::Value responseJson;
    reader >> responseJson;

    if (responseJson.type() != Json::nullValue) {
      // remove the new line at the end
      response.at(response.size() - 1) = '\0';
      LOG_INFO << "[Script] " << mModule << " return json: " << response;
    }

    callback(responseJson);
    break;
  }
  case 'R': {
    // printf("Received raw string of size: %i\n", (int)length);
    if (isValidAscii(reinterpret_cast<const signed char *>(response.c_str()),
                     response.size())) {
      LOG_INFO << "[Script] " << mModule << " return string: " << response;
    } else {
      LOG_INFO << "[Script] " << mModule << " invalid ascii return";
    }

    callback(response);
    break;
  }
  case 'E': {
    LOG_INFO << "[Script] " << mModule << " error " << response;
    std::runtime_error err{response};
    errorCallback(err);
    break;
  }
  default:
    std::cerr << cmd << "n";
    assert(false);
  }
  mToCallWhenDone.pop();
}

int ScriptBindings::getInputFd(Badge<ScriptManager>) { return mInputFd; }

void ScriptBindings::restartAndRequeue() {
  std::unique_lock<std::recursive_mutex> lock{mFdMutex};
  killChild();
  spawnChild();
  // requeue everything that we are currently waiting for
  auto copy = std::move(mToCallWhenDone);
  mToCallWhenDone = {};
  while (!copy.empty()) {
    auto [functionName, params, callback, errorCallback, id] = copy.front();
    execute(functionName, params, std::move(callback), std::move(errorCallback),
            id);
    copy.pop();
  }
}
void ScriptBindings::setTimeout(Badge<struct ScriptManager>,
                                uint32_t pTimeout) {
  // this won't affect any current scripts, but who cares? :)
  std::unique_lock<std::recursive_mutex> lock{mFdMutex};
  mTimeout = pTimeout;
}
