#include "ApiHandler.hpp"

#include <utility>
#include <seasocks/Response.h>
#include "Util.hpp"
#include "nlohmann/json.hpp"

std::shared_ptr<seasocks::Response> ApiHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {
    std::unique_ptr<QueryResult> responseData;
    if(pUrl.path().at(0) == "api") {
        switch(pRequest.verb()) {
            case seasocks::Request::Verb::Get:
                if(pUrl.path().at(1) == "scripts") {
                    if(pUrl.path().at(2) == "all") {
                        responseData = mDb->query("SELECT * FROM scripts");
                    }
                }
                break;
            default:
                break;
        }
    }
    if(responseData) {
        if(pUrl.hasParam("format") && pUrl.queryParam("format") == "list") {
            auto json = queryResultToJson(*responseData);
            return seasocks::Response::jsonResponse(json.dump());
        } else {
            auto json = queryResultToJsonMap(*responseData);
            return seasocks::Response::jsonResponse(json.dump());
        }
    }
    return nullptr;
}

ApiHandler::ApiHandler(std::shared_ptr<DatabaseWrapper> pDb)
: mDb(std::move(pDb)) {

}
