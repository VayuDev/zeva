#include "ApiHandler.hpp"

#include <utility>
#include "zegdb.hpp"
#include <seasocks/util/Json.h>

sp<seasocks::Response> ApiHandler::handle(const seasocks::CrackedUri &pUrl, const seasocks::Request &pRequest) {
    if(pUrl.path().at(0) == "api") {
        switch(pRequest.verb()) {
            case seasocks::Request::Verb::Get:
                if(pUrl.path().at(1) == "scripts") {
                    if(pUrl.path().at(2) == "all") {
                        auto ret = mDb->querySync("SELECT * FROM sample_min_csv");

                    }
                }
                break;
        }
    }
    return nullptr;
}

ApiHandler::ApiHandler(sp<DatabaseNetworkConnection> pDb)
: mDb(std::move(pDb)) {

}
