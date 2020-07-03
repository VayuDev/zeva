#include "Hub.hpp"
#include "DrogonUtil.hpp"
#include "Util.hpp"
#include <filesystem>

void Api::Hub::getRandomWallpaper(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  // check how old the client image is
  const auto &ifModifiedSinceHeader = req->getHeader("if-modified-since");
  if (!ifModifiedSinceHeader.empty()) {
    auto imageFrom = drogon::utils::getHttpDate(ifModifiedSinceHeader);
    if (imageFrom >= trantor::Date::now().after(-180)) {
      // cached response
      auto resp = drogon::HttpResponse::newHttpResponse();
      resp->setStatusCode(drogon::k304NotModified);
      callback(resp);
      return;
    }
  }

  std::filesystem::directory_iterator dir{"html/wallpapers"};
  srand(time(nullptr));
  std::vector<std::string> filenames;
  for (const auto &image : dir) {
    if (image.is_regular_file()) {
      filenames.push_back(image.path());
    }
  }
  if (filenames.empty()) {
    callback(genError("No wallpapers!"));
    return;
  }

  auto file = filenames.at(rand() % filenames.size());
  auto resp = drogon::HttpResponse::newFileResponse(file);
  // resp->addHeader("expires",drogon::utils::getHttpFullDate(trantor::Date::now().after(180000)));
  resp->addHeader("last-modified",
                  drogon::utils::getHttpFullDate(trantor::Date::now()));
  resp->addHeader("cache-control", "public, max-age=604800, immutable");
  callback(resp);
}
