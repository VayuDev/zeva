#include "DatabaseWrapper.hpp"
#include <iostream>

void DatabaseWrapper::onNotification(const std::string &pChannel, const std::string &pPayload) {
    std::unique_lock<std::mutex> lock{mNotificationMutex};
    mNotificationQueue.emplace(std::make_pair(pChannel, pPayload));
    lock.unlock();
    mNotifier.notify_all();
}

DatabaseWrapper::DatabaseWrapper()
: mNotificationThread([this] {
    while(mShouldRun) {
        std::unique_lock<std::mutex> lock{mNotificationMutex};
        mNotifier.wait_for(lock, std::chrono::seconds(1));
        while(!mNotificationQueue.empty()) {
            const auto front = mNotificationQueue.front();
            for(auto& listener: mListeners) {
                if(listener.first == front.first) {
                    lock.unlock();
                    listener.second(front.second);
                    lock.lock();
                }
            }
            mNotificationQueue.pop();
        }
    }
}) {

}

DatabaseWrapper::~DatabaseWrapper() {
    std::unique_lock<std::mutex> lock(mNotificationMutex);
    mShouldRun = false;
    mNotifier.notify_all();
    lock.unlock();

    if(mNotificationThread.joinable())
        mNotificationThread.join();
}

void DatabaseWrapper::addListener(const std::string &pChannel, std::function<void(const std::string &)> pListener) {
    std::unique_lock lock{mNotificationMutex};
    if(mListeningTo.count(pChannel) == 0) {
        listenTo(pChannel);
        mListeningTo.emplace(pChannel);
    }
    mListeners.emplace_back(std::make_pair(pChannel, std::move(pListener)));
}
