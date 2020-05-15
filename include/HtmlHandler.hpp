#pragma once
#include "WebHandler.hpp"

class HtmlHandler : public WebHandler {
    virtual sp<seasocks::Response> handle(const seasocks::CrackedUri& pUrl) override;
};
