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
    for(auto& h: mHandlers) {
        assert(h);
        auto resp = h->handle(path);
        if(resp) {
            return resp;
        }
    }

    return seasocks::Response::error(seasocks::ResponseCode::NotFound, "File not found :c");
}

void WebHttpRouter::addHandler(sp<WebHandler> pHandler) {
    mHandlers.push_back(pHandler);
}
