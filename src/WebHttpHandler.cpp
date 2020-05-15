#include "WebHttpHandler.hpp"
#include "common.hpp"
#include <seasocks/Request.h>
#include <seasocks/ResponseWriter.h>
#include <seasocks/ResponseBuilder.h>
#include <fstream>
#include <Logger.hpp>
#include <seasocks/util/CrackedUri.h>
#include <filesystem>

std::shared_ptr<seasocks::Response> WebHttpHandler::handle(const seasocks::Request &request) {
    const auto& path = request.getRequestUri();
    if(path.find("..") != std::string::npos) {
        return seasocks::Response::error(seasocks::ResponseCode::BadRequest, "Requested an invalid path!");
    }
    seasocks::CrackedUri uri{path};
    if(uri.path().at(0) == "api") {
        ASSERT_NOT_REACHED();
    }
    if(uri.path().at(0) == "html") {
        auto filepath = path.substr(1);
        if(std::filesystem::exists(filepath)) {
            std::ifstream t(filepath);
            std::string str((std::istreambuf_iterator<char>(t)),
                            std::istreambuf_iterator<char>());
            std::filesystem::path filepath{path};
            seasocks::ResponseBuilder responseBuilder;
            if(filepath.extension() == ".css") {
                responseBuilder.withContentType("text/css");
            } else if(filepath.extension() == ".html") {
                responseBuilder.withContentType("text/html");
            } else if(filepath.extension() == ".js") {
                responseBuilder.withContentType("application/javascript");
            }
            responseBuilder << str;
            return responseBuilder.build();
        }
    }
    return seasocks::Response::error(seasocks::ResponseCode::NotFound, "File not found :c");
}
