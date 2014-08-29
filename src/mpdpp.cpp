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

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <map>

#include "charset.h"
#include "error.h"
#include "mpdpp.h"

MPD::Connection Mpd;

namespace MPD {//

Connection::Connection() : m_connection(nullptr),
				m_command_list_active(false),
				m_idle(false),
				m_host("localhost"),
				m_port(6600),
				m_timeout(15)
{
}

Connection::~Connection()
{
	if (m_connection)
		mpd_connection_free(m_connection);
}

void Connection::Connect()
{
	assert(!m_connection);
	try
	{
		m_connection = mpd_connection_new(m_host.c_str(), m_port, m_timeout * 1000);
		checkErrors();
		if (!m_password.empty())
			SendPassword();
		m_fd = mpd_connection_get_fd(m_connection);
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
	return m_connection;
}

void Connection::Disconnect()
{
	if (m_connection)
		mpd_connection_free(m_connection);
	m_connection = nullptr;
	m_command_list_active = false;
	m_idle = false;
}

unsigned Connection::Version() const
{
	return m_connection ? mpd_connection_get_server_version(m_connection)[1] : 0;
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
	mpd_run_password(m_connection, m_password.c_str());
	checkErrors();
}

void Connection::idle()
{
	checkConnection();
	if (!m_idle)
	{
		mpd_send_idle(m_connection);
		checkErrors();
	}
	m_idle = true;
}

int Connection::noidle()
{
	checkConnection();
	int flags = 0;
	if (m_idle && mpd_send_noidle(m_connection))
	{
		m_idle = false;
		flags = mpd_recv_idle(m_connection, true);
		mpd_response_finish(m_connection);
		checkErrors();
	}
	return flags;
}

Statistics Connection::getStatistics()
{
	prechecks();
	mpd_stats *stats = mpd_run_stats(m_connection);
	checkErrors();
	return Statistics(stats);
}

Status Connection::getStatus()
{
	prechecks();
	mpd_status *status = mpd_run_status(m_connection);
	checkErrors();
	return Status(status);
}

void Connection::UpdateDirectory(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_run_update(m_connection, path.c_str());
	checkErrors();
}

void Connection::Play()
{
	prechecksNoCommandsList();
	mpd_run_play(m_connection);
	checkErrors();
}

void Connection::Play(int pos)
{
	prechecksNoCommandsList();
	mpd_run_play_pos(m_connection, pos);
	checkErrors();
}

void Connection::PlayID(int id)
{
	prechecksNoCommandsList();
	mpd_run_play_id(m_connection, id);
	checkErrors();
}

void Connection::Pause(bool state)
{
	prechecksNoCommandsList();
	mpd_run_pause(m_connection, state);
	checkErrors();
}

void Connection::Toggle()
{
	prechecksNoCommandsList();
	mpd_run_toggle_pause(m_connection);
	checkErrors();
}

void Connection::Stop()
{
	prechecksNoCommandsList();
	mpd_run_stop(m_connection);
	checkErrors();
}

void Connection::Next()
{
	prechecksNoCommandsList();
	mpd_run_next(m_connection);
	checkErrors();
}

void Connection::Prev()
{
	prechecksNoCommandsList();
	mpd_run_previous(m_connection);
	checkErrors();
}

void Connection::Move(unsigned from, unsigned to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_move(m_connection, from, to);
	else
	{
		mpd_run_move(m_connection, from, to);
		checkErrors();
	}
}

void Connection::Swap(unsigned from, unsigned to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_swap(m_connection, from, to);
	else
	{
		mpd_run_swap(m_connection, from, to);
		checkErrors();
	}
}

void Connection::Seek(unsigned pos, unsigned where)
{
	prechecksNoCommandsList();
	mpd_run_seek_pos(m_connection, pos, where);
	checkErrors();
}

void Connection::Shuffle()
{
	prechecksNoCommandsList();
	mpd_run_shuffle(m_connection);
	checkErrors();
}

void Connection::ClearMainPlaylist()
{
	prechecksNoCommandsList();
	mpd_run_clear(m_connection);
	checkErrors();
}

void Connection::ClearPlaylist(const std::string &playlist)
{
	prechecksNoCommandsList();
	mpd_run_playlist_clear(m_connection, playlist.c_str());
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
		mpd_send_playlist_add(m_connection, path.c_str(), file.c_str());
	else
	{
		mpd_run_playlist_add(m_connection, path.c_str(), file.c_str());
		checkErrors();
	}
}

void Connection::PlaylistMove(const std::string &path, int from, int to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_playlist_move(m_connection, path.c_str(), from, to);
	else
	{
		mpd_send_playlist_move(m_connection, path.c_str(), from, to);
		mpd_response_finish(m_connection);
		checkErrors();
	}
}

void Connection::Rename(const std::string &from, const std::string &to)
{
	prechecksNoCommandsList();
	mpd_run_rename(m_connection, from.c_str(), to.c_str());
	checkErrors();
}

void Connection::GetPlaylistChanges(unsigned version, SongConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_queue_changes_meta(m_connection, version);
	while (mpd_song *s = mpd_recv_song(m_connection))
		f(Song(s));
	mpd_response_finish(m_connection);
	checkErrors();
}

Song Connection::GetSong(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_send_list_all_meta(m_connection, path.c_str());
	mpd_song *s = mpd_recv_song(m_connection);
	mpd_response_finish(m_connection);
	checkErrors();
	return Song(s);
}

void Connection::GetPlaylistContent(const std::string &path, SongConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_list_playlist_meta(m_connection, path.c_str());
	while (mpd_song *s = mpd_recv_song(m_connection))
		f(Song(s));
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetPlaylistContentNoInfo(const std::string &path, SongConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_list_playlist(m_connection, path.c_str());
	while (mpd_song *s = mpd_recv_song(m_connection))
		f(Song(s));
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetSupportedExtensions(std::set<std::string> &acc)
{
	prechecksNoCommandsList();
	mpd_send_command(m_connection, "decoders", NULL);
	while (mpd_pair *pair = mpd_recv_pair_named(m_connection, "suffix"))
	{
		acc.insert(pair->value);
		mpd_return_pair(m_connection, pair);
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::SetRepeat(bool mode)
{
	prechecksNoCommandsList();
	mpd_run_repeat(m_connection, mode);
	checkErrors();
}

void Connection::SetRandom(bool mode)
{
	prechecksNoCommandsList();
	mpd_run_random(m_connection, mode);
	checkErrors();
}

void Connection::SetSingle(bool mode)
{
	prechecksNoCommandsList();
	mpd_run_single(m_connection, mode);
	checkErrors();
}

void Connection::SetConsume(bool mode)
{
	prechecksNoCommandsList();
	mpd_run_consume(m_connection, mode);
	checkErrors();
}

void Connection::SetVolume(unsigned vol)
{
	prechecksNoCommandsList();
	mpd_run_set_volume(m_connection, vol);
	checkErrors();
}

std::string Connection::GetReplayGainMode()
{
	prechecksNoCommandsList();
	mpd_send_command(m_connection, "replay_gain_status", NULL);
	std::string result;
	if (mpd_pair *pair = mpd_recv_pair_named(m_connection, "replay_gain_mode"))
	{
		result = pair->value;
		mpd_return_pair(m_connection, pair);
	}
	mpd_response_finish(m_connection);
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
	mpd_send_command(m_connection, "replay_gain_mode", rg_mode, NULL);
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::SetCrossfade(unsigned crossfade)
{
	prechecksNoCommandsList();
	mpd_run_crossfade(m_connection, crossfade);
	checkErrors();
}

void Connection::SetPriority(const Song &s, int prio)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_prio_id(m_connection, prio, s.getID());
	else
	{
		mpd_run_prio_id(m_connection, prio, s.getID());
		checkErrors();
	}
}

int Connection::AddSong(const std::string &path, int pos)
{
	prechecks();
	int id;
	if (pos < 0)
		mpd_send_add_id(m_connection, path.c_str());
	else
		mpd_send_add_id_to(m_connection, path.c_str(), pos);
	if (!m_command_list_active)
	{
		id = mpd_recv_song_id(m_connection);
		mpd_response_finish(m_connection);
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
		mpd_send_add(m_connection, path.c_str());
	else
	{
		mpd_run_add(m_connection, path.c_str());
		checkErrors();
	}
}

bool Connection::AddRandomTag(mpd_tag_type tag, size_t number)
{
	StringList tags;
	GetList(tag, [&tags](std::string tag_name) {
		tags.push_back(tag_name);
	});
	if (number > tags.size())
	{
		//if (itsErrorHandler)
		//	itsErrorHandler(this, 0, "Requested number is out of range", itsErrorHandlerUserdata);
		return false;
	}
	else
	{
		std::random_shuffle(tags.begin(), tags.end());
		auto it = tags.begin()+rand()%(tags.size()-number);
		for (size_t i = 0; i < number && it != tags.end(); ++i)
		{
			StartSearch(1);
			AddSearch(tag, *it++);
			SongList songs;
			CommitSearchSongs([&songs](MPD::Song s) {
				songs.push_back(s);
			});
			StartCommandsList();
			for (auto s = songs.begin(); s != songs.end(); ++s)
				AddSong(*s);
			CommitCommandsList();
		}
	}
	return true;
}

bool Connection::AddRandomSongs(size_t number)
{
	prechecksNoCommandsList();
	StringList files;
	mpd_send_list_all(m_connection, "/");
	while (mpd_pair *item = mpd_recv_pair_named(m_connection, "file"))
	{
		files.push_back(item->value);
		mpd_return_pair(m_connection, item);
	}
	mpd_response_finish(m_connection);
	checkErrors();
	
	if (number > files.size())
	{
		//if (itsErrorHandler)
		//	itsErrorHandler(this, 0, "Requested number of random songs is bigger than size of your library", itsErrorHandlerUserdata);
		return false;
	}
	else
	{
		std::random_shuffle(files.begin(), files.end());
		StartCommandsList();
		auto it = files.begin()+rand()%(std::max(size_t(1), files.size()-number));
		for (size_t i = 0; i < number && it != files.end(); ++i, ++it)
			AddSong(*it);
		CommitCommandsList();
	}
	return true;
}

void Connection::Delete(unsigned pos)
{
	prechecks();
	mpd_send_delete(m_connection, pos);
	if (!m_command_list_active)
	{
		mpd_response_finish(m_connection);
		checkErrors();
	}
}

void Connection::PlaylistDelete(const std::string &playlist, unsigned pos)
{
	prechecks();
	mpd_send_playlist_delete(m_connection, playlist.c_str(), pos);
	if (!m_command_list_active)
	{
		mpd_response_finish(m_connection);
		checkErrors();
	}
}

void Connection::StartCommandsList()
{
	prechecksNoCommandsList();
	mpd_command_list_begin(m_connection, true);
	m_command_list_active = true;
	checkErrors();
}

void Connection::CommitCommandsList()
{
	prechecks();
	assert(m_command_list_active);
	mpd_command_list_end(m_connection);
	mpd_response_finish(m_connection);
	m_command_list_active = false;
	checkErrors();
}

void Connection::DeletePlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	mpd_run_rm(m_connection, name.c_str());
	checkErrors();
}

void Connection::LoadPlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	mpd_run_load(m_connection, name.c_str());
	checkErrors();
}

void Connection::SavePlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	mpd_send_save(m_connection, name.c_str());
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetPlaylists(StringConsumer f)
{
	GetDirectory("/", [&f](Item &&item) {
		if (item.type == itPlaylist)
			f(std::move(item.name));
	});
}

void Connection::GetList(mpd_tag_type type, StringConsumer f)
{
	prechecksNoCommandsList();
	mpd_search_db_tags(m_connection, type);
	mpd_search_commit(m_connection);
	while (mpd_pair *item = mpd_recv_pair_tag(m_connection, type))
	{
		f(std::string(item->value));
		mpd_return_pair(m_connection, item);
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::StartSearch(bool exact_match)
{
	prechecksNoCommandsList();
	mpd_search_db_songs(m_connection, exact_match);
}

void Connection::StartFieldSearch(mpd_tag_type item)
{
	prechecksNoCommandsList();
	mpd_search_db_tags(m_connection, item);
}

void Connection::AddSearch(mpd_tag_type item, const std::string &str) const
{
	checkConnection();
	mpd_search_add_tag_constraint(m_connection, MPD_OPERATOR_DEFAULT, item, str.c_str());
}

void Connection::AddSearchAny(const std::string &str) const
{
	checkConnection();
	mpd_search_add_any_tag_constraint(m_connection, MPD_OPERATOR_DEFAULT, str.c_str());
}

void Connection::AddSearchURI(const std::string &str) const
{
	checkConnection();
	mpd_search_add_uri_constraint(m_connection, MPD_OPERATOR_DEFAULT, str.c_str());
}

void Connection::CommitSearchSongs(SongConsumer f)
{
	prechecksNoCommandsList();
	mpd_search_commit(m_connection);
	while (mpd_song *s = mpd_recv_song(m_connection))
		f(Song(s));
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::CommitSearchTags(StringConsumer f)
{
	prechecksNoCommandsList();
	mpd_search_commit(m_connection);
	while (mpd_pair *tag = mpd_recv_pair_tag(m_connection, m_searched_field))
	{
		f(std::string(tag->value));
		mpd_return_pair(m_connection, tag);
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetDirectory(const std::string &directory, ItemConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(m_connection, directory.c_str());
	while (mpd_entity *item = mpd_recv_entity(m_connection))
	{
		Item it;
		switch (mpd_entity_get_type(item))
		{
			case MPD_ENTITY_TYPE_DIRECTORY:
				it.name = mpd_directory_get_path(mpd_entity_get_directory(item));
				it.type = itDirectory;
				break;
			case MPD_ENTITY_TYPE_SONG:
				it.song = std::make_shared<Song>(Song(mpd_song_dup(mpd_entity_get_song(item))));
				it.type = itSong;
				break;
			case MPD_ENTITY_TYPE_PLAYLIST:
				it.name = mpd_playlist_get_path(mpd_entity_get_playlist(item));
				it.type = itPlaylist;
				break;
			default:
				assert(false);
		}
		mpd_entity_free(item);
		f(std::move(it));
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetDirectoryRecursive(const std::string &directory, SongConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_list_all_meta(m_connection, directory.c_str());
	while (mpd_entity *e = mpd_recv_entity(m_connection)) {
		if (mpd_entity_get_type(e) == MPD_ENTITY_TYPE_SONG)
			f(Song(mpd_song_dup(mpd_entity_get_song(e))));
		mpd_entity_free(e);
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetDirectories(const std::string &directory, StringConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(m_connection, directory.c_str());
	while (mpd_directory *dir = mpd_recv_directory(m_connection))
	{
		f(std::string(mpd_directory_get_path(dir)));
		mpd_directory_free(dir);
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetSongs(const std::string &directory, SongConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(m_connection, directory.c_str());
	while (mpd_song *s = mpd_recv_song(m_connection))
		f(Song(s));
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetOutputs(OutputConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_outputs(m_connection);
	while (mpd_output *output = mpd_recv_output(m_connection))
	{
		f(Output(mpd_output_get_name(output), mpd_output_get_enabled(output)));
		mpd_output_free(output);
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::EnableOutput(int id)
{
	prechecksNoCommandsList();
	mpd_run_enable_output(m_connection, id);
	checkErrors();
}

void Connection::DisableOutput(int id)
{
	prechecksNoCommandsList();
	mpd_run_disable_output(m_connection, id);
	checkErrors();
}

void Connection::GetURLHandlers(StringConsumer f)
{
	prechecksNoCommandsList();
	mpd_send_list_url_schemes(m_connection);
	while (mpd_pair *handler = mpd_recv_pair_named(m_connection, "handler"))
	{
		f(std::string(handler->value));
		mpd_return_pair(m_connection, handler);
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::GetTagTypes(StringConsumer f)
{
	
	prechecksNoCommandsList();
	mpd_send_list_tag_types(m_connection);
	while (mpd_pair *tag_type = mpd_recv_pair_named(m_connection, "tagtype"))
	{
		f(std::string(tag_type->value));
		mpd_return_pair(m_connection, tag_type);
	}
	mpd_response_finish(m_connection);
	checkErrors();
}

void Connection::checkConnection() const
{
	if (!m_connection)
		throw ClientError(MPD_ERROR_STATE, "No active MPD connection", false);
}

void Connection::prechecks()
{
	checkConnection();
	noidle();
}

void Connection::prechecksNoCommandsList()
{
	assert(!m_command_list_active);
	prechecks();
}

void Connection::checkErrors() const
{
	mpd_error code = mpd_connection_get_error(m_connection);
	if (code != MPD_ERROR_SUCCESS)
	{
		std::string msg = mpd_connection_get_error_message(m_connection);
		if (code == MPD_ERROR_SERVER)
		{
			mpd_server_error server_code = mpd_connection_get_server_error(m_connection);
			bool clearable = mpd_connection_clear_error(m_connection);
			throw ServerError(server_code, msg, clearable);
		}
		else
		{
			bool clearable = mpd_connection_clear_error(m_connection);
			throw ClientError(code, msg, clearable);
		}
	}
}

}
