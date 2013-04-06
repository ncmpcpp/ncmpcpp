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

#ifndef NCMPCPP_MPDPP_H
#define NCMPCPP_MPDPP_H

#include <cassert>
#include <exception>
#include <set>
#include <vector>

#include <mpd/client.h>
#include "song.h"

namespace MPD {//
	
enum ItemType { itDirectory, itSong, itPlaylist };
enum PlayerState { psUnknown, psStop, psPlay, psPause };
enum ReplayGainMode { rgmOff, rgmTrack, rgmAlbum };

struct ClientError: public std::exception
{
	ClientError(mpd_error code_, std::string msg, bool clearable_)
	: m_code(code_), m_msg(msg), m_clearable(clearable_) { }
	
	virtual const char *what() const noexcept { return m_msg.c_str(); }
	mpd_error code() const { return m_code; }
	bool clearable() const { return m_clearable; }
	
private:
	mpd_error m_code;
	std::string m_msg;
	bool m_clearable;
};

struct ServerError: public std::exception
{
	ServerError(mpd_server_error code_, std::string msg, bool clearable_)
	: m_code(code_), m_msg(msg), m_clearable(clearable_) { }
	
	virtual const char *what() const noexcept { return m_msg.c_str(); }
	mpd_server_error code() const { return m_code; }
	bool clearable() const { return m_clearable; }
	
private:
	mpd_server_error m_code;
	std::string m_msg;
	bool m_clearable;
};

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

struct Status
{
	friend class Connection;
	
	Status() { }
	
	void clear();
	bool empty() const;
	
	int volume() const;
	bool repeat() const;
	bool random() const;
	bool single() const;
	bool consume() const;
	unsigned playlistLength() const;
	unsigned playlistVersion() const;
	PlayerState playerState() const;
	unsigned crossfade() const;
	int currentSongPosition() const;
	int currentSongID() const;
	int nextSongPosition() const;
	int nextSongID() const;
	unsigned elapsedTime() const;
	unsigned totalTime() const;
	unsigned kbps() const;
	unsigned updateID() const;
	const char *error() const;
	
private:
	Status(mpd_status *status) : m_status(status, mpd_status_free) { }
	
	std::shared_ptr<mpd_status> m_status;
};

struct Item
{
	std::shared_ptr<Song> song;
	ItemType type;
	std::string name;
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

typedef std::vector<Item> ItemList;
typedef std::vector<std::string> StringList;
typedef std::vector<Output> OutputList;

class Connection
{
	typedef void (*ErrorHandler) (Connection *, int, const char *, void *);
	
	typedef std::function<void(Item &&)> ItemConsumer;
	typedef std::function<void(Output &&)> OutputConsumer;
	typedef std::function<void(Song &&)> SongConsumer;
	typedef std::function<void(std::string &&)> StringConsumer;
	
public:
	Connection();
	~Connection();
	
	void Connect();
	bool Connected() const;
	void Disconnect();
	
	const std::string &GetHostname() { return m_host; }
	int GetPort() { return m_port; }
	
	unsigned Version() const;
	
	int GetFD() const { return m_fd; }
	
	void SetHostname(const std::string &);
	void SetPort(int port) { m_port = port; }
	void SetTimeout(int timeout) { m_timeout = timeout; }
	void SetPassword(const std::string &password) { m_password = password; }
	void SendPassword();
	
	Statistics getStatistics();
	Status getStatus();
	
	void UpdateDirectory(const std::string &);
	
	void Play();
	void Play(int);
	void PlayID(int);
	void Pause(bool);
	void Toggle();
	void Stop();
	void Next();
	void Prev();
	void Move(unsigned int from, unsigned int to);
	void Swap(unsigned, unsigned);
	void Seek(unsigned);
	void Shuffle();
	void ClearMainPlaylist();
	
	void GetPlaylistChanges(unsigned, SongConsumer f);
	
	Song GetCurrentlyPlayingSong();
	Song GetSong(const std::string &);
	void GetPlaylistContent(const std::string &name, SongConsumer f);
	
	void GetSupportedExtensions(std::set<std::string> &);
	
	void SetRepeat(bool);
	void SetRandom(bool);
	void SetSingle(bool);
	void SetConsume(bool);
	void SetCrossfade(unsigned);
	void SetVolume(unsigned int vol);
	
	std::string GetReplayGainMode();
	void SetReplayGainMode(ReplayGainMode);
	
	void SetPriority(const MPD::Song &s, int prio);
	
	int AddSong(const std::string &, int = -1); // returns id of added song
	int AddSong(const Song &, int = -1); // returns id of added song
	bool AddRandomTag(mpd_tag_type, size_t);
	bool AddRandomSongs(size_t);
	void Add(const std::string &path);
	void Delete(unsigned int pos);
	void PlaylistDelete(const std::string &playlist, unsigned int pos);
	void StartCommandsList();
	void CommitCommandsList();
	
	void DeletePlaylist(const std::string &name);
	void LoadPlaylist(const std::string &name);
	void SavePlaylist(const std::string &);
	void ClearPlaylist(const std::string &playlist);
	void AddToPlaylist(const std::string &, const Song &);
	void AddToPlaylist(const std::string &, const std::string &);
	void PlaylistMove(const std::string &path, int from, int to);
	void Rename(const std::string &from, const std::string &to);
	
	void StartSearch(bool);
	void StartFieldSearch(mpd_tag_type);
	void AddSearch(mpd_tag_type item, const std::string &str) const;
	void AddSearchAny(const std::string &str) const;
	void AddSearchURI(const std::string &str) const;
	void CommitSearchSongs(SongConsumer f);
	void CommitSearchTags(StringConsumer f);
	
	void GetPlaylists(StringConsumer f);
	void GetList(mpd_tag_type type, StringConsumer f);
	void GetDirectory(const std::string &directory, ItemConsumer f);
	void GetDirectoryRecursive(const std::string &directory, SongConsumer f);
	void GetSongs(const std::string &directory, SongConsumer f);
	void GetDirectories(const std::string &directory, StringConsumer f);
	
	void GetOutputs(OutputConsumer f);
	void EnableOutput(int id);
	void DisableOutput(int id);
	
	void GetURLHandlers(StringConsumer f);
	void GetTagTypes(StringConsumer f);
	
	void idle();
	int noidle();
	
private:
	
	void checkConnection() const;
	void prechecks();
	void prechecksNoCommandsList();
	void checkErrors() const;

	mpd_connection *m_connection;
	bool m_command_list_active;
	
	int m_fd;
	bool m_idle;
	
	std::string m_host;
	int m_port;
	int m_timeout;
	std::string m_password;
	
	mpd_tag_type m_searched_field;
};

}

extern MPD::Connection Mpd;
extern MPD::Status MpdStatus;

#endif // NCMPCPP_MPDPP_H
