#pragma once
#include <seasocks/PageHandler.h>

class WebHttpHandler : public seasocks::PageHandler {
public:
    virtual std::shared_ptr<seasocks::Response> handle(const seasocks::Request& request);
private:
};