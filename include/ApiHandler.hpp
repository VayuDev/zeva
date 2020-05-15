#pragma once
#include "WebHandler.hpp"

class ApiHandler : public WebHandler {
public:
    explicit ApiHandler(sp<class DatabaseNetworkConnection> pDb);
    virtual sp<seasocks::Response> handle(const seasocks::CrackedUri& pUrl, const seasocks::Request &pRequest) override;
private:
    sp<class DatabaseNetworkConnection> mDb;
};
