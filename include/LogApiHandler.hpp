#pragma once
#include "WebHandler.hpp"

class LogApiHandler : public WebHandler {
public:
    std::shared_ptr<seasocks::Response> handle(const seasocks::CrackedUri& pUrl, const seasocks::Request &pRequest) override;
};