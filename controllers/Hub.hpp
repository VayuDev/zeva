#pragma once
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>

namespace Api {
class Hub : public drogon::HttpController<Hub> {
public:
    METHOD_LIST_BEGIN
    METHOD_ADD(Hub::getRandomWallpaper, "randomWallpaper", drogon::Get);
    METHOD_LIST_END

    void getRandomWallpaper(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
}