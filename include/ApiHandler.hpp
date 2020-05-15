#pragma once
#include "WebHandler.hpp"

class ApiHandler : public WebHandler {
    virtual sp<seasocks::Response> handle(const seasocks::CrackedUri& pUrl) override;
};
