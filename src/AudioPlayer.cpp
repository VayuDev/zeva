#include "AudioPlayer.hpp"
#include <gst/gst.h>
#include <limits>
#include <cassert>

static void cb_newpad(GstElement *decodebin, GstPad *pad, gpointer data) {
  GstCaps *caps;
  GstStructure *str;
  GstPad *audiopad;

  /* only link once */
  audiopad = gst_element_get_static_pad(GST_ELEMENT(data), "sink");
  if (GST_PAD_IS_LINKED(audiopad)) {
    g_object_unref(audiopad);
    return;
  }

  /* check media type */
  caps = gst_pad_query_caps(pad, NULL);
  str = gst_caps_get_structure(caps, 0);
  if (!g_strrstr(gst_structure_get_name(str), "audio")) {
    gst_caps_unref(caps);
    gst_object_unref(audiopad);
    return;
  }
  gst_caps_unref(caps);

  /* link'n'play */
  gst_pad_link(pad, audiopad);

  g_object_unref(audiopad);
}

AudioPlayer::AudioPlayer() { gst_init(nullptr, nullptr); }

void AudioPlayer::play(const std::string &pFile,
                       std::optional<DurationCallback> &&pDurationCallback) {
  mDurationCallback = std::move(pDurationCallback);
  GstElement *src, *dec, *conv, *sink;
  GstPad *audiopad;

  /* setup */
  pipeline = gst_pipeline_new("pipeline");

  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

  src = gst_element_factory_make("filesrc", "source");
  g_object_set(G_OBJECT(src), "location", pFile.c_str(), NULL);
  dec = gst_element_factory_make("decodebin", "decoder");
  gst_bin_add_many(GST_BIN(pipeline), src, dec, NULL);
  gst_element_link(src, dec);

  /* create audio output */
  audio = gst_bin_new("audiobin");
  g_signal_connect(dec, "pad-added", G_CALLBACK(cb_newpad), audio);

  conv = gst_element_factory_make("audioconvert", "aconv");
  audiopad = gst_element_get_static_pad(conv, "sink");
  sink = gst_element_factory_make("pulsesink", "sink");
  if (!sink) {
    std::cerr << "AudioPlayer: Unable to connect to output sink\n";
    assert(false);
  }
  gst_bin_add_many(GST_BIN(audio), conv, sink, NULL);
  gst_element_link(conv, sink);
  gst_element_add_pad(audio, gst_ghost_pad_new("sink", audiopad));
  gst_object_unref(audiopad);
  gst_bin_add(GST_BIN(pipeline), audio);

  /* run */
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  running = true;
  asyncDone = false;
}

AudioPlayer::~AudioPlayer() {
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(GST_OBJECT(pipeline));
}

void AudioPlayer::poll() {
  auto message = gst_bus_timed_pop(bus, 1000 * 1000);
  if (message) {
    g_print("Got %s message\n", GST_MESSAGE_TYPE_NAME(message));

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
      GError *err;
      gchar *debug;

      gst_message_parse_error(message, &err, &debug);
      g_print("Error: %s\n", err->message);
      g_error_free(err);
      g_free(debug);
      running = false;
      break;
    }
    case GST_MESSAGE_EOS:
      /* end-of-stream */
      running = false;
      break;
    case GST_MESSAGE_ASYNC_DONE:
      asyncDone = true;
      break;
    case GST_MESSAGE_DURATION_CHANGED:
      if (asyncDone && mDurationCallback)
        mDurationCallback->operator()(getDuration());
      break;
    default:
      /* unhandled message */
      break;
    }
    gst_message_unref(message);
  }
}

bool AudioPlayer::ready() { return asyncDone; }

gint64 AudioPlayer::getDuration() {
  if (running) {
    gint64 duration;
    if (gst_element_query_duration(pipeline, GstFormat::GST_FORMAT_TIME,
                                   &duration))
      return duration;
  }
  return std::numeric_limits<gint64>::max();
}

gint64 AudioPlayer::getPosition() {
  if (running) {
    gint64 position;
    if (gst_element_query_position(pipeline, GstFormat::GST_FORMAT_TIME,
                                   &position))
      return position;
  }
  return std::numeric_limits<gint64>::min();
}
void AudioPlayer::seekTo(gint64 pNs) {
  if (!gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                        GST_SEEK_TYPE_SET, pNs, GST_SEEK_TYPE_NONE,
                        GST_CLOCK_TIME_NONE)) {
    throw std::runtime_error("Seeking failed!");
  }
  asyncDone = false;
}
