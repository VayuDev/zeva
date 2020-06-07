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
  void addScript(const std::string &pName, const std::string &pCode,
                 bool pCheckIfCodeChanged = false);
  void deleteScript(const std::string &pName);
  void executeScriptWithCallback(const std::string &pName,
                                 const std::string &pFunction,
                                 const std::vector<ScriptValue> &pParamSetter,
                                 ScriptCallback &&pCallback,
                                 ErrorCallback &&pErrorCallback);
  void onTableChanged(const std::string &pTable, const std::string &pType);

private:
  std::map<std::string, ScriptBindings> mScripts;
  std::shared_mutex mScriptsMutex;

  std::atomic<bool> mShouldRun = true;
  std::thread mScriptReturnCallbackThread;
};