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
  ScriptBindings(const std::string &pModule, const std::string &pCode,
                 uint32_t pTimeout);
  ~ScriptBindings();

  // delete move and copy constructors
  ScriptBindings(const ScriptBindings &) = delete;
  ScriptBindings(ScriptBindings &&) = delete;
  void operator=(const ScriptBindings &) = delete;
  void operator=(const ScriptBindings &&) = delete;

  void execute(const std::string &pFunctionName,
               const std::vector<ScriptValue> &, ScriptCallback &&pCallback,
               ErrorCallback &&pErrorCallback, int64_t pId = -1);
  // std::future<ScriptBindingsReturn> execute(const std::string& pFunctionName,
  // const std::vector<ScriptValue>&, size_t pDepth = 0);
  void checkForNewMessages();
  const std::string &getCode();
  int getInputFd(Badge<class ScriptManager> = {});

  [[nodiscard]] inline uint32_t getTimeout() const { return mTimeout; }
  void setTimeout(Badge<class ScriptManager>, uint32_t pTimeout);

private:
  int mInputFd, mOutputFd;
  pid_t mPid;
  std::string mCode, mModule;
  std::recursive_mutex mFdMutex;
  std::string safeRead(char &cmd);
  void safeWrite(char cmd, const void *buffer, size_t length);
  void killChild();
  void spawnChild();
  int64_t mIdCounter = 0;
  uint32_t mTimeout;
  std::optional<unsigned long> mTimeoutId = 0;
  std::shared_ptr<std::atomic<bool>> mTimeoutShouldRun =
      std::make_shared<std::atomic<bool>>(false);

  void restartAndRequeue();

  std::queue<std::tuple<std::string, std::vector<ScriptValue>, ScriptCallback,
                        ErrorCallback, int64_t>>
      mToCallWhenDone;
};
