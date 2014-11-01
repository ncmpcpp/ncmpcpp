/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

namespace MPD {

enum ItemType { itDirectory, itSong, itPlaylist };
enum PlayerState { psUnknown, psStop, psPlay, psPause };
enum ReplayGainMode { rgmOff, rgmTrack, rgmAlbum };

struct ClientError: public std::exception
{
	ClientError(mpd_error code_, std::string msg, bool clearable_)
	: m_code(code_), m_msg(msg), m_clearable(clearable_) { }
	virtual ~ClientError() noexcept { }
	
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
	virtual ~ServerError() noexcept { }
	
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
	
	bool empty() const { return m_stats.get() == nullptr; }
	
	unsigned artists() const { return mpd_stats_get_number_of_artists(m_stats.get()); }
	unsigned albums() const { return mpd_stats_get_number_of_albums(m_stats.get()); }
	unsigned songs() const { return mpd_stats_get_number_of_songs(m_stats.get()); }
	unsigned long playTime() const { return mpd_stats_get_play_time(m_stats.get()); }
	unsigned long uptime() const { return mpd_stats_get_uptime(m_stats.get()); }
	unsigned long dbUpdateTime() const { return mpd_stats_get_db_update_time(m_stats.get()); }
	unsigned long dbPlayTime() const { return mpd_stats_get_db_play_time(m_stats.get()); }
	
private:
	Statistics(mpd_stats *stats) : m_stats(stats, mpd_stats_free) { }
	
	std::shared_ptr<mpd_stats> m_stats;
};

struct Status
{
	friend class Connection;
	
	Status() { }
	
	void clear() { m_status.reset(); }
	bool empty() const { return m_status.get() == nullptr; }
	
	int volume() const { return mpd_status_get_volume(m_status.get()); }
	bool repeat() const { return mpd_status_get_repeat(m_status.get()); }
	bool random() const { return mpd_status_get_random(m_status.get()); }
	bool single() const { return mpd_status_get_single(m_status.get()); }
	bool consume() const { return mpd_status_get_consume(m_status.get()); }
	unsigned playlistLength() const { return mpd_status_get_queue_length(m_status.get()); }
	unsigned playlistVersion() const { return mpd_status_get_queue_version(m_status.get()); }
	PlayerState playerState() const { return PlayerState(mpd_status_get_state(m_status.get())); }
	unsigned crossfade() const { return mpd_status_get_crossfade(m_status.get()); }
	int currentSongPosition() const { return mpd_status_get_song_pos(m_status.get()); }
	int currentSongID() const { return mpd_status_get_song_id(m_status.get()); }
	int nextSongPosition() const { return mpd_status_get_next_song_pos(m_status.get()); }
	int nextSongID() const { return mpd_status_get_next_song_id(m_status.get()); }
	unsigned elapsedTime() const { return mpd_status_get_elapsed_time(m_status.get()); }
	unsigned totalTime() const { return mpd_status_get_total_time(m_status.get()); }
	unsigned kbps() const { return mpd_status_get_kbit_rate(m_status.get()); }
	unsigned updateID() const { return mpd_status_get_update_id(m_status.get()); }
	const char *error() const { return mpd_status_get_error(m_status.get()); }
	
private:
	Status(mpd_status *status) : m_status(status, mpd_status_free) { }
	
	std::shared_ptr<mpd_status> m_status;
};

struct Playlist
{
	Playlist() { }
	Playlist(mpd_playlist *playlist) : m_playlist(playlist, mpd_playlist_free) { }

	Playlist(const Playlist &rhs) : m_playlist(rhs.m_playlist) { }
	Playlist(Playlist &&rhs) : m_playlist(std::move(rhs.m_playlist)) { }
	Playlist &operator=(Playlist rhs)
	{
		m_playlist = std::move(rhs.m_playlist);
		return *this;
	}

	bool operator==(const Playlist &rhs)
	{
		if (empty() && rhs.empty())
			return true;
		else if (!empty() && !rhs.empty())
			return strcmp(path(), rhs.path()) == 0
			    && lastModified() == rhs.lastModified();
		else
			return false;
	}
	bool operator!=(const Playlist &rhs)
	{
		return !(*this == rhs);
	}

	const char *path() const
	{
		assert(m_playlist.get() != nullptr);
		return mpd_playlist_get_path(m_playlist.get());
	}
	time_t lastModified() const
	{
		assert(m_playlist.get() != nullptr);
		return mpd_playlist_get_last_modified(m_playlist.get());
	}

	bool empty() const { return m_playlist.get() == nullptr; }

private:
	std::shared_ptr<mpd_playlist> m_playlist;
};

struct Item
{
	std::shared_ptr<Song> song;
	ItemType type;
	std::string name;
};

struct Output
{
	Output() { }
	Output(mpd_output *output) : m_output(output, mpd_output_free) { }

	Output(const Output &rhs) : m_output(rhs.m_output) { }
	Output(Output &&rhs) : m_output(std::move(rhs.m_output)) { }
	Output &operator=(Output rhs)
	{
		m_output = std::move(rhs.m_output);
		return *this;
	}

	bool operator==(const Output &rhs) const
	{
		if (empty() && rhs.empty())
			return true;
		else if (!empty() && !rhs.empty())
			return id() == rhs.id()
			    && strcmp(name(), rhs.name()) == 0
			    && enabled() == rhs.enabled();
		else
			return false;
	}
	bool operator!=(const Output &rhs) const
	{
		return !(*this == rhs);
	}

	unsigned id() const
	{
		assert(m_output.get() != nullptr);
		return mpd_output_get_id(m_output.get());
	}
	const char *name() const
	{
		assert(m_output.get() != nullptr);
		return mpd_output_get_name(m_output.get());
	}
	bool enabled() const
	{
		assert(m_output.get() != nullptr);
		return mpd_output_get_enabled(m_output.get());
	}

	bool empty() const { return m_output.get() == nullptr; }

private:
	std::shared_ptr<mpd_output> m_output;
};

typedef std::vector<Item> ItemList;
typedef std::vector<std::string> StringList;
typedef std::vector<Output> OutputList;

template <typename DestT, typename SourceT>
struct Iterator : std::iterator<std::forward_iterator_tag, DestT>
{
	typedef SourceT *(*SourceFetcher)(mpd_connection *);

	friend class Connection;

	Iterator() : m_connection(nullptr), m_fetch_source(nullptr) { }
	~Iterator()
	{
		if (m_connection != nullptr)
			finish();
	}

	void finish()
	{
		// clean up
		assert(m_connection != nullptr);
		mpd_response_finish(m_connection);
		m_object = DestT();
		m_connection = nullptr;
	}

	DestT &operator*() const
	{
		assert(m_connection != nullptr);
		if (m_object.empty())
			throw std::runtime_error("empty object");
		return const_cast<DestT &>(m_object);
	}
	DestT *operator->() const
	{
		return &**this;
	}

	Iterator &operator++()
	{
		assert(m_connection != nullptr);
		assert(m_fetch_source != nullptr);
		auto src = m_fetch_source(m_connection);
		if (src != nullptr)
			m_object = DestT(src);
		else
			finish();
		return *this;
	}
	Iterator operator++(int)
	{
		Iterator it(*this);
		++*this;
		return it;
	}

	bool operator==(const Iterator &rhs)
	{
		return m_connection == rhs.m_connection
		    && m_object == rhs.m_object;
	}
	bool operator!=(const Iterator &rhs)
	{
		return !(*this == rhs);
	}

private:
	Iterator(mpd_connection *connection, SourceFetcher fetch_source)
	: m_connection(connection), m_fetch_source(fetch_source)
	{
		// get the first element
		++*this;
	}

	mpd_connection *m_connection;
	SourceFetcher m_fetch_source;
	DestT m_object;
};

typedef Iterator<Song, mpd_song> SongIterator;
typedef Iterator<Output, mpd_output> OutputIterator;

class Connection
{
	typedef std::function<void(Item)> ItemConsumer;
	typedef std::function<void(Output)> OutputConsumer;
	typedef std::function<void(Song)> SongConsumer;
	typedef std::function<void(std::string)> StringConsumer;
	
public:
	Connection();
	
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
	void Seek(unsigned int pos, unsigned int where);
	void Shuffle();
	void ClearMainPlaylist();
	
	SongIterator GetPlaylistChanges(unsigned);
	
	Song GetCurrentSong();
	Song GetSong(const std::string &);
	SongIterator GetPlaylistContent(const std::string &name);
	SongIterator GetPlaylistContentNoInfo(const std::string &name);
	
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
	SongIterator CommitSearchSongs();
	void CommitSearchTags(StringConsumer f);
	
	void GetPlaylists(StringConsumer f);
	void GetList(mpd_tag_type type, StringConsumer f);
	void GetDirectory(const std::string &directory, ItemConsumer f);
	void GetDirectoryRecursive(const std::string &directory, SongConsumer f);
	SongIterator GetSongs(const std::string &directory);
	void GetDirectories(const std::string &directory, StringConsumer f);
	
	OutputIterator GetOutputs();
	void EnableOutput(int id);
	void DisableOutput(int id);
	
	void GetURLHandlers(StringConsumer f);
	void GetTagTypes(StringConsumer f);
	
	void idle();
	int noidle();
	
private:
	struct ConnectionDeleter {
		void operator()(mpd_connection *connection) {
			mpd_connection_free(connection);
		}
	};

	void checkConnection() const;
	void prechecks();
	void prechecksNoCommandsList();
	void checkErrors() const;

	std::unique_ptr<mpd_connection, ConnectionDeleter> m_connection;
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

#endif // NCMPCPP_MPDPP_H
