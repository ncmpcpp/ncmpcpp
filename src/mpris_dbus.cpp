/***************************************************************************
 *   Copyright (C) 2026-Now rbrgmn <roman.bergman@tutanota.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "mpris_dbus.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "mpdpp.h"
#include "status.h"
#include "statusbar.h"

#ifdef ENABLE_DBUS_MPRIS
#include <dbus/dbus.h>
#endif

namespace {

#ifdef ENABLE_DBUS_MPRIS

// Single exported MPRIS service/object for this process.
constexpr const char *kService = "org.mpris.MediaPlayer2.ncmpcpp";
constexpr const char *kPath = "/org/mpris/MediaPlayer2";
constexpr const char *kMprisInterface = "org.mpris.MediaPlayer2";
constexpr const char *kPlayerInterface = "org.mpris.MediaPlayer2.Player";
constexpr const char *kPropertiesInterface = "org.freedesktop.DBus.Properties";
constexpr const char *kIntrospectInterface =
    "org.freedesktop.DBus.Introspectable";

const char *kIntrospectionXml =
    R"(<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
 <interface name="org.freedesktop.DBus.Introspectable">
  <method name="Introspect">
   <arg name="xml_data" type="s" direction="out"/>
  </method>
 </interface>
 <interface name="org.freedesktop.DBus.Properties">
  <method name="Get">
   <arg name="interface_name" direction="in" type="s"/>
   <arg name="property_name" direction="in" type="s"/>
   <arg name="value" direction="out" type="v"/>
  </method>
  <method name="Set">
   <arg name="interface_name" direction="in" type="s"/>
   <arg name="property_name" direction="in" type="s"/>
   <arg name="value" direction="in" type="v"/>
  </method>
  <method name="GetAll">
   <arg name="interface_name" direction="in" type="s"/>
   <arg name="props" direction="out" type="a{sv}"/>
  </method>
  <signal name="PropertiesChanged">
   <arg name="interface_name" type="s"/>
   <arg name="changed_properties" type="a{sv}"/>
   <arg name="invalidated_properties" type="as"/>
  </signal>
 </interface>
 <interface name="org.mpris.MediaPlayer2">
  <method name="Raise"/>
  <method name="Quit"/>
  <property name="CanQuit" type="b" access="read"/>
  <property name="CanRaise" type="b" access="read"/>
  <property name="CanSetFullscreen" type="b" access="read"/>
  <property name="HasTrackList" type="b" access="read"/>
  <property name="Identity" type="s" access="read"/>
  <property name="DesktopEntry" type="s" access="read"/>
  <property name="Fullscreen" type="b" access="read"/>
  <property name="SupportedUriSchemes" type="as" access="read"/>
  <property name="SupportedMimeTypes" type="as" access="read"/>
 </interface>
 <interface name="org.mpris.MediaPlayer2.Player">
  <method name="Next"/>
  <method name="Previous"/>
  <method name="Pause"/>
  <method name="PlayPause"/>
  <method name="Stop"/>
  <method name="Play"/>
  <method name="Seek">
   <arg name="Offset" type="x" direction="in"/>
  </method>
  <method name="SetPosition">
   <arg name="TrackId" type="o" direction="in"/>
   <arg name="Position" type="x" direction="in"/>
  </method>
  <method name="OpenUri">
   <arg name="Uri" type="s" direction="in"/>
  </method>
  <signal name="Seeked">
   <arg name="Position" type="x"/>
  </signal>
  <property name="PlaybackStatus" type="s" access="read"/>
  <property name="LoopStatus" type="s" access="readwrite"/>
  <property name="Rate" type="d" access="readwrite"/>
  <property name="Shuffle" type="b" access="readwrite"/>
  <property name="Metadata" type="a{sv}" access="read"/>
  <property name="Volume" type="d" access="readwrite"/>
  <property name="Position" type="x" access="read"/>
  <property name="MinimumRate" type="d" access="read"/>
  <property name="MaximumRate" type="d" access="read"/>
  <property name="CanGoNext" type="b" access="read"/>
  <property name="CanGoPrevious" type="b" access="read"/>
  <property name="CanPlay" type="b" access="read"/>
  <property name="CanPause" type="b" access="read"/>
  <property name="CanSeek" type="b" access="read"/>
  <property name="CanControl" type="b" access="read"/>
 </interface>
</node>
)";

DBusConnection *g_conn = nullptr;
int g_fd = -1;

struct Snapshot {
  bool valid = false;
  std::string playback;
  std::string loop;
  bool shuffle = false;
  double volume = 0.0;
  int song_id = -1;
  int64_t position_us = 0;
};

// Last published state snapshot used to emit delta-only PropertiesChanged.
Snapshot g_last_snapshot;

const char *playbackState() {
  // Map internal MPD state to MPRIS PlaybackStatus.
  switch (Status::State::player()) {
  case MPD::psPlay:
    return "Playing";
  case MPD::psPause:
    return "Paused";
  case MPD::psStop:
  case MPD::psUnknown:
    return "Stopped";
  }
  return "Stopped";
}

const char *loopStatus() {
  if (Status::State::single() && Status::State::repeat())
    return "Track";
  if (Status::State::repeat())
    return "Playlist";
  return "None";
}

bool shuffleEnabled() { return Status::State::random(); }

double volumeLevel() {
  const int v = Status::State::volume();
  return v >= 0 ? v / 100.0 : 0.0;
}

int64_t positionMicros() {
  return static_cast<int64_t>(Status::State::elapsedTime()) * 1000000LL;
}

std::string currentTrackPath() {
  int id = Status::State::currentSongID();
  if (id < 0)
    id = 0;
  return std::string("/org/mpris/MediaPlayer2/track/") + std::to_string(id);
}

Snapshot currentSnapshot() {
  Snapshot s;
  s.valid = true;
  s.playback = playbackState();
  s.loop = loopStatus();
  s.shuffle = shuffleEnabled();
  s.volume = volumeLevel();
  s.song_id = Status::State::currentSongID();
  s.position_us = positionMicros();
  return s;
}

DBusMessage *methodReturn(DBusMessage *msg) {
  return dbus_message_new_method_return(msg);
}

DBusMessage *errorReply(DBusMessage *msg, const char *name, const char *text) {
  return dbus_message_new_error(msg, name, text);
}

void appendVariantBool(DBusMessageIter *iter, bool value) {
  dbus_bool_t db = value ? TRUE : FALSE;
  DBusMessageIter v;
  dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
                                   DBUS_TYPE_BOOLEAN_AS_STRING, &v);
  dbus_message_iter_append_basic(&v, DBUS_TYPE_BOOLEAN, &db);
  dbus_message_iter_close_container(iter, &v);
}

void appendVariantString(DBusMessageIter *iter, const char *value) {
  DBusMessageIter v;
  dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
                                   DBUS_TYPE_STRING_AS_STRING, &v);
  dbus_message_iter_append_basic(&v, DBUS_TYPE_STRING, &value);
  dbus_message_iter_close_container(iter, &v);
}

void appendVariantInt64(DBusMessageIter *iter, int64_t value) {
  DBusMessageIter v;
  dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
                                   DBUS_TYPE_INT64_AS_STRING, &v);
  dbus_message_iter_append_basic(&v, DBUS_TYPE_INT64, &value);
  dbus_message_iter_close_container(iter, &v);
}

void appendVariantDouble(DBusMessageIter *iter, double value) {
  DBusMessageIter v;
  dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
                                   DBUS_TYPE_DOUBLE_AS_STRING, &v);
  dbus_message_iter_append_basic(&v, DBUS_TYPE_DOUBLE, &value);
  dbus_message_iter_close_container(iter, &v);
}

void appendVariantStringArray(DBusMessageIter *iter,
                              const std::vector<const char *> &values) {
  DBusMessageIter v, arr;
  dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "as", &v);
  dbus_message_iter_open_container(&v, DBUS_TYPE_ARRAY,
                                   DBUS_TYPE_STRING_AS_STRING, &arr);
  for (auto *value : values)
    dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &value);
  dbus_message_iter_close_container(&v, &arr);
  dbus_message_iter_close_container(iter, &v);
}

void appendVariantMetadata(DBusMessageIter *iter) {
  // Build metadata lazily from current MPD song to keep responses up to date.
  DBusMessageIter v, dict;
  dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "a{sv}", &v);
  dbus_message_iter_open_container(&v, DBUS_TYPE_ARRAY, "{sv}", &dict);

  MPD::Song song;
  try {
    song = Mpd.GetCurrentSong();
  } catch (MPD::Error &) {
  }

  const auto track_id = currentTrackPath();
  const char *track_id_c = track_id.c_str();
  {
    DBusMessageIter entry, variant;
    const char *key = "mpris:trackid";
    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr,
                                     &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_OBJECT_PATH_AS_STRING, &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_OBJECT_PATH,
                                   &track_id_c);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(&dict, &entry);
  }

  if (!song.empty()) {
    auto append_string_prop = [&dict](const char *name,
                                      const std::string &value) {
      if (value.empty())
        return;
      DBusMessageIter entry, variant;
      const char *key = name;
      const char *str = value.c_str();
      dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr,
                                       &entry);
      dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
      dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
                                       DBUS_TYPE_STRING_AS_STRING, &variant);
      dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &str);
      dbus_message_iter_close_container(&entry, &variant);
      dbus_message_iter_close_container(&dict, &entry);
    };

    auto append_string_array_prop =
        [&dict](const char *name, const std::vector<std::string> &values) {
          if (values.empty())
            return;
          DBusMessageIter entry, variant, arr;
          const char *key = name;
          dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr,
                                           &entry);
          dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
          dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "as",
                                           &variant);
          dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY,
                                           DBUS_TYPE_STRING_AS_STRING, &arr);
          for (const auto &value : values) {
            const char *str = value.c_str();
            dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &str);
          }
          dbus_message_iter_close_container(&variant, &arr);
          dbus_message_iter_close_container(&entry, &variant);
          dbus_message_iter_close_container(&dict, &entry);
        };

    auto append_int64_prop = [&dict](const char *name, int64_t value) {
      DBusMessageIter entry, variant;
      const char *key = name;
      dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr,
                                       &entry);
      dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
      dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
                                       DBUS_TYPE_INT64_AS_STRING, &variant);
      dbus_message_iter_append_basic(&variant, DBUS_TYPE_INT64, &value);
      dbus_message_iter_close_container(&entry, &variant);
      dbus_message_iter_close_container(&dict, &entry);
    };

    const auto title = song.getTitle();
    const auto name = song.getName();
    const auto artist = song.getArtist();
    const auto album = song.getAlbum();
    const auto uri = song.getURI();
    const int64_t length_us =
        static_cast<int64_t>(song.getDuration()) * 1000000LL;

    append_string_prop("xesam:title", !title.empty() ? title : name);
    if (!artist.empty())
      append_string_array_prop("xesam:artist", {artist});
    append_string_prop("xesam:album", album);
    append_string_prop("xesam:url", uri);
    append_int64_prop("mpris:length", length_us);
  }

  dbus_message_iter_close_container(&v, &dict);
  dbus_message_iter_close_container(iter, &v);
}

void appendPropBool(DBusMessageIter *dict, const char *name, bool value) {
  DBusMessageIter entry;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &name);
  appendVariantBool(&entry, value);
  dbus_message_iter_close_container(dict, &entry);
}

void appendPropString(DBusMessageIter *dict, const char *name,
                      const char *value) {
  DBusMessageIter entry;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &name);
  appendVariantString(&entry, value);
  dbus_message_iter_close_container(dict, &entry);
}

void appendPropDouble(DBusMessageIter *dict, const char *name, double value) {
  DBusMessageIter entry;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &name);
  appendVariantDouble(&entry, value);
  dbus_message_iter_close_container(dict, &entry);
}

void appendPropInt64(DBusMessageIter *dict, const char *name, int64_t value) {
  DBusMessageIter entry;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &name);
  appendVariantInt64(&entry, value);
  dbus_message_iter_close_container(dict, &entry);
}

void appendPropStringArray(DBusMessageIter *dict, const char *name,
                           const std::vector<const char *> &values) {
  DBusMessageIter entry;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &name);
  appendVariantStringArray(&entry, values);
  dbus_message_iter_close_container(dict, &entry);
}

void appendPropMetadata(DBusMessageIter *dict, const char *name) {
  DBusMessageIter entry;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &name);
  appendVariantMetadata(&entry);
  dbus_message_iter_close_container(dict, &entry);
}

void appendPlayerProp(DBusMessageIter *iter, const char *prop) {
  // Centralized property encoder used by Properties.Get.
  if (!std::strcmp(prop, "PlaybackStatus"))
    appendVariantString(iter, playbackState());
  else if (!std::strcmp(prop, "Metadata"))
    appendVariantMetadata(iter);
  else if (!std::strcmp(prop, "Position"))
    appendVariantInt64(iter, positionMicros());
  else if (!std::strcmp(prop, "Rate") || !std::strcmp(prop, "MinimumRate") ||
           !std::strcmp(prop, "MaximumRate"))
    appendVariantDouble(iter, 1.0);
  else if (!std::strcmp(prop, "Volume"))
    appendVariantDouble(iter, volumeLevel());
  else if (!std::strcmp(prop, "LoopStatus"))
    appendVariantString(iter, loopStatus());
  else if (!std::strcmp(prop, "Shuffle"))
    appendVariantBool(iter, shuffleEnabled());
  else if (!std::strcmp(prop, "CanGoNext") ||
           !std::strcmp(prop, "CanGoPrevious") ||
           !std::strcmp(prop, "CanPlay") || !std::strcmp(prop, "CanPause") ||
           !std::strcmp(prop, "CanControl") || !std::strcmp(prop, "CanSeek"))
    appendVariantBool(iter, true);
  else
    throw std::runtime_error("unknown player property");
}

void appendMprisProp(DBusMessageIter *iter, const char *prop) {
  if (!std::strcmp(prop, "CanQuit") || !std::strcmp(prop, "CanRaise") ||
      !std::strcmp(prop, "HasTrackList") ||
      !std::strcmp(prop, "CanSetFullscreen") ||
      !std::strcmp(prop, "Fullscreen"))
    appendVariantBool(iter, false);
  else if (!std::strcmp(prop, "Identity"))
    appendVariantString(iter, "ncmpcpp");
  else if (!std::strcmp(prop, "DesktopEntry"))
    appendVariantString(iter, "ncmpcpp");
  else if (!std::strcmp(prop, "SupportedUriSchemes"))
    appendVariantStringArray(iter, {"file", "http", "https"});
  else if (!std::strcmp(prop, "SupportedMimeTypes"))
    appendVariantStringArray(iter, {});
  else
    throw std::runtime_error("unknown mpris property");
}

DBusMessage *handleGet(DBusMessage *msg) {
  // org.freedesktop.DBus.Properties.Get
  DBusError err;
  dbus_error_init(&err);
  const char *iface = nullptr;
  const char *prop = nullptr;
  if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &iface,
                             DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID)) {
    auto out = errorReply(msg, DBUS_ERROR_INVALID_ARGS,
                          err.message ? err.message : "invalid args");
    dbus_error_free(&err);
    return out;
  }

  auto out = methodReturn(msg);
  DBusMessageIter iter;
  dbus_message_iter_init_append(out, &iter);
  try {
    if (!std::strcmp(iface, kPlayerInterface))
      appendPlayerProp(&iter, prop);
    else if (!std::strcmp(iface, kMprisInterface))
      appendMprisProp(&iter, prop);
    else {
      dbus_message_unref(out);
      return errorReply(msg, DBUS_ERROR_UNKNOWN_INTERFACE, "unknown interface");
    }
  } catch (std::runtime_error &) {
    dbus_message_unref(out);
    return errorReply(msg, DBUS_ERROR_UNKNOWN_PROPERTY, "unknown property");
  }
  return out;
}

DBusMessage *handleGetAll(DBusMessage *msg) {
  // org.freedesktop.DBus.Properties.GetAll
  DBusError err;
  dbus_error_init(&err);
  const char *iface = nullptr;
  if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &iface,
                             DBUS_TYPE_INVALID)) {
    auto out = errorReply(msg, DBUS_ERROR_INVALID_ARGS,
                          err.message ? err.message : "invalid args");
    dbus_error_free(&err);
    return out;
  }

  auto out = methodReturn(msg);
  DBusMessageIter iter, dict;
  dbus_message_iter_init_append(out, &iter);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);

  if (!std::strcmp(iface, kPlayerInterface)) {
    appendPropString(&dict, "PlaybackStatus", playbackState());
    appendPropMetadata(&dict, "Metadata");
    appendPropInt64(&dict, "Position", positionMicros());
    appendPropDouble(&dict, "Rate", 1.0);
    appendPropDouble(&dict, "MinimumRate", 1.0);
    appendPropDouble(&dict, "MaximumRate", 1.0);
    appendPropDouble(&dict, "Volume", volumeLevel());
    appendPropString(&dict, "LoopStatus", loopStatus());
    appendPropBool(&dict, "Shuffle", shuffleEnabled());
    appendPropBool(&dict, "CanGoNext", true);
    appendPropBool(&dict, "CanGoPrevious", true);
    appendPropBool(&dict, "CanPlay", true);
    appendPropBool(&dict, "CanPause", true);
    appendPropBool(&dict, "CanSeek", true);
    appendPropBool(&dict, "CanControl", true);
  } else if (!std::strcmp(iface, kMprisInterface)) {
    appendPropBool(&dict, "CanQuit", false);
    appendPropBool(&dict, "CanRaise", false);
    appendPropBool(&dict, "CanSetFullscreen", false);
    appendPropBool(&dict, "HasTrackList", false);
    appendPropBool(&dict, "Fullscreen", false);
    appendPropString(&dict, "Identity", "ncmpcpp");
    appendPropString(&dict, "DesktopEntry", "ncmpcpp");
    appendPropStringArray(&dict, "SupportedUriSchemes",
                          {"file", "http", "https"});
    appendPropStringArray(&dict, "SupportedMimeTypes", {});
  } else {
    dbus_message_unref(out);
    return errorReply(msg, DBUS_ERROR_UNKNOWN_INTERFACE, "unknown interface");
  }

  dbus_message_iter_close_container(&iter, &dict);
  return out;
}

void emitSeeked(int64_t position_us) {
  // MPRIS Player.Seeked signal after successful seek operations.
  auto *signal = dbus_message_new_signal(kPath, kPlayerInterface, "Seeked");
  if (signal == nullptr)
    return;
  dbus_message_append_args(signal, DBUS_TYPE_INT64, &position_us,
                           DBUS_TYPE_INVALID);
  dbus_connection_send(g_conn, signal, nullptr);
  dbus_message_unref(signal);
}

void emitPlayerChanged(const Snapshot &old_s, const Snapshot &new_s) {
  // Emit only changed fields to minimize bus traffic and avoid noisy redraws.
  if (g_conn == nullptr)
    return;

  const bool playback_changed =
      !old_s.valid || old_s.playback != new_s.playback;
  const bool loop_changed = !old_s.valid || old_s.loop != new_s.loop;
  const bool shuffle_changed = !old_s.valid || old_s.shuffle != new_s.shuffle;
  const bool volume_changed =
      !old_s.valid || std::fabs(old_s.volume - new_s.volume) > 1e-9;
  const bool song_changed = !old_s.valid || old_s.song_id != new_s.song_id;
  if (!(playback_changed || loop_changed || shuffle_changed || volume_changed ||
        song_changed))
    return;

  auto *signal =
      dbus_message_new_signal(kPath, kPropertiesInterface, "PropertiesChanged");
  if (signal == nullptr)
    return;

  DBusMessageIter iter, changed, invalidated;
  dbus_message_iter_init_append(signal, &iter);
  const char *iface = kPlayerInterface;
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &iface);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &changed);

  if (playback_changed)
    appendPropString(&changed, "PlaybackStatus", playbackState());
  if (loop_changed)
    appendPropString(&changed, "LoopStatus", loopStatus());
  if (shuffle_changed)
    appendPropBool(&changed, "Shuffle", shuffleEnabled());
  if (volume_changed)
    appendPropDouble(&changed, "Volume", volumeLevel());
  if (song_changed) {
    appendPropMetadata(&changed, "Metadata");
    appendPropInt64(&changed, "Position", positionMicros());
  }

  dbus_message_iter_close_container(&iter, &changed);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
                                   DBUS_TYPE_STRING_AS_STRING, &invalidated);
  dbus_message_iter_close_container(&iter, &invalidated);

  dbus_connection_send(g_conn, signal, nullptr);
  dbus_message_unref(signal);
}

bool parseSetValue(DBusMessage *msg, const char **iface, const char **prop,
                   DBusMessageIter *value) {
  // Parse (interface_name, property_name, variant value) from Set call.
  DBusMessageIter it;
  if (!dbus_message_iter_init(msg, &it) ||
      dbus_message_iter_get_arg_type(&it) != DBUS_TYPE_STRING)
    return false;
  dbus_message_iter_get_basic(&it, iface);
  if (!dbus_message_iter_next(&it) ||
      dbus_message_iter_get_arg_type(&it) != DBUS_TYPE_STRING)
    return false;
  dbus_message_iter_get_basic(&it, prop);
  if (!dbus_message_iter_next(&it) ||
      dbus_message_iter_get_arg_type(&it) != DBUS_TYPE_VARIANT)
    return false;
  dbus_message_iter_recurse(&it, value);
  return true;
}

DBusMessage *handleSet(DBusMessage *msg) {
  // org.freedesktop.DBus.Properties.Set for writable Player properties.
  const char *iface = nullptr;
  const char *prop = nullptr;
  DBusMessageIter value;
  if (!parseSetValue(msg, &iface, &prop, &value))
    return errorReply(msg, DBUS_ERROR_INVALID_ARGS, "invalid args");

  if (std::strcmp(iface, kPlayerInterface) != 0)
    return errorReply(msg, DBUS_ERROR_UNKNOWN_INTERFACE, "unknown interface");

  try {
    if (!std::strcmp(prop, "Volume")) {
      if (dbus_message_iter_get_arg_type(&value) != DBUS_TYPE_DOUBLE)
        return errorReply(msg, DBUS_ERROR_INVALID_ARGS,
                          "Volume expects double");
      double v = 0.0;
      dbus_message_iter_get_basic(&value, &v);
      if (v < 0.0)
        v = 0.0;
      if (v > 1.0)
        v = 1.0;
      Mpd.SetVolume(static_cast<unsigned>(std::lround(v * 100.0)));
    } else if (!std::strcmp(prop, "Shuffle")) {
      if (dbus_message_iter_get_arg_type(&value) != DBUS_TYPE_BOOLEAN)
        return errorReply(msg, DBUS_ERROR_INVALID_ARGS, "Shuffle expects bool");
      dbus_bool_t b = FALSE;
      dbus_message_iter_get_basic(&value, &b);
      Mpd.SetRandom(b == TRUE);
    } else if (!std::strcmp(prop, "LoopStatus")) {
      if (dbus_message_iter_get_arg_type(&value) != DBUS_TYPE_STRING)
        return errorReply(msg, DBUS_ERROR_INVALID_ARGS,
                          "LoopStatus expects string");
      const char *ls = nullptr;
      dbus_message_iter_get_basic(&value, &ls);
      if (!std::strcmp(ls, "None")) {
        Mpd.SetRepeat(false);
        Mpd.SetSingle(false);
      } else if (!std::strcmp(ls, "Playlist")) {
        Mpd.SetRepeat(true);
        Mpd.SetSingle(false);
      } else if (!std::strcmp(ls, "Track")) {
        Mpd.SetRepeat(true);
        Mpd.SetSingle(true);
      } else
        return errorReply(msg, DBUS_ERROR_INVALID_ARGS, "invalid LoopStatus");
    } else if (!std::strcmp(prop, "Rate")) {
      if (dbus_message_iter_get_arg_type(&value) != DBUS_TYPE_DOUBLE)
        return errorReply(msg, DBUS_ERROR_INVALID_ARGS, "Rate expects double");
      double rate = 0.0;
      dbus_message_iter_get_basic(&value, &rate);
      if (std::fabs(rate - 1.0) > 1e-9)
        return errorReply(msg, DBUS_ERROR_INVALID_ARGS,
                          "only Rate=1.0 is supported");
    } else
      return errorReply(msg, DBUS_ERROR_PROPERTY_READ_ONLY,
                        "property is read-only");
  } catch (MPD::ClientError &e) {
    Status::handleClientError(e);
    return errorReply(msg, DBUS_ERROR_FAILED, e.what());
  } catch (MPD::ServerError &e) {
    Status::handleServerError(e);
    return errorReply(msg, DBUS_ERROR_FAILED, e.what());
  }

  return methodReturn(msg);
}

void callMpdAndMaybeNotify(void (*fn)(), const char *ok_msg) {
  try {
    fn();
    Statusbar::print(1, ok_msg);
  } catch (MPD::ClientError &e) {
    Status::handleClientError(e);
  } catch (MPD::ServerError &e) {
    Status::handleServerError(e);
  }
}

int64_t floorDivMicrosToSeconds(int64_t us) {
  // MPRIS uses microseconds while MPD seek API expects seconds.
  // Floor division keeps negative offsets well-defined.
  int64_t s = us / 1000000LL;
  if (us < 0 && (us % 1000000LL) != 0)
    --s;
  return s;
}

DBusMessage *handlePlayerMethods(DBusMessage *msg) {
  // Map Player methods to MPD operations.
  if (dbus_message_is_method_call(msg, kPlayerInterface, "Next"))
    callMpdAndMaybeNotify(+[]() { Mpd.Next(); }, "MPRIS: next");
  else if (dbus_message_is_method_call(msg, kPlayerInterface, "Previous"))
    callMpdAndMaybeNotify(+[]() { Mpd.Prev(); }, "MPRIS: previous");
  else if (dbus_message_is_method_call(msg, kPlayerInterface, "PlayPause"))
    callMpdAndMaybeNotify(+[]() { Mpd.Toggle(); }, "MPRIS: play/pause");
  else if (dbus_message_is_method_call(msg, kPlayerInterface, "Play"))
    callMpdAndMaybeNotify(+[]() { Mpd.Play(); }, "MPRIS: play");
  else if (dbus_message_is_method_call(msg, kPlayerInterface, "Pause"))
    callMpdAndMaybeNotify(+[]() { Mpd.Pause(1); }, "MPRIS: pause");
  else if (dbus_message_is_method_call(msg, kPlayerInterface, "Stop"))
    callMpdAndMaybeNotify(+[]() { Mpd.Stop(); }, "MPRIS: stop");
  else if (dbus_message_is_method_call(msg, kPlayerInterface, "Seek")) {
    DBusError err;
    dbus_error_init(&err);
    int64_t offset_us = 0;
    if (!dbus_message_get_args(msg, &err, DBUS_TYPE_INT64, &offset_us,
                               DBUS_TYPE_INVALID)) {
      auto out = errorReply(msg, DBUS_ERROR_INVALID_ARGS,
                            err.message ? err.message : "invalid args");
      dbus_error_free(&err);
      return out;
    }
    try {
      const int pos = Status::State::currentSongPosition();
      if (pos >= 0) {
        int64_t target = static_cast<int64_t>(Status::State::elapsedTime()) +
                         floorDivMicrosToSeconds(offset_us);
        if (target < 0)
          target = 0;
        const int64_t total = Status::State::totalTime();
        if (total > 0 && target > total)
          target = total;
        Mpd.Seek(static_cast<unsigned int>(pos),
                 static_cast<unsigned int>(target));
        emitSeeked(target * 1000000LL);
      }
    } catch (MPD::ClientError &e) {
      Status::handleClientError(e);
      return errorReply(msg, DBUS_ERROR_FAILED, e.what());
    } catch (MPD::ServerError &e) {
      Status::handleServerError(e);
      return errorReply(msg, DBUS_ERROR_FAILED, e.what());
    }
  } else if (dbus_message_is_method_call(msg, kPlayerInterface,
                                         "SetPosition")) {
    DBusError err;
    dbus_error_init(&err);
    const char *track = nullptr;
    int64_t position_us = 0;
    if (!dbus_message_get_args(msg, &err, DBUS_TYPE_OBJECT_PATH, &track,
                               DBUS_TYPE_INT64, &position_us,
                               DBUS_TYPE_INVALID)) {
      auto out = errorReply(msg, DBUS_ERROR_INVALID_ARGS,
                            err.message ? err.message : "invalid args");
      dbus_error_free(&err);
      return out;
    }

    const auto curr = currentTrackPath();
    if (curr == track) {
      try {
        const int pos = Status::State::currentSongPosition();
        if (pos >= 0) {
          int64_t target = floorDivMicrosToSeconds(position_us);
          if (target < 0)
            target = 0;
          const int64_t total = Status::State::totalTime();
          if (total > 0 && target > total)
            target = total;
          Mpd.Seek(static_cast<unsigned int>(pos),
                   static_cast<unsigned int>(target));
          emitSeeked(target * 1000000LL);
        }
      } catch (MPD::ClientError &e) {
        Status::handleClientError(e);
        return errorReply(msg, DBUS_ERROR_FAILED, e.what());
      } catch (MPD::ServerError &e) {
        Status::handleServerError(e);
        return errorReply(msg, DBUS_ERROR_FAILED, e.what());
      }
    }
  } else if (dbus_message_is_method_call(msg, kPlayerInterface, "OpenUri")) {
    DBusError err;
    dbus_error_init(&err);
    const char *uri = nullptr;
    if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &uri,
                               DBUS_TYPE_INVALID)) {
      auto out = errorReply(msg, DBUS_ERROR_INVALID_ARGS,
                            err.message ? err.message : "invalid args");
      dbus_error_free(&err);
      return out;
    }
    try {
      const int id = Mpd.AddSong(uri);
      if (id >= 0)
        Mpd.PlayID(id);
    } catch (MPD::ClientError &e) {
      Status::handleClientError(e);
      return errorReply(msg, DBUS_ERROR_FAILED, e.what());
    } catch (MPD::ServerError &e) {
      Status::handleServerError(e);
      return errorReply(msg, DBUS_ERROR_FAILED, e.what());
    }
  } else
    return nullptr;

  return methodReturn(msg);
}

DBusHandlerResult handleMessage(DBusConnection *, DBusMessage *msg, void *) {
  // Main dispatcher for introspection, Properties API and Player methods.
  if (dbus_message_is_method_call(msg, kIntrospectInterface, "Introspect")) {
    auto out = methodReturn(msg);
    DBusMessageIter iter;
    dbus_message_iter_init_append(out, &iter);
    const char *xml = kIntrospectionXml;
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &xml);
    dbus_connection_send(g_conn, out, nullptr);
    dbus_message_unref(out);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(msg, kPropertiesInterface, "Get")) {
    auto out = handleGet(msg);
    dbus_connection_send(g_conn, out, nullptr);
    dbus_message_unref(out);
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  if (dbus_message_is_method_call(msg, kPropertiesInterface, "GetAll")) {
    auto out = handleGetAll(msg);
    dbus_connection_send(g_conn, out, nullptr);
    dbus_message_unref(out);
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  if (dbus_message_is_method_call(msg, kPropertiesInterface, "Set")) {
    auto out = handleSet(msg);
    dbus_connection_send(g_conn, out, nullptr);
    dbus_message_unref(out);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(msg, kMprisInterface, "Raise") ||
      dbus_message_is_method_call(msg, kMprisInterface, "Quit")) {
    auto out = methodReturn(msg);
    dbus_connection_send(g_conn, out, nullptr);
    dbus_message_unref(out);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  auto out = handlePlayerMethods(msg);
  if (out != nullptr) {
    dbus_connection_send(g_conn, out, nullptr);
    dbus_message_unref(out);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusObjectPathVTable kVTable = {nullptr, handleMessage, nullptr,
                                nullptr, nullptr,       nullptr};

#endif // ENABLE_DBUS_MPRIS

} // namespace

void MPRIS::initialize() {
#ifdef ENABLE_DBUS_MPRIS
  // Connect to session bus, own name and register object path.
  DBusError err;
  dbus_error_init(&err);
  g_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (g_conn == nullptr) {
    dbus_error_free(&err);
    return;
  }
  dbus_connection_set_exit_on_disconnect(g_conn, false);
  const int ret = dbus_bus_request_name(g_conn, kService,
                                        DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
  if (dbus_error_is_set(&err) || ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    dbus_error_free(&err);
    dbus_connection_unref(g_conn);
    g_conn = nullptr;
    return;
  }
  if (!dbus_connection_register_object_path(g_conn, kPath, &kVTable, nullptr)) {
    dbus_connection_unref(g_conn);
    g_conn = nullptr;
    return;
  }
  if (!dbus_connection_get_unix_fd(g_conn, &g_fd))
    g_fd = -1;
  g_last_snapshot = Snapshot();
#endif // ENABLE_DBUS_MPRIS
}

void MPRIS::shutdown() {
#ifdef ENABLE_DBUS_MPRIS
  if (g_conn != nullptr) {
    dbus_connection_unregister_object_path(g_conn, kPath);
    dbus_bus_release_name(g_conn, kService, nullptr);
    dbus_connection_unref(g_conn);
    g_conn = nullptr;
    g_fd = -1;
    g_last_snapshot = Snapshot();
  }
#endif // ENABLE_DBUS_MPRIS
}

void MPRIS::process() {
#ifdef ENABLE_DBUS_MPRIS
  if (g_conn == nullptr)
    return;
  // Pump incoming D-Bus messages, then publish state deltas as signals.
  dbus_connection_read_write(g_conn, 0);
  while (dbus_connection_get_dispatch_status(g_conn) ==
         DBUS_DISPATCH_DATA_REMAINS)
    dbus_connection_dispatch(g_conn);

  const auto snap = currentSnapshot();
  emitPlayerChanged(g_last_snapshot, snap);
  g_last_snapshot = snap;
#endif // ENABLE_DBUS_MPRIS
}

void MPRIS::registerFDCallback(NC::Window &w) {
#ifdef ENABLE_DBUS_MPRIS
  if (g_conn == nullptr || g_fd < 0)
    return;
  w.addFDCallback(g_fd, &MPRIS::process);
#else
  (void)w;
#endif // ENABLE_DBUS_MPRIS
}
