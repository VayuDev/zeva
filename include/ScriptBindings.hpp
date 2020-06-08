#pragma once
#include "ScriptValue.hpp"
#include "Util.hpp"
#include <array>
#include <json/json.h>
#include <string>
#include <variant>

class ProcessDiedException : public std::exception {};

using ScriptCallback = std::function<void(ScriptBindingsReturn &&)>;
using ErrorCallback = std::function<void(std::exception &)>;

inline void IGNORE_SCRIPTCALLBACK(ScriptBindingsReturn &&) {}
inline void IGNORE_ERRORCALLBACK(std::exception &) {}

class ScriptBindings final {
public:
  ScriptBindings(const std::string &pModule, const std::string &pCode);
  ~ScriptBindings();

  // delete move and copy constructors
  ScriptBindings(const ScriptBindings &) = delete;
  ScriptBindings(ScriptBindings &&) = delete;
  void operator=(const ScriptBindings &) = delete;
  void operator=(const ScriptBindings &&) = delete;

  void execute(const std::string &pFunctionName,
               const std::vector<ScriptValue> &, ScriptCallback &&pCallback,
               ErrorCallback &&pErrorCallback);
  // std::future<ScriptBindingsReturn> execute(const std::string& pFunctionName,
  // const std::vector<ScriptValue>&, size_t pDepth = 0);
  void checkForNewMessages();
  const std::string &getCode();

  int getInputFd(Badge<class ScriptManager> = {});

private:
  int mInputFd, mOutputFd;
  pid_t mPid;
  std::string mCode, mModule;
  std::recursive_mutex mFdMutex;
  std::string safeRead(char &cmd);
  void safeWrite(char cmd, const void *buffer, size_t length);
  void killChild();
  void spawnChild();

  std::queue<std::tuple<std::string, std::vector<ScriptValue>, ScriptCallback,
                        ErrorCallback>>
      mToCallWhenDone;
};
