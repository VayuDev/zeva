#include "Hub.hpp"
#include <filesystem>
#include "Util.hpp"

void Api::Hub::getRandomWallpaper(const drogon::HttpRequestPtr&,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    std::filesystem::directory_iterator dir{"html/wallpapers"};
    srand(time(0));
    std::vector<std::string> filenames;
    for(const auto& image: dir) {
        if(image.is_regular_file()) {
            filenames.push_back(image.path());
        }
    }
    if(filenames.size() == 0) {
        callback(genError("No wallpapers!"));
    }
    auto file = filenames.at(rand()%filenames.size());
    auto resp = drogon::HttpResponse::newFileResponse(file);
    resp->addHeader("Cache-Control", "max-age=0, no-cache, must-revalidate, proxy-revalidate");
    callback(resp);
}
