#include "WebHttpRouter.hpp"
#include <seasocks/Request.h>
#include <seasocks/ResponseWriter.h>
#include <seasocks/ResponseBuilder.h>
#include <fstream>
#include <Logger.hpp>
#include <seasocks/util/CrackedUri.h>
#include <filesystem>
#include <cassert>

std::shared_ptr<seasocks::Response> WebHttpRouter::handle(const seasocks::Request &request) {
    if (request.verb() == seasocks::Request::Verb::WebSocket) return seasocks::Response::unhandled();

    seasocks::CrackedUri path{request.getRequestUri()};
    if(path.path().empty()) {
        return seasocks::ResponseBuilder(seasocks::ResponseCode::MovedPermanently).withLocation("/html/hub/hub.html").build();
    }
    for(auto& h: mHandlers) {
        const auto&[path1, path2, handler] = h;
        if(path1.empty() && path2.empty()) {
            assert(handler);
            auto resp = handler->handle(path, request);
            if(resp) {
                return resp;
            }
        } else if(path1 == path.path().at(0) && path2 == path.path().at(1)) {
            auto resp = handler->handle(path.shift().shift(), request);
            if(resp) {
                return resp;
            }
        }
    }

    return seasocks::Response::error(seasocks::ResponseCode::NotFound, "File not found :c");
}

void WebHttpRouter::addHandler(std::string pUrl1, std::string pUrl2, std::shared_ptr<WebHandler> pHandler) {
    mHandlers.emplace_back(std::forward_as_tuple(std::move(pUrl1), std::move(pUrl2), std::move(pHandler)));
}
