#include "ApiHandler.hpp"

sp<seasocks::Response> ApiHandler::handle(const seasocks::CrackedUri &pUrl) {
    if(pUrl.path().at(0) == "api") {
        return seasocks::Response::htmlResponse("Hallo");
    }
    return nullptr;
}
