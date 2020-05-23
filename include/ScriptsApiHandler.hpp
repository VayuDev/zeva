#pragma once
#include "WebHandler.hpp"
#include "DatabaseWrapper.hpp"

class ScriptManager;

class ScriptsApiHandler : public WebHandler {
public:
    explicit ScriptsApiHandler(std::shared_ptr<class DatabaseWrapper> pDb, std::shared_ptr<ScriptManager>);
    std::shared_ptr<seasocks::Response> handle(const seasocks::CrackedUri& pUrl, const seasocks::Request &pRequest) override;
private:
    std::shared_ptr<class DatabaseWrapper>  mDb;
    std::shared_ptr<ScriptManager> mScriptManager;
};