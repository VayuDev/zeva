#include "MusicPlayer.hpp"
#include <ScriptManager.hpp>
#include <fstream>
#include <json/json.h>
#include <trantor/utils/Logger.h>

MusicPlayer::MusicPlayer(const std::string &pConfigPath)
    : mConfigFileLocation(pConfigPath) {}

void MusicPlayer::setPlaylist(std::vector<std::string> &&pPlaylist) {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  mPlaylist = std::move(pPlaylist);
  mIndex = -1;
}
void MusicPlayer::playNextSong() {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();

  if (playSong(mIndex + 1))
    ScriptManager::the().onAudioEvent("NEXT", getCurrentSong());
}
bool MusicPlayer::playSong(size_t pNum) {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  mIndex = pNum;
  try {
    LOG_INFO << "MusicPlayer: Playing " << mPlaylist.at(pNum);
    auto data = mSftp->download(mPlaylist.at(pNum));
    std::ofstream temp{"/dev/shm/zeva.mp3"};
    temp << data;
    temp.close();
    mAudio->play("/dev/shm/zeva.mp3");
    callWhenDurationIsAvailable([this](auto, auto duration) {
      ScriptManager::the().onAudioEvent("DURATION", getCurrentSong(), duration);
    });
    return true;
  } catch (std::exception &e) {
    LOG_INFO << "Unable to play requested song: " << e.what();
    if (!mAudio->isPlaying())
      ScriptManager::the().onAudioEvent("STOP");
    return false;
  }
}
void MusicPlayer::poll() {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  mAudio->poll();
}
void MusicPlayer::pause() {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  if (mAudio->ready()) {
    mAudio->pause();
    ScriptManager::the().onAudioEvent("PAUSE", getCurrentSong());
  }
}
void MusicPlayer::resume() {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  if (mAudio->ready()) {
    mAudio->resume();
    ScriptManager::the().onAudioEvent("RESUME", getCurrentSong());
  }
}
int64_t MusicPlayer::getCurrentMusicDuration() {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  return mAudio->getDuration();
}

int64_t MusicPlayer::getCurrentMusicPosition() {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  return mAudio->getPosition();
}

void MusicPlayer::playPrevSong() {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  if (playSong(mIndex - 1))
    ScriptManager::the().onAudioEvent("PREV", getCurrentSong());
}

std::vector<SftpFile> MusicPlayer::ls(const std::string &pPath) {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  initialize();
  return mSftp->ls(pPath);
}
void MusicPlayer::initialize() {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  if (!mConnected) {
    std::ifstream instream{mConfigFileLocation};
    Json::Value config;
    instream >> config;
    const auto nasConfig = config["nas"];
    mSftp.emplace(nasConfig["user"].asString(), nasConfig["passwd"].asString(),
                  nasConfig["host"].asString(), nasConfig["port"].asInt());
    mAudio.emplace(
        [this] {
          ScriptManager::the().onAudioEvent("DONE", getCurrentSong());
          playNextSong();
        },
        [this](int64_t position, int64_t duration) {
          std::lock_guard<std::recursive_mutex> lock{mMutex};
          for (auto &callback : mDurationCallbacks) {
            callback(position, duration);
          }
          mDurationCallbacks.clear();
        });
    mConnected = true;
  }
}
std::optional<std::string> MusicPlayer::getCurrentSong() noexcept {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  try {
    return mPlaylist.at(mIndex);
  } catch (...) {
    return {};
  }
}
void MusicPlayer::callWhenDurationIsAvailable(
    std::function<void(int64_t, int64_t)> &&callback) {
  std::lock_guard<std::recursive_mutex> lock{mMutex};
  auto duration = getCurrentMusicDuration();
  if (duration != std::numeric_limits<decltype(duration)>::max()) {
    callback(getCurrentMusicPosition(), getCurrentMusicDuration());
    return;
  }
  mDurationCallbacks.emplace_front(std::move(callback));
}
