#include "ScriptManager.hpp"

#include <chrono>
#include <csignal>
#include <drogon/HttpAppFramework.h>
#include <poll.h>
#include <utility>

void ScriptManager::addScript(const std::string &pName,
                              const std::string &pCode,
                              bool pCheckIfCodeChanged) {
  std::lock_guard<std::shared_mutex> lock{mScriptsMutex};
  if (mScripts.count(pName) > 0) {
    if (pCheckIfCodeChanged && mScripts.at(pName).getCode() == pCode)
      return;
    mScripts.erase(pName);
  }
  if (pCheckIfCodeChanged) {
    LOG_INFO << "Script was changed by someone else!";
  }

  mScripts.emplace(std::piecewise_construct, std::forward_as_tuple(pName),
                   std::forward_as_tuple(pName, pCode));
}

void ScriptManager::deleteScript(const std::string &pName) {
  std::lock_guard<std::shared_mutex> lock{mScriptsMutex};
  if (mScripts.count(pName) > 0) {
    mScripts.erase(pName);
  }
}

void ScriptManager::onTableChanged(const std::string &pTable,
                                   const std::string &pType) {
  std::shared_lock<std::shared_mutex> lock{mScriptsMutex};
  if (pTable == "scripts") {
    auto result = drogon::app().getDbClient()->execSqlSync(
        "SELECT name,code FROM scripts");

    lock.unlock();
    for (const auto &row : result) {
      try {
        addScript(row["name"].as<std::string>(), row["code"].as<std::string>(),
                  true);
      } catch (std::exception &e) {
        LOG_ERROR << "Error adding script " << e.what();
      }
    }
    lock.lock();
  }
  // running a log scripts would end in a endless loop of logging
  if (pTable != "log") {
    for (auto &script : mScripts) {
      try {
        script.second.execute(
            "onTableChanged",
            {ScriptValue::makeString(pTable), ScriptValue::makeString(pType)},
            IGNORE_SCRIPTCALLBACK, IGNORE_ERRORCALLBACK);
      } catch (std::exception &e) {
        LOG_ERROR << "Error executing script: " << e.what();
      }
    }
  }
}

void ScriptManager::onAudioEvent(const std::string &pType,
                                 const std::optional<std::string> &pData,
                                 std::optional<int64_t> pData2) {
  std::shared_lock<std::shared_mutex> lock{mScriptsMutex};
  for (auto &script : mScripts) {
    try {
      script.second.execute(
          "onAudio",
          {ScriptValue::makeString(pType),
           pData ? ScriptValue::makeString(*pData) : ScriptValue::makeNull(),
           pData2 ? ScriptValue::makeInt(*pData2) : ScriptValue::makeNull()},
          IGNORE_SCRIPTCALLBACK, IGNORE_ERRORCALLBACK);
    } catch (std::exception &e) {
      LOG_ERROR << "Error executing script: " << e.what();
    }
  }
}

void ScriptManager::executeScriptWithCallback(
    const std::string &pName, const std::string &pFunction,
    const std::vector<ScriptValue> &pParamSetter, ScriptCallback &&pCallback,
    ErrorCallback &&pErrorCallback) {
  std::shared_lock<std::shared_mutex> lock{mScriptsMutex};
  auto cpy = pErrorCallback;
  try {
    mScripts.at(pName).execute(pFunction, pParamSetter, std::move(pCallback),
                               std::move(pErrorCallback));
  } catch (std::exception &e) {
    cpy(e);
  }
}

ScriptManager::~ScriptManager() {
  mShouldRun = false;
  mScriptReturnCallbackThread.join();
}

ScriptManager::ScriptManager()
    : mScriptReturnCallbackThread([this] {
        while (mShouldRun) {
          std::shared_lock<std::shared_mutex> lock{mScriptsMutex};

          if (mScripts.empty()) {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
          }

          struct pollfd fds[mScripts.size()];
          size_t i = 0;
          for (auto &script : mScripts) {
            fds[i].fd = script.second.getInputFd();
            fds[i].events = POLLIN;
            i++;
          }
          lock.unlock();
          auto status = poll(fds, mScripts.size(), 1000);
          if (status < 0) {
            perror("poll()");
          }
          // some new data became available
          if (status > 0) {
            lock.lock();
            for (auto &script : mScripts) {
              script.second.checkForNewMessages();
            }
            lock.unlock();
          }
        }
      }) {
  pthread_setname_np(mScriptReturnCallbackThread.native_handle(),
                     "ScriptManager");
}
