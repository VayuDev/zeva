#pragma once

#include "MusicPlayer.hpp"
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/drogon.h>

namespace Api::Apps {

class Player : public drogon::HttpController<Player> {
public:
  METHOD_LIST_BEGIN
  METHOD_ADD(Player::getStatus, "status", drogon::Get);
  METHOD_ADD(Player::getLs, "ls?path={}", drogon::Get);
  METHOD_ADD(Player::setQueue, "setQueue?queue={}&startIndex={}", drogon::Post);
  METHOD_ADD(Player::resume, "resume", drogon::Post);
  METHOD_ADD(Player::pause, "pause", drogon::Post);
  METHOD_LIST_END

  Player();
  ~Player() override;

  void
  getStatus(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void getLs(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
             std::string &&pPath);
  void setQueue(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                std::string &&pQueueJson, int64_t pStartindex);
  void resume(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void pause(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback);

private:
  MusicPlayer mPlayer;
  trantor::TimerId mTimerId;
};

} // namespace Api::Apps