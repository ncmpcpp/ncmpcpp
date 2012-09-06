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

#include <cassert>
#include <cstdlib>
#include <algorithm>

#include "charset.h"
#include "error.h"
#include "mpdpp.h"

MPD::Connection Mpd;

namespace MPD {//

bool Statistics::empty() const
{
	return m_stats.get() == 0;
}

unsigned Statistics::artists() const
{
	assert(!empty());
	return mpd_stats_get_number_of_artists(m_stats.get());
}

unsigned Statistics::albums() const
{
	assert(!empty());
	return mpd_stats_get_number_of_albums(m_stats.get());
}

unsigned Statistics::songs() const
{
	assert(!empty());
	return mpd_stats_get_number_of_songs(m_stats.get());
}

unsigned long Statistics::playTime() const
{
	assert(!empty());
	return mpd_stats_get_play_time(m_stats.get());
}

unsigned long Statistics::uptime() const
{
	assert(!empty());
	return mpd_stats_get_uptime(m_stats.get());
}

unsigned long Statistics::dbUpdateTime() const
{
	assert(!empty());
	return mpd_stats_get_db_update_time(m_stats.get());
}

unsigned long Statistics::dbPlayTime() const
{
	assert(!empty());
	return mpd_stats_get_db_play_time(m_stats.get());
}

Connection::Connection() : itsConnection(0),
				isCommandsListEnabled(0),
				isIdle(0),
				supportsIdle(0),
				itsHost("localhost"),
				itsPort(6600),
				itsTimeout(15),
				itsCurrentStatus(0),
				itsOldStatus(0),
				itsUpdater(0),
				itsErrorHandler(0)
{
}

Connection::~Connection()
{
	if (itsConnection)
		mpd_connection_free(itsConnection);
	if (itsOldStatus)
		mpd_status_free(itsOldStatus);
	if (itsCurrentStatus)
		mpd_status_free(itsCurrentStatus);
}

bool Connection::Connect()
{
	if (itsConnection)
		return true;
	itsConnection = mpd_connection_new(itsHost.c_str(), itsPort, itsTimeout*1000 /* timeout is in ms now */);
	if (CheckForErrors())
		return false;
	if (!itsPassword.empty())
		SendPassword();
	itsFD = mpd_connection_get_fd(itsConnection);
	supportsIdle = isIdleEnabled && Version() > 13;
	// in UpdateStatus() we compare it to itsElapsedTimer[0],
	// and for the first time it has always evaluate to true
	// so we need it to be zero at this point
	itsElapsedTimer[1] = 0;
	return !CheckForErrors();
}

bool Connection::Connected() const
{
	return itsConnection;
}

void Connection::Disconnect()
{
	if (itsConnection)
		mpd_connection_free(itsConnection);
	if (itsOldStatus)
		mpd_status_free(itsOldStatus);
	if (itsCurrentStatus)
		mpd_status_free(itsCurrentStatus);
	itsConnection = 0;
	isIdle = 0;
	itsCurrentStatus = 0;
	itsOldStatus = 0;
	isCommandsListEnabled = 0;
}

unsigned Connection::Version() const
{
	return itsConnection ? mpd_connection_get_server_version(itsConnection)[1] : 0;
}

void Connection::SetHostname(const std::string &host)
{
	size_t at = host.find("@");
	if (at != std::string::npos)
	{
		itsPassword = host.substr(0, at);
		itsHost = host.substr(at+1);
	}
	else
		itsHost = host;
}

bool Connection::SendPassword()
{
	assert(itsConnection);
	GoBusy();
	assert(!isCommandsListEnabled);
	mpd_run_password(itsConnection, itsPassword.c_str());
	return !CheckForErrors();
}

void Connection::SetStatusUpdater(StatusUpdater updater, void *data)
{
	itsUpdater = updater;
	itsStatusUpdaterUserdata = data;
}

void Connection::SetErrorHandler(ErrorHandler handler, void *data)
{
	itsErrorHandler = handler;
	itsErrorHandlerUserdata = data;
}

void Connection::GoIdle()
{
	if (supportsIdle && !itsIdleBlocked && !isIdle && mpd_send_idle(itsConnection))
		isIdle = 1;
}

int Connection::GoBusy()
{
	int flags = 0;
	if (isIdle && mpd_send_noidle(itsConnection))
	{
		isIdle = 0;
		if (hasData)
			flags = mpd_recv_idle(itsConnection, 1);
		mpd_response_finish(itsConnection);
	}
	return flags;
}

Statistics Connection::getStatistics()
{
	assert(itsConnection);
	GoBusy();
	mpd_stats *stats = mpd_run_stats(itsConnection);
	return Statistics(stats);
}

void Connection::UpdateStatus()
{
	if (!itsConnection)
		return;
	
	int idle_mask = 0;
	if (isIdle)
	{
		if (hasData)
		{
			idle_mask = GoBusy();
			hasData = 0;
		}
		else
		{
			// count local elapsed time as we don't receive
			// this from mpd while being in idle mode
			time(&itsElapsedTimer[1]);
			double diff = difftime(itsElapsedTimer[1], itsElapsedTimer[0]);
			if (diff >= 1.0 && Mpd.GetState() == psPlay)
			{
				time(&itsElapsedTimer[0]);
				itsElapsed += diff;
				StatusChanges changes;
				changes.ElapsedTime = 1;
				if (itsUpdater)
					itsUpdater(this, changes, itsErrorHandlerUserdata);
			}
			return;
		}
	}
	
	// if CheckForErrors() invokes callback, it can do some communication with mpd.
	// the problem is, we *have* to be out from idle mode here and issuing commands
	// will enter it again, which certainly is not desired, so let's block it for
	// a while.
	BlockIdle(true);
	CheckForErrors();
	BlockIdle(false);
	
	if (!itsConnection)
		return;
	
	if (itsOldStatus)
		mpd_status_free(itsOldStatus);
	
	itsOldStatus = itsCurrentStatus;
	itsCurrentStatus = 0;
	
	itsCurrentStatus = mpd_run_status(itsConnection);
	
	if (CheckForErrors())
		return;
	
	if (itsCurrentStatus && itsUpdater)
	{
		if (supportsIdle)
		{
			// sync local elapsed time counter with mpd
			unsigned old_elapsed = itsElapsed;
			itsElapsed = mpd_status_get_elapsed_time(itsCurrentStatus);
			itsChanges.ElapsedTime = itsElapsed != old_elapsed;
			time(&itsElapsedTimer[0]);
		}
		else
			itsElapsed = mpd_status_get_elapsed_time(itsCurrentStatus);
		
		if (!itsOldStatus)
		{
			itsChanges.Playlist = 1;
			itsChanges.SongID = 1;
			itsChanges.Database = 1;
			itsChanges.DBUpdating = 1;
			itsChanges.Volume = 1;
			itsChanges.ElapsedTime = 1;
			itsChanges.Crossfade = 1;
			itsChanges.Random = 1;
			itsChanges.Repeat = 1;
			itsChanges.Single = 1;
			itsChanges.Consume = 1;
			itsChanges.PlayerState = 1;
			itsChanges.StatusFlags = 1;
			itsChanges.Outputs = 1;
		}
		else
		{
			if (idle_mask != 0)
			{
				itsChanges.Playlist = idle_mask & MPD_IDLE_QUEUE;
				itsChanges.StoredPlaylists = idle_mask & MPD_IDLE_STORED_PLAYLIST;
				itsChanges.Database = idle_mask & MPD_IDLE_DATABASE;
				itsChanges.DBUpdating = idle_mask & MPD_IDLE_UPDATE;
				itsChanges.Volume = idle_mask & MPD_IDLE_MIXER;
				itsChanges.StatusFlags = idle_mask & (MPD_IDLE_OPTIONS | MPD_IDLE_UPDATE);
				itsChanges.Outputs = idle_mask & MPD_IDLE_OUTPUT;
			}
			else
			{
				itsChanges.Playlist = mpd_status_get_queue_version(itsOldStatus)
					!= mpd_status_get_queue_version(itsCurrentStatus);
				
				itsChanges.ElapsedTime = mpd_status_get_elapsed_time(itsOldStatus)
						      != mpd_status_get_elapsed_time(itsCurrentStatus);
				
				itsChanges.Database = mpd_status_get_update_id(itsOldStatus)
						 &&  !mpd_status_get_update_id(itsCurrentStatus);
				
				itsChanges.DBUpdating = mpd_status_get_update_id(itsOldStatus)
						     != mpd_status_get_update_id(itsCurrentStatus);
				
				itsChanges.Volume = mpd_status_get_volume(itsOldStatus)
						 != mpd_status_get_volume(itsCurrentStatus);
				
				itsChanges.StatusFlags = itsChanges.Repeat
						||	 itsChanges.Random
						||	 itsChanges.Single
						||	 itsChanges.Consume
						||	 itsChanges.Crossfade
						||	 itsChanges.DBUpdating;
				
				// there is no way to determine if the output has changed or not
				// from mpd status, it's possible only with idle notifications
				itsChanges.Outputs = 0;
			}
			
			itsChanges.SongID = mpd_status_get_song_id(itsOldStatus)
					 != mpd_status_get_song_id(itsCurrentStatus);
			
			itsChanges.Crossfade = mpd_status_get_crossfade(itsOldStatus)
					    != mpd_status_get_crossfade(itsCurrentStatus);
			
			itsChanges.Random = mpd_status_get_random(itsOldStatus)
					 != mpd_status_get_random(itsCurrentStatus);
			
			itsChanges.Repeat = mpd_status_get_repeat(itsOldStatus)
					 != mpd_status_get_repeat(itsCurrentStatus);
			
			itsChanges.Single = mpd_status_get_single(itsOldStatus)
					 != mpd_status_get_single(itsCurrentStatus);
			
			itsChanges.Consume = mpd_status_get_consume(itsOldStatus)
					  != mpd_status_get_consume(itsCurrentStatus);
			
			itsChanges.PlayerState = mpd_status_get_state(itsOldStatus)
					      != mpd_status_get_state(itsCurrentStatus);
		}
		itsUpdater(this, itsChanges, itsErrorHandlerUserdata);
		// status updater could invoke mpd commands that
		// could fail se we need to check for errors
		CheckForErrors();
		GoIdle();
	}
}

bool Connection::UpdateDirectory(const std::string &path)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		bool success = mpd_run_update(itsConnection, path.c_str());
		if (!supportsIdle && success)
			UpdateStatus();
		return success;
	}
	else
	{
		assert(!isIdle);
		return mpd_send_update(itsConnection, path.c_str());
	}
	
}

void Connection::Play()
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_play(itsConnection);
	}
	else
	{
		assert(!isIdle);
		mpd_send_play(itsConnection);
	}
}

void Connection::Play(int pos)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_play_pos(itsConnection, pos);
	}
	else
	{
		assert(!isIdle);
		mpd_send_play_pos(itsConnection, pos);
	}
}

void Connection::PlayID(int id)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_play_id(itsConnection, id);
	}
	else
	{
		assert(!isIdle);
		mpd_send_play_id(itsConnection, id);
	}
}

void Connection::Pause(bool state)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_pause(itsConnection, state);
	}
	else
	{
		assert(!isIdle);
		mpd_send_pause(itsConnection, state);
	}
}

void Connection::Toggle()
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		if (isPlaying())
			mpd_run_toggle_pause(itsConnection);
		else
			mpd_run_play(itsConnection);
	}
	else
	{
		assert(!isIdle);
		if (isPlaying())
			mpd_send_toggle_pause(itsConnection);
		else
			mpd_send_play(itsConnection);
	}
}

void Connection::Stop()
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_stop(itsConnection);
	}
	else
	{
		assert(!isIdle);
		mpd_send_stop(itsConnection);
	}
}

void Connection::Next()
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_next(itsConnection);
	}
	else
	{
		assert(!isIdle);
		mpd_send_next(itsConnection);
	}
}

void Connection::Prev()
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_previous(itsConnection);
	}
	else
	{
		assert(!isIdle);
		mpd_send_previous(itsConnection);
	}
}

bool Connection::Move(unsigned from, unsigned to)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_move(itsConnection, from, to);
	}
	else
	{
		assert(!isIdle);
		return mpd_send_move(itsConnection, from, to);
	}
}

void Connection::Swap(unsigned from, unsigned to)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_swap(itsConnection, from, to);
	}
	else
	{
		assert(!isIdle);
		mpd_send_swap(itsConnection, from, to);
	}
}

void Connection::Seek(unsigned where)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_seek_pos(itsConnection, Mpd.GetCurrentlyPlayingSongPos(), where);
	}
	else
	{
		assert(!isIdle);
		mpd_send_seek_pos(itsConnection, Mpd.GetCurrentlyPlayingSongPos(), where);
	}
}

void Connection::Shuffle()
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_shuffle(itsConnection);
	}
	else
	{
		assert(!isIdle);
		mpd_send_shuffle(itsConnection);
	}
}

bool Connection::ClearMainPlaylist()
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_clear(itsConnection);
	}
	else
	{
		assert(!isIdle);
		return mpd_send_clear(itsConnection);
	}
}

bool Connection::ClearPlaylist(const std::string &playlist)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_playlist_clear(itsConnection, playlist.c_str());
	}
	else
	{
		return mpd_send_playlist_clear(itsConnection, playlist.c_str());
		assert(!isIdle);
	}
}

void Connection::AddToPlaylist(const std::string &path, const Song &s)
{
	AddToPlaylist(path, s.getURI());
}

void Connection::AddToPlaylist(const std::string &path, const std::string &file)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_playlist_add(itsConnection, path.c_str(), file.c_str());
	}
	else
	{
		assert(!isIdle);
		mpd_send_playlist_add(itsConnection, path.c_str(), file.c_str());
	}
}

bool Connection::PlaylistMove(const std::string &path, int from, int to)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_send_playlist_move(itsConnection, path.c_str(), from, to)
		    && mpd_response_finish(itsConnection);
	}
	else
	{
		assert(!isIdle);
		return mpd_send_playlist_move(itsConnection, path.c_str(), from, to);
	}
}

bool Connection::Rename(const std::string &from, const std::string &to)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_rename(itsConnection, from.c_str(), to.c_str());
	}
	else
	{
		assert(!isIdle);
		return mpd_send_rename(itsConnection, from.c_str(), to.c_str());
	}
}

SongList Connection::GetPlaylistChanges(unsigned version)
{
	SongList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_queue_changes_meta(itsConnection, version);
	while (mpd_song *s = mpd_recv_song(itsConnection))
		result.push_back(Song(s));
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

Song Connection::GetSong(const std::string &path)
{
	if (!itsConnection)
		return Song();
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_list_all_meta(itsConnection, path.c_str());
	mpd_song *s = mpd_recv_song(itsConnection);
	mpd_response_finish(itsConnection);
	GoIdle();
	return Song(s);
}

int Connection::GetCurrentSongPos() const
{
	return itsCurrentStatus ? mpd_status_get_song_pos(itsCurrentStatus) : -1;
}

int Connection::GetCurrentlyPlayingSongPos() const
{
	return isPlaying() ? GetCurrentSongPos() : -1;
}

Song Connection::GetCurrentlyPlayingSong()
{
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_song *s = itsConnection && isPlaying() ? mpd_run_current_song(itsConnection) : 0;
	Song result = s ? Song(s) : Song();
	GoIdle();
	return result;
}

SongList Connection::GetPlaylistContent(const std::string &path)
{
	SongList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_list_playlist_meta(itsConnection, path.c_str());
	while (mpd_song *s = mpd_recv_song(itsConnection))
		result.push_back(Song(s));
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

void Connection::GetSupportedExtensions(std::set<std::string> &acc)
{
	if (!itsConnection)
		return;
	assert(!isCommandsListEnabled);
	GoBusy();
	
	mpd_send_command(itsConnection, "decoders", NULL);
	while (mpd_pair *pair = mpd_recv_pair_named(itsConnection, "suffix"))
	{
		acc.insert(pair->value);
		mpd_return_pair(itsConnection, pair);
	}
	mpd_response_finish(itsConnection);
	
	GoIdle();
}

void Connection::SetRepeat(bool mode)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_repeat(itsConnection, mode);
	}
	else
	{
		assert(!isIdle);
		mpd_send_repeat(itsConnection, mode);
	}
}

void Connection::SetRandom(bool mode)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_random(itsConnection, mode);
	}
	else
	{
		assert(!isIdle);
		mpd_send_random(itsConnection, mode);
	}
}

void Connection::SetSingle(bool mode)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_single(itsConnection, mode);
	}
	else
	{
		assert(!isIdle);
		mpd_send_single(itsConnection, mode);
	}
}

void Connection::SetConsume(bool mode)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_consume(itsConnection, mode);
	}
	else
	{
		assert(!isIdle);
		mpd_send_consume(itsConnection, mode);
	}
}

void Connection::SetVolume(unsigned vol)
{
	if (!itsConnection || vol > 100)
		return;
	assert(!isCommandsListEnabled);
	GoBusy();
	if (mpd_run_set_volume(itsConnection, vol) && !supportsIdle)
		UpdateStatus();
}

std::string Connection::GetReplayGainMode()
{
	if (!itsConnection)
		return "Unknown";
	assert(!isCommandsListEnabled);
	GoBusy();
	if (!mpd_send_command(itsConnection, "replay_gain_status", NULL))
		return "Unknown";
	std::string result;
	if (mpd_pair *pair = mpd_recv_pair_named(itsConnection, "replay_gain_mode"))
	{
		result = pair->value;
		if (!result.empty())
			result[0] = toupper(result[0]);
		mpd_return_pair(itsConnection, pair);
	}
	mpd_response_finish(itsConnection);
	return result;
}

void Connection::SetReplayGainMode(ReplayGainMode mode)
{
	if (!itsConnection)
		return;
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
			FatalError("undefined value of ReplayGainMode!");
	}
	if (!isCommandsListEnabled)
		GoBusy();
	else
		assert(!isIdle);
	if (!mpd_send_command(itsConnection, "replay_gain_mode", rg_mode, NULL))
		return;
	if (!isCommandsListEnabled)
		mpd_response_finish(itsConnection);
}

void Connection::SetCrossfade(unsigned crossfade)
{
	if (!itsConnection)
		return;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		mpd_run_crossfade(itsConnection, crossfade);
	}
	else
	{
		assert(!isIdle);
		mpd_send_crossfade(itsConnection, crossfade);
	}
}

bool Connection::SetPriority(const Song &s, int prio)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_prio_id(itsConnection, prio, s.getID());
	}
	else
	{
		assert(!isIdle);
		return mpd_send_prio_id(itsConnection, prio, s.getID());
	}
}

int Connection::AddSong(const std::string &path, int pos)
{
	if (!itsConnection)
		return -1;
	int id;
	if (!isCommandsListEnabled)
		GoBusy();
	else
		assert(!isIdle);
	if (pos < 0)
		mpd_send_add_id(itsConnection, path.c_str());
	else
		mpd_send_add_id_to(itsConnection, path.c_str(), pos);
	if (!isCommandsListEnabled)
	{
		id = mpd_recv_song_id(itsConnection);
		mpd_response_finish(itsConnection);
	}
	else
		id = 0;
	return id;
}

int Connection::AddSong(const Song &s, int pos)
{
	return AddSong((!s.isFromDatabase() ? "file://" : "") + s.getURI(), pos);
}

bool Connection::Add(const std::string &path)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_add(itsConnection, path.c_str());
	}
	else
	{
		assert(!isIdle);
		return mpd_send_add(itsConnection, path.c_str());
	}
}

bool Connection::AddRandomTag(mpd_tag_type tag, size_t number)
{
	if (!itsConnection && !number)
		return false;
	assert(!isCommandsListEnabled);
	
	auto tags = GetList(tag);
	if (number > tags.size())
	{
		if (itsErrorHandler)
			itsErrorHandler(this, 0, "Requested number is out of range!", itsErrorHandlerUserdata);
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
			auto songs = CommitSearchSongs();
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
	if (!itsConnection && !number)
		return false;
	assert(!isCommandsListEnabled);
	
	StringList files;
	
	GoBusy();
	mpd_send_list_all(itsConnection, "/");
	while (mpd_pair *item = mpd_recv_pair_named(itsConnection, "file"))
	{
		files.push_back(item->value);
		mpd_return_pair(itsConnection, item);
	}
	mpd_response_finish(itsConnection);
	
	if (number > files.size())
	{
		if (itsErrorHandler)
			itsErrorHandler(this, 0, "Requested number of random songs is bigger than size of your library!", itsErrorHandlerUserdata);
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

bool Connection::Delete(unsigned pos)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
		GoBusy();
	else
		assert(!isIdle);
	bool result = mpd_send_delete(itsConnection, pos);
	if (!isCommandsListEnabled)
		result = mpd_response_finish(itsConnection);
	return result;
}

bool Connection::DeleteID(unsigned id)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
		GoBusy();
	else
		assert(!isIdle);
	bool result = mpd_send_delete_id(itsConnection, id);
	if (!isCommandsListEnabled)
		result = mpd_response_finish(itsConnection);
	return result;
}

bool Connection::PlaylistDelete(const std::string &playlist, unsigned pos)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_playlist_delete(itsConnection, playlist.c_str(), pos);
	}
	else
	{
		assert(!isIdle);
		return mpd_send_playlist_delete(itsConnection, playlist.c_str(), pos);
	}
}

void Connection::StartCommandsList()
{
	if (!itsConnection)
		return;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_command_list_begin(itsConnection, 1);
	isCommandsListEnabled = 1;
}

bool Connection::CommitCommandsList()
{
	if (!itsConnection)
		return false;
	assert(isCommandsListEnabled);
	assert(!isIdle);
	mpd_command_list_end(itsConnection);
	isCommandsListEnabled = 0;
	return mpd_response_finish(itsConnection);
}

bool Connection::DeletePlaylist(const std::string &name)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_rm(itsConnection, name.c_str());
	}
	else
	{
		assert(!isIdle);
		return mpd_send_rm(itsConnection, name.c_str());
	}
}

bool Connection::LoadPlaylist(const std::string &name)
{
	if (!itsConnection)
		return false;
	assert(!isCommandsListEnabled);
	GoBusy();
	return mpd_run_load(itsConnection, name.c_str());
}

int Connection::SavePlaylist(const std::string &name)
{
	if (!itsConnection)
		return false;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_save(itsConnection, name.c_str());
	mpd_response_finish(itsConnection);
	
	if (mpd_connection_get_error(itsConnection) == MPD_ERROR_SERVER
	&&  mpd_connection_get_server_error(itsConnection) == MPD_SERVER_ERROR_EXIST)
		return MPD_SERVER_ERROR_EXIST;
	else
		return CheckForErrors();
}

StringList Connection::GetPlaylists()
{
	StringList result;
	if (!itsConnection)
		return result;
	auto items = GetDirectory("/");
	for (auto it = items.begin(); it != items.end(); ++it)
		if (it->type == itPlaylist)
			result.push_back(it->name);
	return result;
}

StringList Connection::GetList(mpd_tag_type type)
{
	StringList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_search_db_tags(itsConnection, type);
	mpd_search_commit(itsConnection);
	while (mpd_pair *item = mpd_recv_pair_tag(itsConnection, type))
	{
		result.push_back(item->value);
		mpd_return_pair(itsConnection, item);
	}
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

void Connection::StartSearch(bool exact_match)
{
	if (itsConnection)
		mpd_search_db_songs(itsConnection, exact_match);
}

void Connection::StartFieldSearch(mpd_tag_type item)
{
	if (itsConnection)
	{
		itsSearchedField = item;
		mpd_search_db_tags(itsConnection, item);
	}
}

void Connection::AddSearch(mpd_tag_type item, const std::string &str) const
{
	// mpd version < 0.14.* doesn't support empty search constraints
	if (Version() < 14 && str.empty())
		return;
	if (itsConnection)
		mpd_search_add_tag_constraint(itsConnection, MPD_OPERATOR_DEFAULT, item, str.c_str());
}

void Connection::AddSearchAny(const std::string &str) const
{
	assert(!str.empty());
	if (itsConnection)
		mpd_search_add_any_tag_constraint(itsConnection, MPD_OPERATOR_DEFAULT, str.c_str());
}

void Connection::AddSearchURI(const std::string &str) const
{
	assert(!str.empty());
	if (itsConnection)
		mpd_search_add_uri_constraint(itsConnection, MPD_OPERATOR_DEFAULT, str.c_str());
}

SongList Connection::CommitSearchSongs()
{
	SongList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_search_commit(itsConnection);
	while (mpd_song *s = mpd_recv_song(itsConnection))
		result.push_back(Song(s));
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

StringList Connection::CommitSearchTags()
{
	StringList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_search_commit(itsConnection);
	while (mpd_pair *tag = mpd_recv_pair_tag(itsConnection, itsSearchedField))
	{
		result.push_back(tag->value);
		mpd_return_pair(itsConnection, tag);
	}
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

ItemList Connection::GetDirectory(const std::string &path)
{
	ItemList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_list_meta(itsConnection, path.c_str());
	while (mpd_entity *item = mpd_recv_entity(itsConnection))
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
		result.push_back(std::move(it));
	}
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

SongList Connection::GetDirectoryRecursive(const std::string &path)
{
	SongList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_list_all_meta(itsConnection, path.c_str());
	while (mpd_song *s = mpd_recv_song(itsConnection))
		result.push_back(Song(s));
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

StringList Connection::GetDirectories(const std::string &path)
{
	StringList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_list_meta(itsConnection, path.c_str());
	while (mpd_directory *dir = mpd_recv_directory(itsConnection))
	{
		result.push_back(mpd_directory_get_path(dir));
		mpd_directory_free(dir);
	}
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

SongList Connection::GetSongs(const std::string &path)
{
	SongList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_list_meta(itsConnection, path.c_str());
	while (mpd_song *s = mpd_recv_song(itsConnection))
		result.push_back(Song(s));
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

OutputList Connection::GetOutputs()
{
	OutputList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_outputs(itsConnection);
	while (mpd_output *output = mpd_recv_output(itsConnection))
	{
		result.push_back(Output(mpd_output_get_name(output), mpd_output_get_enabled(output)));
		mpd_output_free(output);
	}
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

bool Connection::EnableOutput(int id)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_enable_output(itsConnection, id);
	}
	else
	{
		assert(!isIdle);
		return mpd_send_enable_output(itsConnection, id);
	}
}

bool Connection::DisableOutput(int id)
{
	if (!itsConnection)
		return false;
	if (!isCommandsListEnabled)
	{
		GoBusy();
		return mpd_run_disable_output(itsConnection, id);
	}
	else
	{
		assert(!isIdle);
		return mpd_send_disable_output(itsConnection, id);
	}
}

StringList Connection::GetURLHandlers()
{
	StringList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_list_url_schemes(itsConnection);
	while (mpd_pair *handler = mpd_recv_pair_named(itsConnection, "handler"))
	{
		result.push_back(handler->value);
		mpd_return_pair(itsConnection, handler);
	}
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

StringList Connection::GetTagTypes()
{
	StringList result;
	if (!itsConnection)
		return result;
	assert(!isCommandsListEnabled);
	GoBusy();
	mpd_send_list_tag_types(itsConnection);
	while (mpd_pair *tag_type = mpd_recv_pair_named(itsConnection, "tagtype"))
	{
		result.push_back(tag_type->value);
		mpd_return_pair(itsConnection, tag_type);
	}
	mpd_response_finish(itsConnection);
	GoIdle();
	return result;
}

int Connection::CheckForErrors()
{
	int error_code = MPD_ERROR_SUCCESS;
	if ((error_code = mpd_connection_get_error(itsConnection)) != MPD_ERROR_SUCCESS)
	{
		itsErrorMessage = mpd_connection_get_error_message(itsConnection);
		if (error_code == MPD_ERROR_SERVER)
			error_code |= (mpd_connection_get_server_error(itsConnection) << 8);
		if (!mpd_connection_clear_error(itsConnection))
		{
			Disconnect();
			// notify about mpd state changed to unknown.
			StatusChanges changes;
			changes.PlayerState = 1;
			if (itsUpdater)
				itsUpdater(this, changes, itsErrorHandlerUserdata);
		}
		if (itsErrorHandler)
			itsErrorHandler(this, error_code, itsErrorMessage.c_str(), itsErrorHandlerUserdata);
	}
	return error_code;
}

}
