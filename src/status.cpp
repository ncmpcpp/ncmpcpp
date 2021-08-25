/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "curses/menu_impl.h"
#include "screens/browser.h"
#include "charset.h"
#include "format_impl.h"
#include "global.h"
#include "helpers.h"
#include "macro_utilities.h"
#include "screens/lyrics.h"
#include "screens/media_library.h"
#include "screens/outputs.h"
#include "screens/playlist.h"
#include "screens/playlist_editor.h"
#include "screens/search_engine.h"
#include "screens/sel_items_adder.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "screens/tag_editor.h"
#include "screens/visualizer.h"
#include "title.h"
#include "utility/string.h"

using Global::myScreen;

using Global::wFooter;
using Global::wHeader;

using Global::Timer;
using Global::VolumeState;

namespace {

boost::posix_time::ptime past = boost::posix_time::from_time_t(0);

size_t playing_song_scroll_begin = 0;
size_t first_line_scroll_begin = 0;
size_t second_line_scroll_begin = 0;

bool m_status_initialized;

char m_consume;
char m_crossfade;
char m_db_updating;
char m_repeat;
char m_random;
char m_single;

int m_current_song_id;
int m_current_song_pos;
unsigned m_elapsed_time;
unsigned m_kbps;
MPD::PlayerState m_player_state;
unsigned m_playlist_version;
unsigned m_playlist_length;
unsigned m_total_time;
int m_volume;

void drawTitle(const MPD::Song &np)
{
	assert(!np.empty());
	windowTitle(Format::stringify<char>(Config.song_window_title_format, &np));
}

std::string playerStateToString(MPD::PlayerState ps)
{
	std::string result;
	switch (ps)
	{
		case MPD::psUnknown:
			switch (Config.design)
			{
				case Design::Alternative:
					result = "[unknown]";
					break;
				case Design::Classic:
					break;
			}
			break;
		case MPD::psPlay:
			switch (Config.design)
			{
				case Design::Alternative:
					result = "[playing]";
					break;
				case Design::Classic:
					result = "Playing:";
					break;
			}
			break;
		case MPD::psPause:
			switch (Config.design)
			{
				case Design::Alternative:
					result = "[paused]";
					break;
				case Design::Classic:
					result = "Paused:";
					break;
			}
			break;
		case MPD::psStop:
			switch (Config.design)
			{
				case Design::Alternative:
					result = "[stopped]";
					break;
				case Design::Classic:
					break;
			}
			break;
	}
	return result;
}

void initialize_status()
{
	// get full info about new connection
	Status::update(-1);

	if (Config.jump_to_now_playing_song_at_start)
	{
		int curr_pos = Status::State::currentSongPosition();
		if  (curr_pos >= 0)
		{
			myPlaylist->main().highlight(curr_pos);
			if (isVisible(myPlaylist))
				myPlaylist->refresh();
		}
	}

	// Set TCP_NODELAY on the tcp socket as we are using write-write-read pattern
	// a lot (noidle - write, command - write, then read the result of command),
	// which kills the performance.
	int flag = 1;
	setsockopt(Mpd.GetFD(), IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

	myBrowser->fetchSupportedExtensions();
#	ifdef ENABLE_OUTPUTS
	myOutputs->fetchList();
#	endif // ENABLE_OUTPUTS
#	ifdef ENABLE_VISUALIZER
	myVisualizer->CloseDataSource();
	myVisualizer->OpenDataSource();
	myVisualizer->FindOutputID();
#	endif // ENABLE_VISUALIZER

	m_status_initialized = true;
	wFooter->addFDCallback(Mpd.GetFD(), Statusbar::Helpers::mpd);
	if (Config.connected_message_on_startup)
	{
		Statusbar::printf("Connected to %1%", Mpd.GetHostname());
	}
}

}

/*************************************************************************/

void Status::handleClientError(MPD::ClientError &e)
{
	if (!e.clearable())
		Mpd.Disconnect();
	Statusbar::printf("ncmpcpp: %1%", e.what());
}

void Status::handleServerError(MPD::ServerError &e)
{
	Statusbar::printf("MPD: %1%", e.what());
	if (e.code() == MPD_SERVER_ERROR_PERMISSION)
	{
		try
		{
			NC::Window::ScopedPromptHook helper(*wFooter, nullptr);
			Statusbar::put() << "Password: ";
			Mpd.SetPassword(wFooter->prompt("", -1, true));
			Mpd.SendPassword();
			Statusbar::print("Password accepted");
		}
		// SendPassword might throw if connection is closed
		catch (MPD::ClientError &e_prim)
		{
			handleClientError(e_prim);
		}
		// Wrong password, we'll ask again later
		catch (MPD::ServerError &e_prim)
		{
			Statusbar::printf("MPD: %1%", e_prim.what());
		}
		// If prompt asking for a password is aborted, exit the application to
		// prevent getting stuck in the prompt indefinitely.
		catch (NC::PromptAborted &)
		{
			Actions::ExitMainLoop = true;
		}
	}
}

/*************************************************************************/

void Status::trace(bool update_timer, bool update_window_timeout)
{
	if (update_timer)
		Timer = boost::posix_time::microsec_clock::local_time();
	if (Mpd.Connected())
	{
		if (!m_status_initialized)
			initialize_status();

		if (m_player_state == MPD::psPlay
		&&  Global::Timer - past > boost::posix_time::seconds(1))
		{
			// update elapsed time/bitrate of the current song
			Status::Changes::elapsedTime(true);
			wFooter->refresh();
			past = Timer;
		}

		applyToVisibleWindows(&BaseScreen::update);
		Statusbar::tryRedraw();

		Mpd.idle();
	}
	// Update timeout after MPD as it may depend on its status.
	if (update_window_timeout)
	{
		// set appropriate window timeout
		int nc_wtimeout = std::numeric_limits<int>::max();
		applyToVisibleWindows([&nc_wtimeout](BaseScreen *s) {
			nc_wtimeout = std::min(nc_wtimeout, s->windowTimeout());
		});
		wFooter->setTimeout(nc_wtimeout);
	}
}

void Status::update(int event)
{
	auto st = Mpd.getStatus();
	m_current_song_pos = st.currentSongPosition();
	m_elapsed_time = st.elapsedTime();
	m_kbps = st.kbps();
	m_player_state = st.playerState();
	m_playlist_length = st.playlistLength();
	m_total_time = st.totalTime();
	m_volume = st.volume();

	if (event & MPD_IDLE_DATABASE)
		Changes::database();
	if (event & MPD_IDLE_STORED_PLAYLIST)
		Changes::storedPlaylists();
	if (event & MPD_IDLE_PLAYLIST)
	{
		Changes::playlist(m_playlist_version);
		m_playlist_version = st.playlistVersion();
	}
	if (event & MPD_IDLE_PLAYER)
	{
		Changes::playerState();
		if (m_current_song_id != st.currentSongID())
		{
			Changes::songID(st.currentSongID());
			m_current_song_id = st.currentSongID();
		}
	}
	if (event & MPD_IDLE_MIXER)
		Changes::mixer();
	if (event & MPD_IDLE_OUTPUT)
		Changes::outputs();
	if (event & (MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS))
	{
		if (event & MPD_IDLE_UPDATE)
		{
			m_db_updating = st.updateID() ? 'U' : 0;
			if (m_status_initialized)
				Statusbar::printf("Database update %1%", m_db_updating ? "started" : "finished");
		}
		if (event & MPD_IDLE_OPTIONS)
		{
			if (('r' == m_repeat) != st.repeat())
			{
				m_repeat = st.repeat() ? 'r' : 0;
				if (m_status_initialized)
					Statusbar::printf("Repeat mode is %1%", !m_repeat ? "off" : "on");
			}
			if (('z' == m_random) != st.random())
			{
				m_random = st.random() ? 'z' : 0;
				if (m_status_initialized)
					Statusbar::printf("Random mode is %1%", !m_random ? "off" : "on");
			}
			if (('s' == m_single) != st.single())
			{
				m_single = st.single() ? 's' : 0;
				if (m_status_initialized)
					Statusbar::printf("Single mode is %1%", !m_single ? "off" : "on");
			}
			if (('c' == m_consume) != st.consume())
			{
				m_consume = st.consume() ? 'c' : 0;
				if (m_status_initialized)
					Statusbar::printf("Consume mode is %1%", !m_consume ? "off" : "on");
			}
			if (('x' == m_crossfade) != (st.crossfade() != 0))
			{
				int crossfade = st.crossfade();
				m_crossfade = crossfade ? 'x' : 0;
				if (m_status_initialized)
					Statusbar::printf("Crossfade set to %1% seconds", crossfade);
			}
		}
		Changes::flags();
	}
	m_status_initialized = true;

	if (event & MPD_IDLE_PLAYER)
		wFooter->refresh();

	if (event & (MPD_IDLE_PLAYLIST | MPD_IDLE_DATABASE | MPD_IDLE_PLAYER))
		applyToVisibleWindows(&BaseScreen::refreshWindow);
}

void Status::clear()
{
	// reset local variables
	m_status_initialized = false;
	m_repeat = 0;
	m_random = 0;
	m_single = 0;
	m_consume = 0;
	m_crossfade = 0;
	m_db_updating = 0;
	m_current_song_id = -1;
	m_current_song_pos = -1;
	m_kbps = 0;
	m_player_state = MPD::psUnknown;
	m_playlist_length = 0;
	m_playlist_version = 0;
	m_total_time = 0;
	m_volume = -1;
}

/*************************************************************************/

bool Status::State::consume()
{
	return m_consume != 0;
}

bool Status::State::crossfade()
{
	return m_crossfade != 0;
}

bool Status::State::repeat()
{
	return m_repeat != 0;
}

bool Status::State::random()
{
	return m_random != 0;
}

bool Status::State::single()
{
	return m_single != 0;
}

int Status::State::currentSongID()
{
	return m_current_song_id;
}

int Status::State::currentSongPosition()
{
	return m_current_song_pos;
}

unsigned Status::State::playlistLength()
{
	return m_playlist_length;
}

unsigned Status::State::elapsedTime()
{
	return m_elapsed_time;
}

MPD::PlayerState Status::State::player()
{
	return m_player_state;
}

unsigned Status::State::totalTime()
{
	return m_total_time;
}

int Status::State::volume()
{
	return m_volume;
}

/*************************************************************************/

void Status::Changes::playlist(unsigned previous_version)
{
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter(ReapplyFilter::Yes, myPlaylist->main());

		if (m_playlist_length < myPlaylist->main().size())
		{
			auto it = myPlaylist->main().begin()+m_playlist_length;
			auto end = myPlaylist->main().end();
			for (; it != end; ++it)
				myPlaylist->unregisterSong(it->value());
			myPlaylist->main().resizeList(m_playlist_length);
		}

		MPD::SongIterator s = Mpd.GetPlaylistChanges(previous_version), end;
		for (; s != end; ++s)
		{
			size_t pos = s->getPosition();
			myPlaylist->registerSong(*s);
			if (pos < myPlaylist->main().size())
			{
				// if song's already in playlist, replace it with a new one
				MPD::Song &old_s = myPlaylist->main()[pos].value();
				myPlaylist->unregisterSong(old_s);
				old_s = std::move(*s);
			}
			else // otherwise just add it to playlist
				myPlaylist->main().addItem(std::move(*s));
		}
	}

	myPlaylist->reloadTotalLength();
	myPlaylist->reloadRemaining();

	// When we're in multi-column screens, it might happen that songs visible on
	// the screen are added, but they will not be immediately marked as such
	// because the window that contains them is not the active one at the moment,
	// so we need to refresh them manually.
	if (isVisible(myLibrary)
	    && !myLibrary->isActiveWindow(myLibrary->Songs))
		myLibrary->Songs.refresh();
	if (isVisible(myPlaylistEditor)
	    && !myPlaylistEditor->isActiveWindow(myPlaylistEditor->Content))
		myPlaylistEditor->Content.refresh();
}

void Status::Changes::storedPlaylists()
{
	myPlaylistEditor->requestPlaylistsUpdate();
	myPlaylistEditor->requestContentUpdate();
	if (!myBrowser->isLocal() && myBrowser->inRootDirectory())
		myBrowser->requestUpdate();
}

void Status::Changes::database()
{
	myBrowser->requestUpdate();
#	ifdef HAVE_TAGLIB_H
	myTagEditor->Dirs->clear();
#	endif // HAVE_TAGLIB_H
	myLibrary->requestTagsUpdate();
	myLibrary->requestAlbumsUpdate();
	myLibrary->requestSongsUpdate();
}

void Status::Changes::playerState()
{
	if (!Config.execute_on_player_state_change.empty())
	{
		auto stateToEnv = [](MPD::PlayerState st) -> const char * {
			switch (st)
			{
			case MPD::psPlay:    return "play";
			case MPD::psStop:    return "stop";
			case MPD::psPause:   return "pause";
			case MPD::psUnknown: return "unknown";
			}
			throw std::logic_error("unreachable");
		};
		setenv("MPD_PLAYER_STATE", stateToEnv(m_player_state), 1);
		// Since we're setting a MPD_PLAYER_STATE, we need to block.
		runExternalCommand(Config.execute_on_player_state_change, true);
		unsetenv("MPD_PLAYER_STATE");
	}

	switch (m_player_state)
	{
		case MPD::psPlay:
		{
			auto np = myPlaylist->nowPlayingSong();
			if (!np.empty())
				drawTitle(np);
			myPlaylist->reloadRemaining();
			break;
		}
		case MPD::psStop:
			windowTitle("ncmpcpp " VERSION);
			if (Progressbar::isUnlocked())
				Progressbar::draw(0, 0);
			myPlaylist->reloadRemaining();
			if (Config.design == Design::Alternative)
			{
				*wHeader << NC::XY(0, 0) << NC::TermManip::ClearToEOL;
				*wHeader << NC::XY(0, 1) << NC::TermManip::ClearToEOL;
				mixer();
				flags();
			}
#			ifdef ENABLE_VISUALIZER
			if (isVisible(myVisualizer))
				myVisualizer->Clear();
#			endif // ENABLE_VISUALIZER
			break;
		default:
			break;
	}

	std::string state = playerStateToString(m_player_state);
	if (Config.design == Design::Alternative)
	{
		*wHeader << NC::XY(0, 1) << NC::Format::Bold << state << NC::Format::NoBold;
		wHeader->refresh();
	}
	else if (Statusbar::isUnlocked() && Config.statusbar_visibility)
	{
		*wFooter << NC::XY(0, 1);
		if (state.empty())
			*wFooter << NC::TermManip::ClearToEOL;
		else
			*wFooter << NC::Format::Bold << state << NC::Format::NoBold;
	}

	// needed for immediate display after starting
	// player from stopped state or seeking
	elapsedTime(false);
}

void Status::Changes::songID(int song_id)
{
	// update information about current song
	myPlaylist->reloadRemaining();
	playing_song_scroll_begin = 0;
	first_line_scroll_begin = 0;
	second_line_scroll_begin = 0;
#	ifdef ENABLE_VISUALIZER
	myVisualizer->ResetAutoScaleMultiplier();
#	endif // ENABLE_VISUALIZER
	if (m_player_state != MPD::psStop)
	{
		auto &pl = myPlaylist->main();

		// try to find the song with new id in the playlist
		auto it = std::find_if(pl.beginV(), pl.endV(), [song_id](const MPD::Song &s) {
			return s.getID() == unsigned(song_id);
		});
		// if it's not there (playlist may be outdated), fetch it
		const auto &s = it != pl.endV() ? *it : Mpd.GetCurrentSong();
		if (!s.empty())
		{
			if (!Config.execute_on_song_change.empty())
			{
				// We need to block to allow sending output to the terminal so a script
				// can e.g. set the album art.
				runExternalCommand(Config.execute_on_song_change, true);
			}

			if (Config.fetch_lyrics_in_background)
				myLyrics->fetchInBackground(s, false);

			drawTitle(s);

			if (Config.autocenter_mode)
				myPlaylist->locateSong(s);

			if (Config.now_playing_lyrics
			    && isVisible(myLyrics)
			    && myLyrics->previousScreen() == myPlaylist)
				myLyrics->fetch(s);
		}
	}
	elapsedTime(false);
}

void Status::Changes::elapsedTime(bool update_elapsed)
{
	auto np = myPlaylist->nowPlayingSong();
	if (m_player_state == MPD::psStop || np.empty())
	{
		// MPD is not playing, clear statusbar and exit.
		if (Statusbar::isUnlocked() && Config.statusbar_visibility)
			*wFooter << NC::XY(0, 1)
			         << NC::TermManip::ClearToEOL;
		if (Progressbar::isUnlocked())
			Progressbar::draw(0, 0);
		return;
	}

	if (update_elapsed)
	{
		auto st = Mpd.getStatus();
		m_elapsed_time = st.elapsedTime();
		m_kbps = st.kbps();
	}

	std::string ps = playerStateToString(m_player_state);
	std::string tracklength;

	drawTitle(np);
	switch (Config.design)
	{
		case Design::Classic:
			if (Statusbar::isUnlocked() && Config.statusbar_visibility)
			{
				if (Config.display_bitrate && m_kbps)
				{
					tracklength += "(";
					tracklength += boost::lexical_cast<std::string>(m_kbps);
					tracklength += " kbps) ";
				}
				tracklength += "[";
				if (m_total_time)
				{
					if (Config.display_remaining_time)
					{
						tracklength += "-";
						tracklength += MPD::Song::ShowTime(m_total_time-m_elapsed_time);
					}
					else
						tracklength += MPD::Song::ShowTime(m_elapsed_time);
					tracklength += "/";
					tracklength += MPD::Song::ShowTime(m_total_time);
				}
				else
					tracklength += MPD::Song::ShowTime(m_elapsed_time);
				tracklength += "]";
				NC::WBuffer np_song;
				Format::print(Config.song_status_wformat, np_song, &np);
				*wFooter << NC::XY(0, 1)
				         << NC::TermManip::ClearToEOL
				         << Config.player_state_color
				         << ps
				         << NC::FormattedColor::End<>(Config.player_state_color)
				         << " ";
				writeCyclicBuffer(
					np_song, *wFooter, playing_song_scroll_begin,
					wFooter->getWidth()-ps.length()-tracklength.length()-2, L" ** ");
				*wFooter << NC::XY(wFooter->getWidth()-tracklength.length(), 1)
				         << Config.statusbar_time_color
				         << tracklength
				         << NC::FormattedColor::End<>(Config.statusbar_time_color);
			}
			break;
		case Design::Alternative:
			if (Config.display_remaining_time)
			{
				tracklength = "-";
				tracklength += MPD::Song::ShowTime(m_total_time-m_elapsed_time);
			}
			else
				tracklength = MPD::Song::ShowTime(m_elapsed_time);
			if (m_total_time)
			{
				tracklength += "/";
				tracklength += MPD::Song::ShowTime(m_total_time);
			}
			// bitrate here doesn't look good, but it can be moved somewhere else later
			if (Config.display_bitrate && m_kbps)
			{
				tracklength += " (";
				tracklength += boost::lexical_cast<std::string>(m_kbps);
				tracklength += " kbps)";
			}

			NC::WBuffer first, second;
			Format::print(Config.new_header_first_line, first, &np);
			Format::print(Config.new_header_second_line, second, &np);

			size_t first_len = wideLength(first.str());
			size_t first_margin = std::max(tracklength.length()+1, VolumeState.length())*2;
			size_t first_start = first_len < COLS-first_margin
			                                 ? (COLS-first_len)/2
			                                 : tracklength.length()+1;
			size_t second_len = wideLength(second.str());
			size_t second_margin = (std::max(ps.length(), size_t(8))+1)*2;
			size_t second_start = second_len < COLS-second_margin
			                                   ? (COLS-second_len)/2
			                                   : ps.length()+1;
			if (!Global::SeekingInProgress)
				*wHeader << NC::XY(0, 0)
				         << NC::TermManip::ClearToEOL
				         << Config.statusbar_time_color
				         << tracklength
				         << NC::FormattedColor::End<>(Config.statusbar_time_color);

			*wHeader << NC::XY(first_start, 0);

			writeCyclicBuffer(first, *wHeader, first_line_scroll_begin,
			                  COLS-tracklength.length()-VolumeState.length()-1, L" ** ");

			*wHeader << NC::XY(0, 1)
			         << NC::TermManip::ClearToEOL
			         << Config.player_state_color
			         << ps
			         << NC::FormattedColor::End<>(Config.player_state_color)
			         << NC::XY(second_start, 1);

			writeCyclicBuffer(second, *wHeader, second_line_scroll_begin,
			                  COLS-ps.length()-8-2, L" ** ");

			*wHeader << NC::XY(wHeader->getWidth()-VolumeState.length(), 0)
			         << Config.volume_color
			         << VolumeState
			         << NC::FormattedColor::End<>(Config.volume_color);

			flags();
	}
	if (Progressbar::isUnlocked())
		Progressbar::draw(m_elapsed_time, m_total_time);
}

void Status::Changes::flags()
{
	if (!Config.header_visibility && Config.design == Design::Classic)
		return;

	std::string switch_state;
	switch (Config.design)
	{
		case Design::Classic:
			if (m_repeat)
				switch_state += m_repeat;
			if (m_random)
				switch_state += m_random;
			if (m_single)
				switch_state += m_single;
			if (m_consume)
				switch_state += m_consume;
			if (m_crossfade)
				switch_state += m_crossfade;
			if (m_db_updating)
				switch_state += m_db_updating;

			*wHeader << Config.state_line_color;
			mvwhline(wHeader->raw(), 1, 0, 0, COLS);
			*wHeader << NC::FormattedColor::End<>(Config.state_line_color);

			if (!switch_state.empty())
				*wHeader << NC::XY(COLS-switch_state.length()-3, 1)
				         << Config.state_line_color
				         << "["
				         << NC::FormattedColor::End<>(Config.state_line_color)
								 << Config.state_flags_color
								 << switch_state
				         << NC::FormattedColor::End<>(Config.state_flags_color)
				         << Config.state_line_color
								 << "]"
				         << NC::FormattedColor::End<>(Config.state_line_color);

			break;
		case Design::Alternative:
			switch_state += '[';
			switch_state += m_repeat ? m_repeat : '-';
			switch_state += m_random ? m_random : '-';
			switch_state += m_single ? m_single : '-';
			switch_state += m_consume ? m_consume : '-';
			switch_state += m_crossfade ? m_crossfade : '-';
			switch_state += m_db_updating ? m_db_updating : '-';
			switch_state += ']';
			*wHeader << NC::XY(COLS-switch_state.length(), 1)
			         << Config.state_flags_color
			         << switch_state
			         << NC::FormattedColor::End<>(Config.state_flags_color);
			if (!Config.header_visibility) // in this case also draw separator
			{
				*wHeader << Config.alternative_ui_separator_color;
				mvwhline(wHeader->raw(), 2, 0, 0, COLS);
				*wHeader << NC::FormattedColor::End<>(Config.alternative_ui_separator_color);
			}
			break;
	}
	wHeader->refresh();
}

void Status::Changes::mixer()
{
	if (!Config.display_volume_level
	    || (!Config.header_visibility && Config.design == Design::Classic))
		return;

	switch (Config.design)
	{
		case Design::Classic:
			VolumeState = " Volume: ";
			break;
		case Design::Alternative:
			VolumeState = " Vol: ";
			break;
	}
	if (m_volume < 0)
		VolumeState += "n/a";
	else
	{
		VolumeState += boost::lexical_cast<std::string>(m_volume);
		VolumeState += "%";
	}
	*wHeader << NC::XY(wHeader->getWidth()-VolumeState.length(), 0)
	         << Config.volume_color
	         << VolumeState
	         << NC::FormattedColor::End<>(Config.volume_color);
	wHeader->refresh();
}

void Status::Changes::outputs()
{
#	ifdef ENABLE_OUTPUTS
	myOutputs->fetchList();
	if (isVisible(myOutputs))
		myOutputs->refreshWindow();
#	endif // ENABLE_OUTPUTS
}
