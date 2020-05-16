#pragma once
#include "WebHandler.hpp"
#include "DatabaseWrapper.hpp"

class ScriptManager;

class ApiHandler : public WebHandler {
public:
    explicit ApiHandler(std::shared_ptr<class DatabaseWrapper> pDb, std::shared_ptr<ScriptManager>);
    virtual std::shared_ptr<seasocks::Response> handle(const seasocks::CrackedUri& pUrl, const seasocks::Request &pRequest) override;
private:
    std::shared_ptr<class DatabaseWrapper>  mDb;
    std::shared_ptr<ScriptManager> mScriptManager;
};
