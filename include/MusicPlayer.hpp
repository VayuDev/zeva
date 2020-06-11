#pragma once

#include "AudioPlayer.hpp"
#include "SftpClient.hpp"
#include <string>
#include <vector>

class MusicPlayer {
public:
  explicit MusicPlayer(const std::string &pConfigPath);
  void setPlaylist(std::vector<std::string> &&pPlaylist);
  void playPrevSong();
  void playNextSong();
  void poll();
  void pause();
  void resume();
  int64_t getCurrentMusicDuration();
  int64_t getCurrentMusicPosition();

private:
  std::optional<AudioPlayer> mAudio;
  std::optional<SftpClient> mSftp;
  std::vector<std::string> mPlaylist;
  int64_t mIndex = -1;
  void playSong(size_t pNum);
};