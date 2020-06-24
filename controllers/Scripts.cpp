#include "Scripts.hpp"
#include "DrogonUtil.hpp"
#include "ScriptManager.hpp"
#include "Util.hpp"

void Api::Scripts::getAllScripts(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  drogon::app().getDbClient()->execSqlAsync(
      "SELECT * FROM scripts ORDER BY name",
      [callback = std::move(callback)](const drogon::orm::Result &r) {
        Json::Value ret;
        for (const auto &row : r) {
          Json::Value jsonRow;
          jsonRow["id"] = row["id"].as<std::string>();
          jsonRow["name"] = row["name"].as<std::string>();
          ret.append(std::move(jsonRow));
        }
        auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(ret));
        callback(resp);
      },
      genErrorHandler(callback));
}

void Api::Scripts::getScript(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int64_t scriptid) {
  drogon::app().getDbClient()->execSqlAsync(
      "SELECT * FROM scripts WHERE id=$1",
      [callback = std::move(callback)](const drogon::orm::Result &r) {
        Json::Value jsonRow;
        jsonRow["id"] = r.at(0)["id"].as<std::string>();
        jsonRow["name"] = r.at(0)["name"].as<std::string>();
        jsonRow["code"] = r.at(0)["code"].as<std::string>();

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(std::move(jsonRow));
        callback(resp);
      },
      genErrorHandler(callback), scriptid);
}

void Api::Scripts::updateScript(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int64_t scriptid, std::string &&pCode) {
  drogon::app().getDbClient()->execSqlAsync(
      "SELECT name FROM scripts WHERE id=$1",
      [callback = std::move(callback), pCode = std::move(pCode),
       scriptid](const drogon::orm::Result &r) mutable {
        auto name = r.at(0)["name"].as<std::string>();
        drogon::HttpResponsePtr response;
        try {
          ScriptManager::the().addScript(name, pCode);
          response = drogon::HttpResponse::newHttpResponse();
        } catch (std::exception &e) {
          response = genError(e.what());
        }
        drogon::app().getDbClient()->execSqlAsync(
            "UPDATE scripts SET code=$1 WHERE id=$2",
            [callback = std::move(callback), response = std::move(response)](
                const drogon::orm::Result &) { callback(response); },
            genErrorHandler(callback), std::move(pCode), scriptid);
      },
      genErrorHandler(callback), scriptid);
}

void Api::Scripts::runScript(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int64_t scriptid, std::string &&pParam) {
  drogon::app().getDbClient()->execSqlAsync(
      "SELECT name FROM scripts WHERE id=$1",
      [callback = std::move(callback),
       param = std::move(pParam)](const drogon::orm::Result &r) mutable {
        auto name = r.at(0)["name"].as<std::string>();
        ScriptValue scriptParameter;
        if (!param.empty()) {
          try {
            scriptParameter = ScriptValue::makeInt(std::stoll(param));
          } catch (...) {
            scriptParameter = ScriptValue::makeString(param);
          }
        }
        auto callbackCpy = callback;
        ScriptManager::the().executeScriptWithCallback(
            name, "onRunOnce", {std::move(scriptParameter)},
            [callback = std::move(callback)](ScriptBindingsReturn &&ret) {
              auto returnString = std::get_if<std::string>(&ret);

              Json::Value json;
              json["type"] = "ok";
              if (returnString) {
                if (isValidAscii(reinterpret_cast<const signed char *>(
                                     returnString->c_str()),
                                 returnString->size())) {
                  json["return"] = *returnString;
                } else {
                  json["return"] = "(invalid ascii)";
                }
              } else {
                json["return"] = std::move(std::get<Json::Value>(ret));
              }
              callback(
                  drogon::HttpResponse::newHttpJsonResponse(std::move(json)));
            },
            genDefErrorHandler(callbackCpy));
      },
      genErrorHandler(callback), scriptid);
}

void Api::Scripts::drawScript(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int64_t scriptid) {
  auto callbackCpy = callback;
  drogon::app().getDbClient()->execSqlAsync(
      "SELECT name FROM scripts WHERE id=$1",
      [callback = std::move(callback)](const drogon::orm::Result &r) mutable {
        auto name = r.at(0)["name"].as<std::string>();
        ScriptManager::the().executeScriptWithCallback(
            name, "drawImage", {},
            [callback = std::move(callback)](ScriptBindingsReturn &&ret) {
              auto string = std::get_if<std::string>(&ret);
              if (string) {
                auto resp = genResponse(*string);
                resp->setContentTypeCode(drogon::CT_IMAGE_PNG);
                callback(resp);
              } else {
                callback(genError("Script didn't return a string!"));
              }
            },
            genDefErrorHandler(callback));
      },
      genErrorHandler(callbackCpy), scriptid);
}

void Api::Scripts::deleteScript(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int64_t scriptid) {
  drogon::app().getDbClient()->execSqlAsync(
      "DELETE FROM scripts WHERE id=$1 RETURNING name",
      [callback = std::move(callback)](const drogon::orm::Result &r) {
        auto name = r.at(0)["name"].as<std::string>();
        ScriptManager::the().deleteScript(name);
        LOG_INFO << "[Script] " << name << " deleted";
        callback(genResponse("Deleted script"));
      },
      genErrorHandler(callback), scriptid);
}

void Api::Scripts::createScript(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&pName) {
  const auto libname = pName + ".wren";
  std::filesystem::directory_iterator wrenLibs("assets/wren_libs");
  for (const auto &file : wrenLibs) {
    if (file.is_regular_file() && file.path().extension() == ".wren") {
      if (file.path().filename() == libname) {
        callback(
            genError("Name conflict: A library with this name already exists"));
        return;
      }
    }
  }
  auto code = readWholeFile("assets/template.wren");
  code.replace(code.find("$ScriptModule$"), strlen("$ScriptModule$"),
               getScriptClassNameFromScriptName(pName));
  std::string name = pName;
  drogon::app().getDbClient()->execSqlAsync(
      "INSERT INTO scripts (name, code) VALUES ($1, $2) RETURNING id",
      [callback = std::move(callback), name,
       code](const drogon::orm::Result &r) {
        try {
          ScriptManager::the().addScript(name, code);
          Json::Value response;
          response["name"] = name;
          response["id"] = r.at(0)["id"].as<int64_t>();
          callback(
              drogon::HttpResponse::newHttpJsonResponse(std::move(response)));
        } catch (std::exception &e) {
          callback(genError(e.what()));
        }
      },
      genErrorHandler(callback), pName, code);
}
