#include "WallpaperDownloader.hpp"
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpClient.h>
#include <filesystem>
#include <fstream>

namespace WallpaperDownloader {
const char *REDDIT_USER_AGENT = "ZeVa Wallpaper Bot";

void downloadWallpapers() {
  auto client = drogon::HttpClient::newHttpClient("https://www.reddit.com");
  auto req = drogon::HttpRequest::newHttpRequest();

  req->addHeader("User-Agent", REDDIT_USER_AGENT);
  req->setPath("/r/wallpapers/hot.json?count=10");
  req->addHeader("Accept-Language", "en-US,en;q=0.5");
  client->sendRequest(req, [](drogon::ReqResult result,
                              const drogon::HttpResponsePtr &resp) {
    if (result != drogon::ReqResult::Ok) {
      LOG_WARN << "Wallpapers: Reddit request returned with error, is the "
                  "internet working?";
      return;
    }
    auto &json = *resp->getJsonObject();
    json = json["data"]["children"];
    LOG_INFO << "Wallpapers: Successfully got the wallpaper list!";
    if (json.empty()) {
      LOG_WARN << "Wallpapers: No qualified wallpapers found";
      return;
    }
    // delete all files from the directory if it exists
    if (std::filesystem::exists("html/wallpapers")) {
      std::filesystem::directory_iterator dir{"html/wallpapers"};
      for (const auto &file : dir) {
        if (file.is_regular_file()) {
          std::filesystem::remove(file.path());
        }
      }
    } else {
      // or create the directory otherwise
      std::filesystem::create_directories("html/wallpapers");
    }

    for (const auto &post : json) {
      if (post["data"]["ups"].asInt64() >= 100) {
        const auto &urlStr = post["data"]["url"].asString();
        LOG_INFO << "Wallpapers: Fetching: " << urlStr;
        auto req = drogon::HttpRequest::newHttpRequest();

        try {
          // ugly way to get something like https://i.reddit.it from a full
          // request url this will fail horribly if the string isn't formatted
          // correctly that's why we have the try-catch
          auto thirdSlashIndex =
              urlStr.find('/', urlStr.find('/', urlStr.find('/') + 1) + 1);
          auto hostname = urlStr.substr(0, thirdSlashIndex);
          auto client = drogon::HttpClient::newHttpClient(hostname);
          req->setPath(urlStr.substr(thirdSlashIndex));
          req->addHeader("Accept-Language", "en-US,en;q=0.5");
          req->addHeader("User-Agent", REDDIT_USER_AGENT);
          client->sendRequest(req, [urlStr](
                                       drogon::ReqResult result,
                                       const drogon::HttpResponsePtr &resp) {
            if (result != drogon::ReqResult::Ok) {
              LOG_WARN << "Wallpapers: Failed to request " << urlStr;
              return;
            }
            auto filename = urlStr.substr(urlStr.find_last_of('/') + 1);
            std::ofstream out{"html/wallpapers/" + filename, std::ios::binary};
            if (!out.is_open()) {
              LOG_ERROR << "Failed to open " << ("html/wallpapers/" + filename);
              return;
            }
            out.write(resp->getBody().c_str(), resp->getBody().size());
          });
        } catch (std::exception &e) {
          LOG_WARN << "Wallpapers: " << e.what();
        }
      }
    }
  });
}
} // namespace WallpaperDownloader