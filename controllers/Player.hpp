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
  METHOD_LIST_END

  void
  getStatus(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void getLs(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
             std::string &&pPath);

private:
  MusicPlayer mPlayer;
};

} // namespace Api::Apps