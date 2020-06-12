#include "MusicPlayer.hpp"
#include <fstream>
#include <json/json.h>
#include <trantor/utils/Logger.h>

MusicPlayer::MusicPlayer(const std::string &pConfigPath)
    : mConfigFileLocation(pConfigPath) {}
void MusicPlayer::setPlaylist(std::vector<std::string> &&pPlaylist) {
  initialize();
  mPlaylist = std::move(pPlaylist);
  mIndex = -1;
}
void MusicPlayer::playNextSong() {
  initialize();
  playSong(++mIndex);
}
void MusicPlayer::playSong(size_t pNum) {
  initialize();
  try {
    LOG_INFO << "MusicPlayer: Playing " << mPlaylist.at(pNum);
    auto data = mSftp->download(mPlaylist.at(pNum));
    std::ofstream temp{"/dev/shm/zeva.mp3"};
    temp << data;
    temp.close();
    mAudio->play("/dev/shm/zeva.mp3");
  } catch (std::exception &e) {
    LOG_INFO << "Unable to play requested song: " << e.what();
  }
}
void MusicPlayer::poll() {
  initialize();
  mAudio->poll();
}
void MusicPlayer::pause() {
  initialize();
  if (mAudio->ready())
    mAudio->pause();
}
void MusicPlayer::resume() {
  initialize();
  if (mAudio->ready())
    mAudio->resume();
}
int64_t MusicPlayer::getCurrentMusicDuration() {
  initialize();
  return mAudio->getDuration();
}

int64_t MusicPlayer::getCurrentMusicPosition() {
  initialize();
  return mAudio->getPosition();
}

void MusicPlayer::playPrevSong() {
  initialize();
  playSong(--mIndex);
}

std::vector<SftpFile> MusicPlayer::ls(const std::string &pPath) {
  initialize();
  return mSftp->ls(pPath);
}
void MusicPlayer::initialize() {
  if (!mConnected) {
    std::ifstream instream{mConfigFileLocation};
    Json::Value config;
    instream >> config;
    const auto nasConfig = config["nas"];
    mSftp.emplace(nasConfig["user"].asString(), nasConfig["passwd"].asString(),
                  nasConfig["host"].asString(), nasConfig["port"].asInt());
    mAudio.emplace([this] { playNextSong(); });
    mConnected = true;
  }
}
