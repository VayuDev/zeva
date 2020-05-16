#include "ApiHandler.hpp"

#include <utility>
#include <seasocks/Response.h>
#include "Util.hpp"
#include "nlohmann/json.hpp"

std::shared_ptr<seasocks::Response> ApiHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {
    if(pUrl.path().at(0) == "api") {
        switch(pRequest.verb()) {
            case seasocks::Request::Verb::Get:
                if(pUrl.path().at(1) == "scripts") {
                    if(pUrl.path().at(2) == "all") {
                        auto res = mDb->query("SELECT * FROM scripts");
                        auto json = queryResultToJson(*res);
                        return seasocks::Response::jsonResponse(json.dump());
                    }
                }
                break;
            default:
                break;
        }
    }
    return nullptr;
}

ApiHandler::ApiHandler(std::shared_ptr<DatabaseWrapper> pDb)
: mDb(std::move(pDb)) {

}
