#include "Player.hpp"
#include "DrogonUtil.hpp"

Api::Apps::Player::Player() {
  mTimerId = drogon::app().getLoop()->runEvery(0.01, [this] {
    try {
      mPlayer.poll();
    } catch (...) {
    }
  });
}
Api::Apps::Player::~Player() {
  drogon::app().getLoop()->invalidateTimer(mTimerId);
}

void Api::Apps::Player::getStatus(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  std::shared_lock<std::shared_mutex> lock(mLastLsMutex);
  Json::Value resp;
  resp["path"] = mLastLs;
  {
    Json::Value queue;
    for (const auto &song : mPlayer.getPlaylist()) {
      queue.append(song);
    }
    resp["queue"] = std::move(queue);
  }
  resp["queueIndex"] = mPlayer.getCurrentSongIndex();
  callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
}
void Api::Apps::Player::getLs(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&pPath) {
  while (pPath.find("./") == 0) {
    pPath = pPath.substr(2);
  }
  Json::Value resp = Json::arrayValue;
  try {
    auto vec = mPlayer.ls(pPath);
    for (const auto &str : vec) {
      Json::Value file;
      file["name"] = pPath + "/" + str.name;
      file["size"] = str.fileSize;
      file["directory"] = str.isDirectory;
      file["musicfile"] = isMusicFile(str.name);
      resp.append(std::move(file));
    }
    // update last ls
    {
      std::lock_guard<std::shared_mutex> lock(mLastLsMutex);
      mLastLs = pPath;
    }
    callback(drogon::HttpResponse::newHttpJsonResponse(std::move(resp)));
  } catch (std::exception &e) {
    callback(genError(e.what()));
  }
}

void Api::Apps::Player::setQueue(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&pQueueJson, int64_t pStartindex) {
  try {
    std::stringstream inputStream{pQueueJson};
    Json::Value val;
    inputStream >> val;
    std::vector<std::string> songs;
    for (auto &song : val) {
      if (!song.isString()) {
        callback(genError("Please pass an array of strings!"));
        return;
      }
      songs.emplace_back(song.asString());
    }
    if (pStartindex < 0 || pStartindex >= songs.size()) {
      callback(genError("Invalid startindex!"));
      return;
    }
    mPlayer.setPlaylist(std::move(songs));
    mPlayer.playSong(pStartindex);
    callback(genResponse("ok"));
  } catch (std::exception &e) {
    callback(genError(e.what()));
  }
}
void Api::Apps::Player::resume(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    mPlayer.resume();
    callback(genResponse("ok"));
  } catch (std::exception &e) {
    callback(genError(e.what()));
  }
}
void Api::Apps::Player::pause(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    mPlayer.pause();
    callback(genResponse("ok"));
  } catch (std::exception &e) {
    callback(genError(e.what()));
  }
}
void Api::Apps::Player::getDuration(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string &&pSongname) {
  auto currentlyPlayingSong = mPlayer.getCurrentSong();
  if (!currentlyPlayingSong) {
    callback(genError("No song currently playing"));
    return;
  }
  if (*currentlyPlayingSong != pSongname) {
    callback(genError("This song isn't currently being played"));
    return;
  }
  mPlayer.callWhenDurationIsAvailable(
      [callback = std::move(callback)](auto position, auto duration) mutable {
        Json::Value response;
        response["position"] = position;
        response["duration"] = duration;
        callback(
            drogon::HttpResponse::newHttpJsonResponse(std::move(response)));
      });
}
void Api::Apps::Player::next(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  mPlayer.playNextSong();
  callback(genResponse("ok"));
}
void Api::Apps::Player::prev(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  mPlayer.playPrevSong();
  callback(genResponse("ok"));
}
