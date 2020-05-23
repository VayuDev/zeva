#pragma once
#include <seasocks/PageHandler.h>
#include <WebHandler.hpp>
#include <list>

class WebHttpRouter : public seasocks::PageHandler {
public:
    void addHandler(std::string pUrl1, std::string pUrl2, std::shared_ptr<WebHandler> pHandler);
    std::shared_ptr<seasocks::Response> handle(const seasocks::Request& request) override;
private:
    std::list<std::tuple<std::string, std::string, std::shared_ptr<WebHandler>>> mHandlers;
};