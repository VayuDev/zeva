#pragma once
#include <seasocks/PageHandler.h>
#include <WebHandler.hpp>
#include <list>

class WebHttpRouter : public seasocks::PageHandler {
public:
    void addHandler(std::shared_ptr<WebHandler> pHandler);
    virtual std::shared_ptr<seasocks::Response> handle(const seasocks::Request& request);
private:
    std::list<std::shared_ptr<WebHandler>> mHandlers;
};