#pragma once
#include <functional>
#include <gst/gst.h>
#include <iostream>

using DurationCallback = std::function<void(int64_t)>;

class AudioPlayer {
public:
  AudioPlayer();
  ~AudioPlayer();
  void play(const std::string &pFilename,
            std::optional<DurationCallback> && = {});
  void poll();
  [[nodiscard]] inline bool isPlaying() const { return running; }
  bool ready();
  gint64 getDuration();
  gint64 getPosition();
  void seekTo(gint64 pNs);

  void pause();
  void resume();

private:
  void destruct();

  bool running = false, asyncDone = false;
  GstBus *bus = nullptr;
  GstElement *pipeline = nullptr, *audio = nullptr;
  std::optional<DurationCallback> mDurationCallback;
};