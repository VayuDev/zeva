#include "ScriptManager.hpp"
#include "wren.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include "DatabaseHelper.hpp"
#include "LogWebsocket.hpp"
#include "PostgreSQLDatabase.hpp"
#include <drogon/HttpAppFramework.h>

#include "DrogonUtil.hpp"
#include "Util.hpp"
#include "WallpaperDownloader.hpp"
#include <csignal>
#include <drogon/HttpClient.h>
#include <regex>

#include "MusicPlayer.hpp"

static void sighandler(int) { drogon::app().quit(); }

int main() {
  // init database
  auto conn = std::make_shared<PostgreSQLDatabase>();
  DatabaseHelper::createDb(*conn);

  srand(time(0));
  // event trigger
  conn->addListener("on_table_changed", [](const std::string &pPayload) {
    auto splitterLocation = pPayload.find('%');
    if (splitterLocation == std::string::npos) {
      LOG_ERROR << "Database notify isn't formatted correctly!" << pPayload;
      ;
    } else {
      ScriptManager::the().onTableChanged(
          pPayload.substr(0, splitterLocation),
          pPayload.substr(splitterLocation + 1,
                          pPayload.length() - splitterLocation - 1));
    }
  });

  auto logWebsocketController = std::make_shared<LogWebsocket>();
  conn->addListener("newlog", [conn, logWebsocketController](
                                  const std::string &payload) {
    auto percentIndex = payload.find('%');
    auto eventType = payload.substr(0, percentIndex);
    if (eventType == "INSERT") {
      auto id = std::stoll(payload.substr(percentIndex + 1));
      try {
        auto resp = conn->query("SELECT level,msg FROM log WHERE id=$1",
                                {QueryValue::makeInt(id)});
        if (!resp)
          return;
        logWebsocketController->newLogMessage(resp->getValue(0, 0).intValue,
                                              resp->getValue(0, 1).stringValue);
      } catch (std::exception &e) {
        std::cerr << "Error requesting log: " << e.what() << "\n";
      }
    } else {
      logWebsocketController->init();
    }
  });

  long passedTime = std::numeric_limits<long>::max();
  // handle notifications
  drogon::app().getLoop()->runEvery(0.1, [conn, &passedTime]() mutable {
    conn->awaitNotifications(1);
    // delete the old log about every hour
    if (passedTime++ > 60 * 600) {
      try {
        conn->query("DELETE FROM log WHERE created < (CURRENT_TIMESTAMP - '7 "
                    "days' :: interval)");
      } catch (std::exception &e) {
        LOG_ERROR << "Error awaiting notification: " << e.what();
      }

      passedTime = 0;
    }
  });

  // setup log
  std::regex levelFinder{R"([a-zA-Z]+)"};
  trantor::Logger::setOutputFunction(
      [levelFinder = std::move(levelFinder), conn,
       logWebsocketController](const char *str, uint64_t len) {
        if (isValidAscii(reinterpret_cast<const signed char *>(str), len)) {
          bool shortened = false;
          if (len > 500) {
            len = 500;
            shortened = true;
          }
          // remove the newline
          std::string msg{str, len - 1};
          if (shortened) {
            msg += "...";
          }
          std::cout << msg << "\n";

          if (drogon::app().isRunning()) {

            std::smatch levelMatch;
            int64_t level = trantor::Logger::kInfo;
            if (std::regex_search(msg, levelMatch, levelFinder)) {
              std::string restString = levelMatch.suffix();
              if (std::regex_search(restString, levelMatch, levelFinder)) {
                const std::string &logLevel = levelMatch[0];
                level = stringToLogLevel(logLevel);
              }
            }

            drogon::app().getDbClient()->execSqlAsync(
                "INSERT INTO log (level, created, msg) VALUES ($1, "
                "CURRENT_TIMESTAMP, $2)",
                [](const drogon::orm::Result &) {},
                [](const drogon::orm::DrogonDbException &) {}, level,
                std::move(msg));
            /*conn->query("INSERT INTO log (level, created, msg) VALUES
               ($1::bigint, CURRENT_TIMESTAMP, $2::text)",
                        {QueryValue::makeInt(level),
               QueryValue::makeString(std::move(msg))});*/
          }
        } else {
          std::cout << "INVALID ASCII\n";
        }
      },
      []() { std::cout << std::flush; });

  // setup webserver
  signal(SIGTERM, sighandler);
  signal(SIGINT, sighandler);
  signal(SIGABRT, sighandler);
  drogon::app().addListener("0.0.0.0", 8080);
  drogon::app().loadConfigFile(getConfigFileLocation());
  drogon::app().setLogLevel(trantor::Logger::LogLevel::kDebug);
  drogon::app().registerController(logWebsocketController);

  auto resp404 = drogon::HttpResponse::newHttpResponse();
  resp404->setStatusCode(drogon::HttpStatusCode::k404NotFound);
  resp404->setBody("Not Implemented");
  drogon::app().setCustom404Page(resp404);

  drogon::app().setCustomErrorHandler([](drogon::HttpStatusCode pStatus) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(pStatus);
    resp->setBody(std::to_string((int)pStatus));
    return resp;
  });

  drogon::app().registerHandler(
      "/", [](const drogon::HttpRequestPtr &,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        callback(drogon::HttpResponse::newRedirectionResponse("/hub/hub.html"));
      });

  // load scripts
  drogon::app().getLoop()->queueInLoop([logWebsocketController] {
    auto client = std::make_shared<PostgreSQLDatabase>();
    auto res = client->query("SELECT name,code FROM scripts");
    for (size_t i = 0; i < res->getRowCount(); ++i) {
      auto name = res->getValue(i, 0).stringValue;
      auto code = res->getValue(i, 1).stringValue;
      try {
        ScriptManager::the().addScript(name, code);
      } catch (std::exception &e) {
        LOG_ERROR << "Failed to load script '" << name
                  << "' with exception: " << e.what();
      }
    }
    logWebsocketController->init();
    LOG_WARN << "Server completely started";
  });
  drogon::app().getLoop()->runAfter(1, [] {
    WallpaperDownloader::downloadWallpapers();
    drogon::app().getLoop()->runEvery(
        60 * 10, [] { WallpaperDownloader::downloadWallpapers(); });
  });
  drogon::app().run();
  LOG_WARN << "Quitting server";
}
