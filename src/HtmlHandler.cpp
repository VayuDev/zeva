#include "HtmlHandler.hpp"
#include <fstream>
#include <filesystem>
#include <seasocks/ResponseBuilder.h>

std::shared_ptr<seasocks::Response> HtmlHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {
    if(pUrl.path().at(0) == "html") {
        std::string filename;
        for (const auto &e: pUrl.path()) {
            filename += e + "/";
        }
        filename = filename.substr(0, filename.length() - 1);
        if (std::filesystem::exists(filename)) {
            std::ifstream t(filename);
            std::string str((std::istreambuf_iterator<char>(t)),
                            std::istreambuf_iterator<char>());
            std::filesystem::path filepath{filename};
            seasocks::ResponseBuilder responseBuilder;
            if (filepath.extension() == ".css") {
                responseBuilder.withContentType("text/css");
            } else if (filepath.extension() == ".html") {
                responseBuilder.withContentType("text/html");
            } else if (filepath.extension() == ".js") {
                responseBuilder.withContentType("application/javascript");
            }
            responseBuilder << str;
            return responseBuilder.build();
        }
    }
    return nullptr;
}
