#include "MusicPlayer.hpp"
#include <fstream>
#include <json/json.h>
#include <trantor/utils/Logger.h>

MusicPlayer::MusicPlayer(const std::string &pConfigPath) {
  std::ifstream instream{pConfigPath};
  Json::Value config;
  instream >> config;
  const auto nasConfig = config["nas"];
  mSftp.emplace(nasConfig["user"].asString(), nasConfig["passwd"].asString(),
                nasConfig["host"].asString(), nasConfig["port"].asInt());
  mAudio.emplace([this] { playNextSong(); });
}
void MusicPlayer::setPlaylist(std::vector<std::string> &&pPlaylist) {
  mPlaylist = std::move(pPlaylist);
  mIndex = -1;
}
void MusicPlayer::playNextSong() { playSong(++mIndex); }
void MusicPlayer::playSong(size_t pNum) {
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
void MusicPlayer::poll() { mAudio->poll(); }
void MusicPlayer::pause() {
  if (mAudio->ready())
    mAudio->pause();
}
void MusicPlayer::resume() {
  if (mAudio->ready())
    mAudio->resume();
}
int64_t MusicPlayer::getCurrentMusicDuration() { return mAudio->getDuration(); }
int64_t MusicPlayer::getCurrentMusicPosition() { return mAudio->getPosition(); }
void MusicPlayer::playPrevSong() {
  playSong(--mIndex);
}
