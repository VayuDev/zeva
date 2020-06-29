#pragma once
#include "ScriptBindings.hpp"
#include <future>
#include <map>
#include <shared_mutex>
#include <thread>
#include <variant>

class ScriptManager final {
public:
  static ScriptManager &the() {
    static ScriptManager scriptManager;
    return scriptManager;
  }
  ScriptManager();
  ~ScriptManager();
  void addScript(const std::string &pName, const std::string &pCode, uint32_t pTimeout,
                 bool pCheckIfCodeChanged = false);
  void deleteScript(const std::string &pName);
  void executeScriptWithCallback(const std::string &pName,
                                 const std::string &pFunction,
                                 const std::vector<ScriptValue> &pParamSetter,
                                 ScriptCallback &&pCallback,
                                 ErrorCallback &&pErrorCallback);
  // Should be called when a table changes. Returns (almost) immediately.
  void onTableChanged(const std::string &pTable, const std::string &pType);

  // Should be called when an audio event happens (like skipping a song).
  // Returns (almost) immediately.
  void onAudioEvent(const std::string &pType,
                    const std::optional<std::string> &pData = {},
                    std::optional<int64_t> pData2 = {});

private:
  std::map<std::string, ScriptBindings> mScripts;
  std::shared_mutex mScriptsMutex;

  std::atomic<bool> mShouldRun = true;
  std::thread mScriptReturnCallbackThread;
};