#include "Player.hpp"
#include "DrogonUtil.hpp"

void Api::Apps::Player::getStatus(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  Json::Value resp;
  resp["path"] = ".";
  callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
}
void Api::Apps::Player::getLs(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&pPath) {
  Json::Value resp = Json::arrayValue;
  try {
    auto vec = mPlayer.ls(pPath);
    for(const auto& str: vec) {
      Json::Value file;
      file["name"] = str.name;
      file["size"] = str.fileSize;
      file["directory"] = str.isDirectory;
      resp.append(std::move(file));
    }
    callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
  } catch(std::exception& e) {
    callback(genError(e.what()));
  }
}
