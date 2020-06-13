#pragma once

#include "AudioPlayer.hpp"
#include "SftpClient.hpp"
#include "Util.hpp"
#include <string>
#include <vector>

class MusicPlayer {
public:
  explicit MusicPlayer(
      const std::string &pConfigPath = getConfigFileLocation());
  void setPlaylist(std::vector<std::string> &&pPlaylist);
  void playPrevSong();
  void playNextSong();
  void playSong(size_t pNum);
  void poll();
  void pause();
  void resume();
  int64_t getCurrentMusicDuration();
  int64_t getCurrentMusicPosition();
  std::vector<SftpFile> ls(const std::string &pPath);

private:
  void initialize();
  std::optional<AudioPlayer> mAudio;
  std::optional<SftpClient> mSftp;
  std::vector<std::string> mPlaylist;
  int64_t mIndex = -1;
  std::string mConfigFileLocation;
  bool mConnected = false;
  std::recursive_mutex mMutex;
};