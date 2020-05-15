#pragma once
#include <seasocks/Response.h>
#include <seasocks/util/CrackedUri.h>
#include "common.hpp"

class WebHandler {
public:
    virtual sp<seasocks::Response> handle(const seasocks::CrackedUri& pUrl) = 0;
private:
};