#include "WebHttpRouter.hpp"
#include "common.hpp"
#include <seasocks/Request.h>
#include <seasocks/ResponseWriter.h>
#include <seasocks/ResponseBuilder.h>
#include <fstream>
#include <Logger.hpp>
#include <seasocks/util/CrackedUri.h>
#include <filesystem>

std::shared_ptr<seasocks::Response> WebHttpRouter::handle(const seasocks::Request &request) {

    seasocks::CrackedUri path{request.getRequestUri()};
    if(path.path().empty()) {
        return seasocks::ResponseBuilder(seasocks::ResponseCode::MovedPermanently).withLocation("/html/index.html").build();
    }
    for(auto& h: mHandlers) {
        assert(h);
        auto resp = h->handle(path, request);
        if(resp) {
            return resp;
        }
    }

    return seasocks::Response::error(seasocks::ResponseCode::NotFound, "File not found :c");
}

void WebHttpRouter::addHandler(sp<WebHandler> pHandler) {
    mHandlers.push_back(pHandler);
}
