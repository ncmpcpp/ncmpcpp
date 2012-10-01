/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
 *   electricityispower@gmail.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef _MPDPP_H
#define _MPDPP_H

#include <cassert>
#include <set>
#include <vector>

#include <mpd/client.h>
#include "song.h"

namespace MPD {//
	
enum ItemType { itDirectory, itSong, itPlaylist };
enum PlayerState { psUnknown, psStop, psPlay, psPause };
enum ReplayGainMode { rgmOff, rgmTrack, rgmAlbum };

struct Statistics
{
	friend class Connection;
	
	bool empty() const;
	
	unsigned artists() const;
	unsigned albums() const;
	unsigned songs() const;
	unsigned long playTime() const;
	unsigned long uptime() const;
	unsigned long dbUpdateTime() const;
	unsigned long dbPlayTime() const;
	
private:
	Statistics(mpd_stats *stats) : m_stats(stats, mpd_stats_free) { }
	
	std::shared_ptr<mpd_stats> m_stats;
};

struct Item
{
	std::shared_ptr<Song> song;
	ItemType type;
	std::string name;
};

struct StatusChanges
{
	StatusChanges() : Playlist(0), StoredPlaylists(0), SongID(0), Database(0), DBUpdating(0), Volume(0), ElapsedTime(0), Crossfade(0), Random(0), Repeat(0), Single(0), Consume(0), PlayerState(0), StatusFlags(0), Outputs(0) { }
	bool Playlist:1;
	bool StoredPlaylists:1;
	bool SongID:1;
	bool Database:1;
	bool DBUpdating:1;
	bool Volume:1;
	bool ElapsedTime:1;
	bool Crossfade:1;
	bool Random:1;
	bool Repeat:1;
	bool Single:1;
	bool Consume:1;
	bool PlayerState:1;
	bool StatusFlags:1;
	bool Outputs:1;
};

struct Output
{
	Output(const std::string &name_, bool enabled) : m_name(name_), m_enabled(enabled) { }
	
	const std::string &name() const { return m_name; }
	bool isEnabled() const { return m_enabled; }
	
private:
	std::string m_name;
	bool m_enabled;
};

struct TagMTime
{
	TagMTime(const std::string &tag_) : m_tag(tag_), m_mtime(0) { }
	TagMTime(const std::string &tag_, time_t mtime_) : m_tag(tag_), m_mtime(mtime_) { }

	const std::string &tag() const { return m_tag; }
	time_t mtime() const { return m_mtime; }

	void set_mtime(time_t mtime_)
	{
		m_mtime = mtime_;
	}

	void set_tag(std::string tag_)
	{
		m_tag = tag_;
	}

	bool hasMTime()
	{
		return (m_mtime != 0);
	}

private:
	std::string m_tag;
	time_t m_mtime;
};

typedef std::vector<TagMTime> TagMTimeList;
typedef std::vector<Item> ItemList;
typedef std::vector<std::string> StringList;
typedef std::vector<Output> OutputList;

class Connection
{
	typedef void (*StatusUpdater) (Connection *, StatusChanges, void *);
	typedef void (*ErrorHandler) (Connection *, int, const char *, void *);
	
	typedef std::function<void(Item &&)> ItemConsumer;
	typedef std::function<void(Output &&)> OutputConsumer;
	typedef std::function<void(Song &&)> SongConsumer;
	typedef std::function<void(std::string &&)> StringConsumer;
	
public:
	Connection();
	~Connection();
	
	bool Connect();
	bool Connected() const;
	void Disconnect();
	
	const std::string & GetHostname() { return itsHost; }
	int GetPort() { return itsPort; }
	
	unsigned Version() const;
	
	void SetIdleEnabled(bool val) { isIdleEnabled = val; }
	void BlockIdle(bool val) { itsIdleBlocked = val; }
	bool SupportsIdle() const { return supportsIdle; }
	void OrderDataFetching() { hasData = 1; }
	int GetFD() const { return itsFD; }
	
	void SetHostname(const std::string &);
	void SetPort(int port) { itsPort = port; }
	void SetTimeout(int timeout) { itsTimeout = timeout; }
	void SetPassword(const std::string &password) { itsPassword = password; }
	bool SendPassword();
	
	Statistics getStatistics();
	
	void SetStatusUpdater(StatusUpdater, void *);
	void SetErrorHandler(ErrorHandler, void *);
	void UpdateStatus();
	bool UpdateDirectory(const std::string &);
	
	void Play();
	void Play(int);
	void PlayID(int);
	void Pause(bool);
	void Toggle();
	void Stop();
	void Next();
	void Prev();
	bool Move(unsigned, unsigned);
	void Swap(unsigned, unsigned);
	void Seek(unsigned);
	void Shuffle();
	bool ClearMainPlaylist();
	
	bool isPlaying() const { return GetState() > psStop; }
	
	PlayerState GetState() const { return itsCurrentStatus ? PlayerState(mpd_status_get_state(itsCurrentStatus)) : psUnknown; }
	PlayerState GetOldState() const { return itsOldStatus ? PlayerState(mpd_status_get_state(itsOldStatus)) : psUnknown; }
	bool GetRepeat() const { return itsCurrentStatus ? mpd_status_get_repeat(itsCurrentStatus) : 0; }
	bool GetRandom() const { return itsCurrentStatus ? mpd_status_get_random(itsCurrentStatus) : 0; }
	bool GetSingle() const { return itsCurrentStatus ? mpd_status_get_single(itsCurrentStatus) : 0; }
	bool GetConsume() const { return itsCurrentStatus ? mpd_status_get_consume(itsCurrentStatus) : 0; }
	bool GetDBIsUpdating() const { return itsCurrentStatus ? mpd_status_get_update_id(itsCurrentStatus) : 0; }
	int GetVolume() const { return itsCurrentStatus ? mpd_status_get_volume(itsCurrentStatus) : -1; }
	unsigned GetCrossfade() const { return itsCurrentStatus ? mpd_status_get_crossfade(itsCurrentStatus) : 0; }
	unsigned GetPlaylistID() const { return itsCurrentStatus ? mpd_status_get_queue_version(itsCurrentStatus) : 0; }
	unsigned GetOldPlaylistID() const { return itsOldStatus ? mpd_status_get_queue_version(itsOldStatus) : 0; }
	unsigned GetElapsedTime() const { return itsCurrentStatus ? itsElapsed : 0; }
	int GetTotalTime() const { return itsCurrentStatus ? mpd_status_get_total_time(itsCurrentStatus) : 0; }
	unsigned GetBitrate() const { return itsCurrentStatus ? mpd_status_get_kbit_rate(itsCurrentStatus) : 0; }
	
	size_t GetPlaylistLength() const { return itsCurrentStatus ? mpd_status_get_queue_length(itsCurrentStatus) : 0; }
	void GetPlaylistChanges(unsigned, SongConsumer f);
	
	const std::string &GetErrorMessage() const { return itsErrorMessage; }
	
	Song GetCurrentlyPlayingSong();
	int GetCurrentSongPos() const;
	Song GetSong(const std::string &);
	void GetPlaylistContent(const std::string &name, SongConsumer f);
	
	void GetSupportedExtensions(std::set<std::string> &);
	
	void SetRepeat(bool);
	void SetRandom(bool);
	void SetSingle(bool);
	void SetConsume(bool);
	void SetCrossfade(unsigned);
	void SetVolume(unsigned);
	
	std::string GetReplayGainMode();
	void SetReplayGainMode(ReplayGainMode);
	
	bool SetPriority(const Song &s, int prio);
	
	int AddSong(const std::string &, int = -1); // returns id of added song
	int AddSong(const Song &, int = -1); // returns id of added song
	bool AddRandomTag(mpd_tag_type, size_t);
	bool AddRandomSongs(size_t);
	bool Add(const std::string &path);
	bool Delete(unsigned);
	bool DeleteID(unsigned);
	bool PlaylistDelete(const std::string &, unsigned);
	void StartCommandsList();
	bool CommitCommandsList();
	
	bool DeletePlaylist(const std::string &);
	bool LoadPlaylist(const std::string &name);
	int SavePlaylist(const std::string &);
	bool ClearPlaylist(const std::string &);
	void AddToPlaylist(const std::string &, const Song &);
	void AddToPlaylist(const std::string &, const std::string &);
	bool PlaylistMove(const std::string &, int, int);
	bool Rename(const std::string &, const std::string &);
	
	void StartSearch(bool);
	void StartFieldSearch(mpd_tag_type);
	void StartFieldSearchMTime(mpd_tag_type, bool);
	void AddSearch(mpd_tag_type, const std::string &) const;
	void AddSearchAny(const std::string &str) const;
	void AddSearchURI(const std::string &str) const;
	void CommitSearchSongs(SongConsumer f);
	void CommitSearchTags(StringConsumer f);
	TagMTimeList CommitSearchTagsMTime();
	
	void GetPlaylists(StringConsumer f);
	void GetList(mpd_tag_type type, StringConsumer f);
	TagMTimeList GetListMTime(mpd_tag_type, bool);
	void GetDirectory(const std::string &directory, ItemConsumer f);
	void GetDirectoryRecursive(const std::string &directory, SongConsumer f);
	void GetSongs(const std::string &directory, SongConsumer f);
	void GetDirectories(const std::string &directory, StringConsumer f);
	
	void GetOutputs(OutputConsumer f);
	bool EnableOutput(int);
	bool DisableOutput(int);
	
	void GetURLHandlers(StringConsumer f);
	void GetTagTypes(StringConsumer f);
	
private:
	void GoIdle();
	int GoBusy();
	
	int CheckForErrors();

	mpd_connection *itsConnection;
	bool isCommandsListEnabled;
	
	std::string itsErrorMessage;
	
	int itsFD;
	bool isIdle;
	bool isIdleEnabled;
	bool itsIdleBlocked;
	bool supportsIdle;
	bool hasData;
	
	std::string itsHost;
	int itsPort;
	int itsTimeout;
	std::string itsPassword;
	
	mpd_status *itsCurrentStatus;
	mpd_status *itsOldStatus;
	
	unsigned itsElapsed;
	time_t itsElapsedTimer[2];
	
	StatusChanges itsChanges;
	
	StatusUpdater itsUpdater;
	void *itsStatusUpdaterUserdata;
	ErrorHandler itsErrorHandler;
	void *itsErrorHandlerUserdata;
	
	mpd_tag_type itsSearchedField;
	bool itsSearchFieldMTime;
};

}

extern MPD::Connection Mpd;

#endif
