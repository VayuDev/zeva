#include "DatabaseWrapper.hpp"

void DatabaseWrapper::onNotification(const std::string &pPayload) {
    for(auto& func: mListeners) {
        func(pPayload);
    }
}
