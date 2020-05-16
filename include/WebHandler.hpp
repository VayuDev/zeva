#pragma once
#include <seasocks/Response.h>
#include <seasocks/Request.h>
#include <seasocks/util/CrackedUri.h>

class WebHandler {
public:
    virtual std::shared_ptr<seasocks::Response> handle(const seasocks::CrackedUri& pUrl, const seasocks::Request &pRequest) = 0;
private:
};