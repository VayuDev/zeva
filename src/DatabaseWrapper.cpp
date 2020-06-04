#include "DatabaseWrapper.hpp"
#include <iostream>

void DatabaseWrapper::onNotification(const std::string &pChannel, const std::string &pPayload) {
    for(auto& func: mListeners) {
        if(func.first == pChannel)
            func.second(pPayload);
    }
}
