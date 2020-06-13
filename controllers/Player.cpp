#include "Player.hpp"
#include "DrogonUtil.hpp"

Api::Apps::Player::Player() {
  mTimerId = drogon::app().getLoop()->runEvery(0.01, [this] {
    try {
      mPlayer.poll();
    } catch(...) {

    }
  });
}
Api::Apps::Player::~Player() {
  drogon::app().getLoop()->invalidateTimer(mTimerId);
}


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
    for (const auto &str : vec) {
      Json::Value file;
      file["name"] = str.name;
      file["size"] = str.fileSize;
      file["directory"] = str.isDirectory;
      file["musicfile"] = isMusicFile(str.name);
      resp.append(std::move(file));
    }
    callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
  } catch (std::exception &e) {
    callback(genError(e.what()));
  }
}
void Api::Apps::Player::setQueue(const drogon::HttpRequestPtr &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 std::string &&pQueueJson,
                                 int64_t pStartindex) {

  std::stringstream inputStream{pQueueJson};
  Json::Value val;
  inputStream >> val;
  std::vector<std::string> songs;
  for(auto& song: val) {
    if(!song.isString()) {
      callback(genError("Please pass an array of strings!"));
      return;
    }
    songs.emplace_back(song.asString());
  }
  if(pStartindex < 0 || pStartindex >= songs.size()) {
    callback(genError("Invalid startindex!"));
    return;
  }
  mPlayer.setPlaylist(std::move(songs));
  mPlayer.playSong(pStartindex);
  callback(genResponse("ok"));
}