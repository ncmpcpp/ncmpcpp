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

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <map>

#include "charset.h"
#include "mpdpp.h"

MPD::Connection Mpd;

namespace {

const char *mpdDirectory(const std::string &directory)
{
	// MPD <= 0.19 accepts "/" for a root directory whereas later
	// versions do not, so provide a compatibility layer.
	if (directory == "/")
		return "";
	else
		return directory.c_str();
}

template <typename ObjectT, typename SourceT>
std::function<bool(typename MPD::Iterator<ObjectT>::State &)>
defaultFetcher(SourceT *(fetcher)(mpd_connection *))
{
	return [fetcher](typename MPD::Iterator<ObjectT>::State &state) {
		auto src = fetcher(state.connection());
		if (src != nullptr)
		{
			state.setObject(src);
			return true;
		}
		else
			return false;
	};
}

bool fetchItemSong(MPD::SongIterator::State &state)
{
	auto src = mpd_recv_entity(state.connection());
	while (src != nullptr && mpd_entity_get_type(src) != MPD_ENTITY_TYPE_SONG)
	{
		mpd_entity_free(src);
		src = mpd_recv_entity(state.connection());
	}
	if (src != nullptr)
	{
		state.setObject(mpd_song_dup(mpd_entity_get_song(src)));
		mpd_entity_free(src);
		return true;
	}
	else
		return false;
}

}

namespace MPD {

void checkConnectionErrors(mpd_connection *conn)
{
	mpd_error code = mpd_connection_get_error(conn);
	if (code != MPD_ERROR_SUCCESS)
	{
		std::string msg = mpd_connection_get_error_message(conn);
		if (code == MPD_ERROR_SERVER)
		{
			mpd_server_error server_code = mpd_connection_get_server_error(conn);
			bool clearable = mpd_connection_clear_error(conn);
			throw ServerError(server_code, msg, clearable);
		}
		else
		{
			bool clearable = mpd_connection_clear_error(conn);
			throw ClientError(code, msg, clearable);
		}
	}
}

Connection::Connection() : m_connection(nullptr),
				m_command_list_active(false),
				m_idle(false),
				m_host("localhost"),
				m_port(6600),
				m_timeout(15)
{
}

void Connection::Connect()
{
	assert(!m_connection);
	try
	{
		m_connection.reset(mpd_connection_new(m_host.c_str(), m_port, m_timeout * 1000));
		checkErrors();
		if (!m_password.empty())
			SendPassword();
		m_fd = mpd_connection_get_fd(m_connection.get());
		checkErrors();
	}
	catch (MPD::ClientError &e)
	{
		Disconnect();
		throw e;
	}
}

bool Connection::Connected() const
{
	return m_connection.get() != nullptr;
}

void Connection::Disconnect()
{
	m_connection = nullptr;
	m_command_list_active = false;
	m_idle = false;
}

unsigned Connection::Version() const
{
	return m_connection ? mpd_connection_get_server_version(m_connection.get())[1] : 0;
}

void Connection::SetHostname(const std::string &host)
{
	size_t at = host.find("@");
	if (at != std::string::npos)
	{
		m_password = host.substr(0, at);
		m_host = host.substr(at+1);
	}
	else
		m_host = host;
}

void Connection::SendPassword()
{
	assert(m_connection);
	noidle();
	assert(!m_command_list_active);
	mpd_run_password(m_connection.get(), m_password.c_str());
	checkErrors();
}

void Connection::idle()
{
	checkConnection();
	if (!m_idle)
	{
		mpd_send_idle(m_connection.get());
		checkErrors();
	}
	m_idle = true;
}

int Connection::noidle()
{
	checkConnection();
	int flags = 0;
	if (m_idle && mpd_send_noidle(m_connection.get()))
	{
		m_idle = false;
		flags = mpd_recv_idle(m_connection.get(), true);
		mpd_response_finish(m_connection.get());
		checkErrors();
	}
	return flags;
}

void Connection::setNoidleCallback(NoidleCallback callback)
{
	m_noidle_callback = std::move(callback);
}

Statistics Connection::getStatistics()
{
	prechecks();
	mpd_stats *stats = mpd_run_stats(m_connection.get());
	checkErrors();
	return Statistics(stats);
}

Status Connection::getStatus()
{
	prechecks();
	mpd_status *status = mpd_run_status(m_connection.get());
	checkErrors();
	return Status(status);
}

void Connection::UpdateDirectory(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_run_update(m_connection.get(), path.c_str());
	checkErrors();
}

void Connection::Play()
{
	prechecksNoCommandsList();
	mpd_run_play(m_connection.get());
	checkErrors();
}

void Connection::Play(int pos)
{
	prechecksNoCommandsList();
	mpd_run_play_pos(m_connection.get(), pos);
	checkErrors();
}

void Connection::PlayID(int id)
{
	prechecksNoCommandsList();
	mpd_run_play_id(m_connection.get(), id);
	checkErrors();
}

void Connection::Pause(bool state)
{
	prechecksNoCommandsList();
	mpd_run_pause(m_connection.get(), state);
	checkErrors();
}

void Connection::Toggle()
{
	prechecksNoCommandsList();
	mpd_run_toggle_pause(m_connection.get());
	checkErrors();
}

void Connection::Stop()
{
	prechecksNoCommandsList();
	mpd_run_stop(m_connection.get());
	checkErrors();
}

void Connection::Next()
{
	prechecksNoCommandsList();
	mpd_run_next(m_connection.get());
	checkErrors();
}

void Connection::Prev()
{
	prechecksNoCommandsList();
	mpd_run_previous(m_connection.get());
	checkErrors();
}

void Connection::Move(unsigned from, unsigned to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_move(m_connection.get(), from, to);
	else
	{
		mpd_run_move(m_connection.get(), from, to);
		checkErrors();
	}
}

void Connection::Swap(unsigned from, unsigned to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_swap(m_connection.get(), from, to);
	else
	{
		mpd_run_swap(m_connection.get(), from, to);
		checkErrors();
	}
}

void Connection::Seek(unsigned pos, unsigned where)
{
	prechecksNoCommandsList();
	mpd_run_seek_pos(m_connection.get(), pos, where);
	checkErrors();
}

void Connection::Shuffle()
{
	prechecksNoCommandsList();
	mpd_run_shuffle(m_connection.get());
	checkErrors();
}

void Connection::ShuffleRange(unsigned start, unsigned end)
{
	prechecksNoCommandsList();
	mpd_run_shuffle_range(m_connection.get(), start, end);
	checkErrors();
}

void Connection::ClearMainPlaylist()
{
	prechecksNoCommandsList();
	mpd_run_clear(m_connection.get());
	checkErrors();
}

void Connection::ClearPlaylist(const std::string &playlist)
{
	prechecksNoCommandsList();
	mpd_run_playlist_clear(m_connection.get(), playlist.c_str());
	checkErrors();
}

void Connection::AddToPlaylist(const std::string &path, const Song &s)
{
	AddToPlaylist(path, s.getURI());
}

void Connection::AddToPlaylist(const std::string &path, const std::string &file)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_playlist_add(m_connection.get(), path.c_str(), file.c_str());
	else
	{
		mpd_run_playlist_add(m_connection.get(), path.c_str(), file.c_str());
		checkErrors();
	}
}

void Connection::PlaylistMove(const std::string &path, int from, int to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_playlist_move(m_connection.get(), path.c_str(), from, to);
	else
	{
		mpd_send_playlist_move(m_connection.get(), path.c_str(), from, to);
		mpd_response_finish(m_connection.get());
		checkErrors();
	}
}

void Connection::Rename(const std::string &from, const std::string &to)
{
	prechecksNoCommandsList();
	mpd_run_rename(m_connection.get(), from.c_str(), to.c_str());
	checkErrors();
}

SongIterator Connection::GetPlaylistChanges(unsigned version)
{
	prechecksNoCommandsList();
	mpd_send_queue_changes_meta(m_connection.get(), version);
	checkErrors();
	return SongIterator(m_connection.get(), defaultFetcher<Song>(mpd_recv_song));
}

Song Connection::GetCurrentSong()
{
	prechecksNoCommandsList();
	mpd_send_current_song(m_connection.get());
	mpd_song *s = mpd_recv_song(m_connection.get());
	mpd_response_finish(m_connection.get());
	checkErrors();
	// currentsong doesn't return error if there is no playing song.
	if (s == nullptr)
		return Song();
	else
		return Song(s);
}

Song Connection::GetSong(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_send_list_all_meta(m_connection.get(), path.c_str());
	mpd_song *s = mpd_recv_song(m_connection.get());
	mpd_response_finish(m_connection.get());
	checkErrors();
	return Song(s);
}

SongIterator Connection::GetPlaylistContent(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_send_list_playlist_meta(m_connection.get(), path.c_str());
	SongIterator result(m_connection.get(), defaultFetcher<Song>(mpd_recv_song));
	checkErrors();
	return result;
}

SongIterator Connection::GetPlaylistContentNoInfo(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_send_list_playlist(m_connection.get(), path.c_str());
	SongIterator result(m_connection.get(), defaultFetcher<Song>(mpd_recv_song));
	checkErrors();
	return result;
}

StringIterator Connection::GetSupportedExtensions()
{
	prechecksNoCommandsList();
	mpd_send_command(m_connection.get(), "decoders", NULL);
	checkErrors();
	return StringIterator(m_connection.get(), [](StringIterator::State &state) {
		auto src = mpd_recv_pair_named(state.connection(), "suffix");
		if (src != nullptr)
		{
			state.setObject(src->value);
			mpd_return_pair(state.connection(), src);
			return true;
		}
		else
			return false;
	});
}

void Connection::SetRepeat(bool mode)
{
	prechecksNoCommandsList();
	mpd_run_repeat(m_connection.get(), mode);
	checkErrors();
}

void Connection::SetRandom(bool mode)
{
	prechecksNoCommandsList();
	mpd_run_random(m_connection.get(), mode);
	checkErrors();
}

void Connection::SetSingle(bool mode)
{
	prechecksNoCommandsList();
	mpd_run_single(m_connection.get(), mode);
	checkErrors();
}

void Connection::SetConsume(bool mode)
{
	prechecksNoCommandsList();
	mpd_run_consume(m_connection.get(), mode);
	checkErrors();
}

void Connection::SetVolume(unsigned vol)
{
	prechecksNoCommandsList();
	mpd_run_set_volume(m_connection.get(), vol);
	checkErrors();
}

void Connection::ChangeVolume(int change)
{
	prechecksNoCommandsList();
	mpd_run_change_volume(m_connection.get(), change);
	checkErrors();
}


std::string Connection::GetReplayGainMode()
{
	prechecksNoCommandsList();
	mpd_send_command(m_connection.get(), "replay_gain_status", NULL);
	std::string result;
	if (mpd_pair *pair = mpd_recv_pair_named(m_connection.get(), "replay_gain_mode"))
	{
		result = pair->value;
		mpd_return_pair(m_connection.get(), pair);
	}
	mpd_response_finish(m_connection.get());
	checkErrors();
	return result;
}

void Connection::SetReplayGainMode(ReplayGainMode mode)
{
	prechecksNoCommandsList();
	const char *rg_mode;
	switch (mode)
	{
		case rgmOff:
			rg_mode = "off";
			break;
		case rgmTrack:
			rg_mode = "track";
			break;
		case rgmAlbum:
			rg_mode = "album";
			break;
		default:
			rg_mode = "";
			break;
	}
	mpd_send_command(m_connection.get(), "replay_gain_mode", rg_mode, NULL);
	mpd_response_finish(m_connection.get());
	checkErrors();
}

void Connection::SetCrossfade(unsigned crossfade)
{
	prechecksNoCommandsList();
	mpd_run_crossfade(m_connection.get(), crossfade);
	checkErrors();
}

void Connection::SetPriority(const Song &s, int prio)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_prio_id(m_connection.get(), prio, s.getID());
	else
	{
		mpd_run_prio_id(m_connection.get(), prio, s.getID());
		checkErrors();
	}
}

int Connection::AddSong(const std::string &path, int pos)
{
	prechecks();
	int id;
	if (pos < 0)
		mpd_send_add_id(m_connection.get(), path.c_str());
	else
		mpd_send_add_id_to(m_connection.get(), path.c_str(), pos);
	if (!m_command_list_active)
	{
		id = mpd_recv_song_id(m_connection.get());
		mpd_response_finish(m_connection.get());
		checkErrors();
	}
	else
		id = 0;
	return id;
}

int Connection::AddSong(const Song &s, int pos)
{
	return AddSong((!s.isFromDatabase() ? "file://" : "") + s.getURI(), pos);
}

void Connection::Add(const std::string &path)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_add(m_connection.get(), path.c_str());
	else
	{
		mpd_run_add(m_connection.get(), path.c_str());
		checkErrors();
	}
}

bool Connection::AddRandomTag(mpd_tag_type tag, size_t number, std::mt19937 &rng)
{
	std::vector<std::string> tags(
		std::make_move_iterator(GetList(tag)),
		std::make_move_iterator(StringIterator())
	);
	if (number > tags.size())
		return false;

	std::shuffle(tags.begin(), tags.end(), rng);
	auto it = tags.begin();
	for (size_t i = 0; i < number && it != tags.end(); ++i)
	{
		StartSearch(true);
		AddSearch(tag, *it++);
		std::vector<std::string> paths;
		MPD::SongIterator s = CommitSearchSongs(), end;
		for (; s != end; ++s)
			paths.push_back(s->getURI());
		StartCommandsList();
		for (const auto &path : paths)
			AddSong(path);
		CommitCommandsList();
	}
	return true;
}

bool Connection::AddRandomSongs(size_t number, std::mt19937 &rng)
{
	prechecksNoCommandsList();
	std::vector<std::string> files;
	mpd_send_list_all(m_connection.get(), "/");
	while (mpd_pair *item = mpd_recv_pair_named(m_connection.get(), "file"))
	{
		files.push_back(item->value);
		mpd_return_pair(m_connection.get(), item);
	}
	mpd_response_finish(m_connection.get());
	checkErrors();
	
	if (number > files.size())
	{
		//if (itsErrorHandler)
		//	itsErrorHandler(this, 0, "Requested number of random songs is bigger than size of your library", itsErrorHandlerUserdata);
		return false;
	}
	else
	{
		std::shuffle(files.begin(), files.end(), rng);
		StartCommandsList();
		auto it = files.begin();
		for (size_t i = 0; i < number && it != files.end(); ++i, ++it)
			AddSong(*it);
		CommitCommandsList();
	}
	return true;
}

void Connection::Delete(unsigned pos)
{
	prechecks();
	mpd_send_delete(m_connection.get(), pos);
	if (!m_command_list_active)
	{
		mpd_response_finish(m_connection.get());
		checkErrors();
	}
}

void Connection::PlaylistDelete(const std::string &playlist, unsigned pos)
{
	prechecks();
	mpd_send_playlist_delete(m_connection.get(), playlist.c_str(), pos);
	if (!m_command_list_active)
	{
		mpd_response_finish(m_connection.get());
		checkErrors();
	}
}

void Connection::StartCommandsList()
{
	prechecksNoCommandsList();
	mpd_command_list_begin(m_connection.get(), true);
	m_command_list_active = true;
	checkErrors();
}

void Connection::CommitCommandsList()
{
	prechecks();
	assert(m_command_list_active);
	mpd_command_list_end(m_connection.get());
	mpd_response_finish(m_connection.get());
	m_command_list_active = false;
	checkErrors();
}

void Connection::DeletePlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	mpd_run_rm(m_connection.get(), name.c_str());
	checkErrors();
}

void Connection::LoadPlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	mpd_run_load(m_connection.get(), name.c_str());
	checkErrors();
}

void Connection::SavePlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	mpd_send_save(m_connection.get(), name.c_str());
	mpd_response_finish(m_connection.get());
	checkErrors();
}

PlaylistIterator Connection::GetPlaylists()
{
	prechecksNoCommandsList();
	mpd_send_list_playlists(m_connection.get());
	checkErrors();
	return PlaylistIterator(m_connection.get(), defaultFetcher<Playlist>(mpd_recv_playlist));
}

StringIterator Connection::GetList(mpd_tag_type type)
{
	prechecksNoCommandsList();
	mpd_search_db_tags(m_connection.get(), type);
	mpd_search_commit(m_connection.get());
	checkErrors();
	return StringIterator(m_connection.get(), [type](StringIterator::State &state) {
		auto src = mpd_recv_pair_tag(state.connection(), type);
		if (src != nullptr)
		{
			state.setObject(src->value);
			mpd_return_pair(state.connection(), src);
			return true;
		}
		else
			return false;
	});
}

void Connection::StartSearch(bool exact_match)
{
	prechecksNoCommandsList();
	mpd_search_db_songs(m_connection.get(), exact_match);
}

void Connection::StartFieldSearch(mpd_tag_type item)
{
	prechecksNoCommandsList();
	mpd_search_db_tags(m_connection.get(), item);
}

void Connection::AddSearch(mpd_tag_type item, const std::string &str) const
{
	checkConnection();
	mpd_search_add_tag_constraint(m_connection.get(), MPD_OPERATOR_DEFAULT, item, str.c_str());
}

void Connection::AddSearchAny(const std::string &str) const
{
	checkConnection();
	mpd_search_add_any_tag_constraint(m_connection.get(), MPD_OPERATOR_DEFAULT, str.c_str());
}

void Connection::AddSearchURI(const std::string &str) const
{
	checkConnection();
	mpd_search_add_uri_constraint(m_connection.get(), MPD_OPERATOR_DEFAULT, str.c_str());
}

SongIterator Connection::CommitSearchSongs()
{
	prechecksNoCommandsList();
	mpd_search_commit(m_connection.get());
	checkErrors();
	return SongIterator(m_connection.get(), defaultFetcher<Song>(mpd_recv_song));
}

ItemIterator Connection::GetDirectory(const std::string &directory)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(m_connection.get(), mpdDirectory(directory));
	checkErrors();
	return ItemIterator(m_connection.get(), defaultFetcher<Item>(mpd_recv_entity));
}

SongIterator Connection::GetDirectoryRecursive(const std::string &directory)
{
	prechecksNoCommandsList();
	mpd_send_list_all_meta(m_connection.get(), mpdDirectory(directory));
	checkErrors();
	return SongIterator(m_connection.get(), fetchItemSong);
}

DirectoryIterator Connection::GetDirectories(const std::string &directory)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(m_connection.get(), mpdDirectory(directory));
	checkErrors();
	return DirectoryIterator(m_connection.get(), defaultFetcher<Directory>(mpd_recv_directory));
}

SongIterator Connection::GetSongs(const std::string &directory)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(m_connection.get(), mpdDirectory(directory));
	checkErrors();
	return SongIterator(m_connection.get(), defaultFetcher<Song>(mpd_recv_song));
}

OutputIterator Connection::GetOutputs()
{
	prechecksNoCommandsList();
	mpd_send_outputs(m_connection.get());
	checkErrors();
	return OutputIterator(m_connection.get(), defaultFetcher<Output>(mpd_recv_output));
}

void Connection::EnableOutput(int id)
{
	prechecksNoCommandsList();
	mpd_run_enable_output(m_connection.get(), id);
	checkErrors();
}

void Connection::DisableOutput(int id)
{
	prechecksNoCommandsList();
	mpd_run_disable_output(m_connection.get(), id);
	checkErrors();
}

StringIterator Connection::GetURLHandlers()
{
	prechecksNoCommandsList();
	mpd_send_list_url_schemes(m_connection.get());
	checkErrors();
	return StringIterator(m_connection.get(), [](StringIterator::State &state) {
		auto src = mpd_recv_pair_named(state.connection(), "handler");
		if (src != nullptr)
		{
			state.setObject(src->value);
			mpd_return_pair(state.connection(), src);
			return true;
		}
		else
			return false;
	});
}

StringIterator Connection::GetTagTypes()
{
	
	prechecksNoCommandsList();
	mpd_send_list_tag_types(m_connection.get());
	checkErrors();
	return StringIterator(m_connection.get(), [](StringIterator::State &state) {
		auto src = mpd_recv_pair_named(state.connection(), "tagtype");
		if (src != nullptr)
		{
			state.setObject(src->value);
			mpd_return_pair(state.connection(), src);
			return true;
		}
		else
			return false;
	});
}

void Connection::checkConnection() const
{
	if (!m_connection)
		throw ClientError(MPD_ERROR_STATE, "No active MPD connection", false);
}

void Connection::prechecks()
{
	checkConnection();
	int flags = noidle();
	if (flags && m_noidle_callback)
		m_noidle_callback(flags);
}

void Connection::prechecksNoCommandsList()
{
	assert(!m_command_list_active);
	prechecks();
}

void Connection::checkErrors() const
{
	checkConnectionErrors(m_connection.get());
}

}
