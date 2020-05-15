#pragma once
#include <seasocks/Response.h>
#include <seasocks/Request.h>
#include <seasocks/util/CrackedUri.h>
#include "common.hpp"

class WebHandler {
public:
    virtual sp<seasocks::Response> handle(const seasocks::CrackedUri& pUrl, const seasocks::Request &pRequest) = 0;
private:
};