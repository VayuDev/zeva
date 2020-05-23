#include "LogApiHandler.hpp"
#include "Logger.hpp"
#include <nlohmann/json.hpp>

std::shared_ptr<seasocks::Response> LogApiHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {
    if(pUrl.path().at(0) == "all") {
        nlohmann::json response = Logger::getAllLoggerNames();
        return seasocks::Response::jsonResponse(response.dump());
    }
    return seasocks::Response::unhandled();
}
