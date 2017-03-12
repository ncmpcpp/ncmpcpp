/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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
#include <random>
#include <set>
#include <vector>

#include <mpd/client.h>
#include "song.h"

namespace MPD {

void checkConnectionErrors(mpd_connection *conn);

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
	friend struct Connection;
	
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
	friend struct Connection;
	
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

struct Directory
{
	Directory()
	: m_last_modified(0)
	{ }
	Directory(const mpd_directory *directory)
	{
		assert(directory != nullptr);
		m_path = mpd_directory_get_path(directory);
		m_last_modified = mpd_directory_get_last_modified(directory);
	}
	Directory(std::string path_, time_t last_modified = 0)
	: m_path(std::move(path_))
	, m_last_modified(last_modified)
	{ }

	bool operator==(const Directory &rhs) const
	{
		return m_path == rhs.m_path
		    && m_last_modified == rhs.m_last_modified;
	}
	bool operator!=(const Directory &rhs) const
	{
		return !(*this == rhs);
	}

	const std::string &path() const
	{
		return m_path;
	}
	time_t lastModified() const
	{
		return m_last_modified;
	}

private:
	std::string m_path;
	time_t m_last_modified;
};

struct Playlist
{
	Playlist()
	: m_last_modified(0)
	{ }
	Playlist(const mpd_playlist *playlist)
	{
		assert(playlist != nullptr);
		m_path = mpd_playlist_get_path(playlist);
		m_last_modified = mpd_playlist_get_last_modified(playlist);
	}
	Playlist(std::string path_, time_t last_modified = 0)
	: m_path(std::move(path_))
	, m_last_modified(last_modified)
	{
		if (m_path.empty())
			throw std::runtime_error("empty path");
	}

	bool operator==(const Playlist &rhs) const
	{
		return m_path == rhs.m_path
		    && m_last_modified == rhs.m_last_modified;
	}
	bool operator!=(const Playlist &rhs) const
	{
		return !(*this == rhs);
	}

	const std::string &path() const
	{
		return m_path;
	}
	time_t lastModified() const
	{
		return m_last_modified;
	}

private:
	std::string m_path;
	time_t m_last_modified;
};

struct Item
{
	enum class Type { Directory, Song, Playlist };

	Item(mpd_entity *entity)
	{
		assert(entity != nullptr);
		switch (mpd_entity_get_type(entity))
		{
			case MPD_ENTITY_TYPE_DIRECTORY:
				m_type = Type::Directory;
				m_directory = Directory(mpd_entity_get_directory(entity));
				break;
			case MPD_ENTITY_TYPE_SONG:
				m_type = Type::Song;
				m_song = Song(mpd_song_dup(mpd_entity_get_song(entity)));
				break;
			case MPD_ENTITY_TYPE_PLAYLIST:
				m_type = Type::Playlist;
				m_playlist = Playlist(mpd_entity_get_playlist(entity));
				break;
			default:
				throw std::runtime_error("unknown mpd_entity type");
		}
		mpd_entity_free(entity);
	}
	Item(Directory directory_)
	: m_type(Type::Directory)
	, m_directory(std::move(directory_))
	{ }
	Item(Song song_)
	: m_type(Type::Song)
	, m_song(std::move(song_))
	{ }
	Item(Playlist playlist_)
	: m_type(Type::Playlist)
	, m_playlist(std::move(playlist_))
	{ }

	bool operator==(const Item &rhs) const
	{
		return m_directory == rhs.m_directory
		    && m_song == rhs.m_song
		    && m_playlist == rhs.m_playlist;
	}
	bool operator!=(const Item &rhs) const
	{
		return !(*this == rhs);
	}

	Type type() const
	{
		return m_type;
	}

	Directory &directory()
	{
		return const_cast<Directory &>(
			static_cast<const Item &>(*this).directory());
	}
	Song &song()
	{
		return const_cast<Song &>(
			static_cast<const Item &>(*this).song());
	}
	Playlist &playlist()
	{
		return const_cast<Playlist &>(
			static_cast<const Item &>(*this).playlist());
	}

	const Directory &directory() const
	{
		assert(m_type == Type::Directory);
		return m_directory;
	}
	const Song &song() const
	{
		assert(m_type == Type::Song);
		return m_song;
	}
	const Playlist &playlist() const
	{
		assert(m_type == Type::Playlist);
		return m_playlist;
	}

private:
	Type m_type;
	Directory m_directory;
	Song m_song;
	Playlist m_playlist;
};

struct Output
{
	Output() { }
	Output(mpd_output *output)
	: m_output(output, mpd_output_free)
	{ }

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

template <typename ObjectT>
struct Iterator: std::iterator<std::input_iterator_tag, ObjectT>
{
	// shared state of the iterator
	struct State
	{
		friend Iterator;

		typedef std::function<bool(State &)> Fetcher;

		State(mpd_connection *connection_, Fetcher fetcher)
		: m_connection(connection_)
		, m_fetcher(fetcher)
		{
			assert(m_connection != nullptr);
			assert(m_fetcher != nullptr);
		}
		~State()
		{
			mpd_response_finish(m_connection);
		}

		mpd_connection *connection() const
		{
			return m_connection;
		}

		void setObject(ObjectT object)
		{
			if (hasObject())
				*m_object = std::move(object);
			else
				m_object.reset(new ObjectT(std::move(object)));
		}

	private:
		bool operator==(const State &rhs) const
		{
			return m_connection == rhs.m_connection
			    && m_object == m_object;
		}
		bool operator!=(const State &rhs) const
		{
			return !(*this == rhs);
		}

		bool fetch()
		{
			return m_fetcher(*this);
		}
		ObjectT &getObject() const
		{
			return *m_object;
		}
		bool hasObject() const
		{
			return m_object.get() != nullptr;
		}

		mpd_connection *m_connection;
		Fetcher m_fetcher;
		std::unique_ptr<ObjectT> m_object;
	};

	Iterator()
	: m_state(nullptr)
	{ }
	Iterator(mpd_connection *connection, typename State::Fetcher fetcher)
	: m_state(std::make_shared<State>(connection, std::move(fetcher)))
	{
		// get the first element
		++*this;
	}
	~Iterator()
	{
		if (m_state)
			checkConnectionErrors(m_state->connection());
	}

	void finish()
	{
		assert(m_state);
		// check errors and change the iterator into end iterator
		checkConnectionErrors(m_state->connection());
		m_state = nullptr;
	}

	ObjectT &operator*() const
	{
		if (!m_state)
			throw std::runtime_error("no object associated with the iterator");
		assert(m_state->hasObject());
		return m_state->getObject();
	}
	ObjectT *operator->() const
	{
		return &**this;
	}

	Iterator &operator++()
	{
		assert(m_state);
		if (!m_state->fetch())
			finish();
		return *this;
	}
	Iterator operator++(int)
	{
		Iterator it(*this);
		++*this;
		return it;
	}

	bool operator==(const Iterator &rhs) const
	{
		return m_state == rhs.m_state;
	}
	bool operator!=(const Iterator &rhs) const
	{
		return !(*this == rhs);
	}

private:
	std::shared_ptr<State> m_state;
};

typedef Iterator<Directory> DirectoryIterator;
typedef Iterator<Item> ItemIterator;
typedef Iterator<Output> OutputIterator;
typedef Iterator<Playlist> PlaylistIterator;
typedef Iterator<Song> SongIterator;
typedef Iterator<std::string> StringIterator;

struct Connection
{
	typedef std::function<void(int)> NoidleCallback;

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
	void ShuffleRange(unsigned start, unsigned end);
	void ClearMainPlaylist();
	
	SongIterator GetPlaylistChanges(unsigned);
	
	Song GetCurrentSong();
	Song GetSong(const std::string &);
	SongIterator GetPlaylistContent(const std::string &name);
	SongIterator GetPlaylistContentNoInfo(const std::string &name);
	
	StringIterator GetSupportedExtensions();
	
	void SetRepeat(bool);
	void SetRandom(bool);
	void SetSingle(bool);
	void SetConsume(bool);
	void SetCrossfade(unsigned);
	void SetVolume(unsigned int vol);
	void ChangeVolume(int change);

	std::string GetReplayGainMode();
	void SetReplayGainMode(ReplayGainMode);
	
	void SetPriority(const MPD::Song &s, int prio);
	
	int AddSong(const std::string &, int = -1); // returns id of added song
	int AddSong(const Song &, int = -1); // returns id of added song
	bool AddRandomTag(mpd_tag_type, size_t, std::mt19937 &rng);
	bool AddRandomSongs(size_t number, std::mt19937 &rng);
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
	
	PlaylistIterator GetPlaylists();
	StringIterator GetList(mpd_tag_type type);
	ItemIterator GetDirectory(const std::string &directory);
	SongIterator GetDirectoryRecursive(const std::string &directory);
	SongIterator GetSongs(const std::string &directory);
	DirectoryIterator GetDirectories(const std::string &directory);
	
	OutputIterator GetOutputs();
	void EnableOutput(int id);
	void DisableOutput(int id);
	
	StringIterator GetURLHandlers();
	StringIterator GetTagTypes();
	
	void idle();
	int noidle();
	void setNoidleCallback(NoidleCallback callback);
	
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

	NoidleCallback m_noidle_callback;
	std::unique_ptr<mpd_connection, ConnectionDeleter> m_connection;
	bool m_command_list_active;
	
	int m_fd;
	bool m_idle;
	
	std::string m_host;
	int m_port;
	int m_timeout;
	std::string m_password;
};

}

extern MPD::Connection Mpd;

#endif // NCMPCPP_MPDPP_H
