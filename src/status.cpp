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

#include <boost/date_time/posix_time/posix_time.hpp>

#include "browser.h"
#include "charset.h"
#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "media_library.h"
#include "outputs.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "sel_items_adder.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "tag_editor.h"
#include "visualizer.h"
#include "title.h"
#include "utility/string.h"

using Global::myScreen;

using Global::wFooter;
using Global::wHeader;

using Global::Timer;
using Global::VolumeState;

namespace {//

boost::posix_time::ptime past = boost::posix_time::from_time_t(0);

size_t playing_song_scroll_begin = 0;
size_t first_line_scroll_begin = 0;
size_t second_line_scroll_begin = 0;

MPD::Status m_status;
unsigned m_elapsed_time = 0;

// local copies of these are needed to be independent
// of the order of idle events incoming from MPD.
char m_repeat = 0;
char m_random = 0;
char m_single = 0;
char m_consume = 0;
char m_crossfade = 0;
char m_db_updating = 0;
int m_current_song_id = 0;
unsigned m_playlist_version = 0;

void drawTitle(const MPD::Song &np)
{
	assert(!np.empty());
	windowTitle(np.toString(Config.song_window_title_format, Config.tags_separator));
}

std::string playerStateToString(MPD::PlayerState ps)
{
	std::string result;
	switch (ps)
	{
		case MPD::psUnknown:
			result = "[unknown]";
			break;
		case MPD::psPlay:
			if (Config.design == Design::Alternative)
				result = "[playing]";
			else
				result = "Playing: ";
			break;
		case MPD::psPause:
			if (Config.design == Design::Alternative)
				result = "[paused] ";
			else
				result = "[Paused] ";
			break;
		case MPD::psStop:
			if (Config.design == Design::Alternative)
				result = "[stopped]";
			break;
		default:
			break;
	}
	return result;
}

}

void Status::handleClientError(MPD::ClientError &e)
{
	if (!e.clearable())
		Mpd.Disconnect();
	Statusbar::printf("ncmpcpp: %1%", e.what());
}

void Status::handleServerError(MPD::ServerError &e)
{
	if (e.code() == MPD_SERVER_ERROR_PERMISSION)
	{
		wFooter->setGetStringHelper(nullptr);
		Statusbar::put() << "Password: ";
		Mpd.SetPassword(wFooter->getString(0, true));
		Mpd.SendPassword();
		Statusbar::print("Password accepted");
		wFooter->setGetStringHelper(Statusbar::Helpers::getString);
	}
	else if (e.code() == MPD_SERVER_ERROR_NO_EXIST && myScreen == myBrowser)
	{
		myBrowser->GetDirectory(getParentDirectory(myBrowser->CurrentDir()));
		myBrowser->refresh();
	}
	Statusbar::printf("MPD: %1%", e.what());
}

/*************************************************************************/

void Status::trace(bool update_timer, bool update_window_timeout)
{
	if (update_timer)
		Timer = boost::posix_time::microsec_clock::local_time();
	if (Mpd.Connected())
	{
		if (m_status.playerState() == MPD::psPlay
		&&  Global::Timer - past > boost::posix_time::seconds(1))
		{
			// update elapsed time/bitrate of the current song
			Status::Changes::elapsedTime(true);
			wFooter->refresh();
			past = Timer;
		}

		applyToVisibleWindows(&BaseScreen::update);
		Statusbar::tryRedraw();

		if (update_window_timeout)
		{
			// set appropriate window timeout
			int nc_wtimeout = std::numeric_limits<int>::max();
			applyToVisibleWindows([&nc_wtimeout](BaseScreen *s) {
				nc_wtimeout = std::min(nc_wtimeout, s->windowTimeout());
			});
			wFooter->setTimeout(nc_wtimeout);
		}

		Mpd.idle();
	}
}

void Status::update(int event)
{
	MPD::Status old_status = m_status;
	m_status = Mpd.getStatus();
	m_elapsed_time = m_status.elapsedTime();
	
	if (event & MPD_IDLE_DATABASE)
		Changes::database();
	if (event & MPD_IDLE_STORED_PLAYLIST)
		Changes::storedPlaylists();
	if (event & MPD_IDLE_PLAYLIST)
	{
		Changes::playlist(m_playlist_version);
		m_playlist_version = m_status.playlistVersion();
	}
	if (event & MPD_IDLE_PLAYER)
	{
		Changes::playerState();
		if (m_current_song_id != m_status.currentSongID())
		{
			Changes::songID();
			m_current_song_id = m_status.currentSongID();
		}
	}
	if (event & MPD_IDLE_MIXER)
		Changes::mixer();
	if (event & MPD_IDLE_OUTPUT)
		Changes::outputs();
	if (event & (MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS))
	{
		bool show_msg = !old_status.empty();
		if (event & MPD_IDLE_UPDATE)
			Changes::dbUpdateState(show_msg);
		if (event & MPD_IDLE_OPTIONS)
		{
			if (('r' == m_repeat) != m_status.repeat())
				Changes::repeat(show_msg);
			if (('z' == m_random) != m_status.random())
				Changes::random(show_msg);
			if (('s' == m_single) != m_status.single())
				Changes::single(show_msg);
			if (('c' == m_consume) != m_status.consume())
				Changes::consume(show_msg);
			if (('x' == m_crossfade) != m_status.crossfade())
				Changes::crossfade(show_msg);
		}
		Changes::flags();
	}
	
	if (event & MPD_IDLE_PLAYER)
		wFooter->refresh();
	
	if (event & (MPD_IDLE_PLAYLIST | MPD_IDLE_DATABASE | MPD_IDLE_PLAYER))
		applyToVisibleWindows(&BaseScreen::refreshWindow);
}

void Status::clear()
{
	// reset local variables
	m_status.clear();
	m_repeat = 0;
	m_random = 0;
	m_single = 0;
	m_consume = 0;
	m_crossfade = 0;
	m_db_updating = 0;
	m_current_song_id = 0;
	m_playlist_version = 0;
}

const MPD::Status &Status::get()
{
	return m_status;
}

unsigned Status::elapsedTime()
{
	return m_elapsed_time;
}

/*************************************************************************/

void Status::Changes::playlist(unsigned previous_version)
{
	myPlaylist->main().clearSearchResults();
	withUnfilteredMenuReapplyFilter(myPlaylist->main(), [previous_version]() {
		size_t playlist_length = m_status.playlistLength();
		if (playlist_length < myPlaylist->main().size())
		{
			auto it = myPlaylist->main().begin()+playlist_length;
			auto end = myPlaylist->main().end();
			for (; it != end; ++it)
				myPlaylist->unregisterSong(it->value());
			myPlaylist->main().resizeList(playlist_length);
		}
		
		Mpd.GetPlaylistChanges(previous_version, [](MPD::Song s) {
			size_t pos = s.getPosition();
			if (pos < myPlaylist->main().size())
			{
				// if song's already in playlist, replace it with a new one
				MPD::Song &old_s = myPlaylist->main()[pos].value();
				myPlaylist->unregisterSong(old_s);
				old_s = s;
			}
			else // otherwise just add it to playlist
				myPlaylist->main().addItem(s);
			myPlaylist->registerSong(s);
		});
	});
	
	if (m_status.playerState() != MPD::psStop)
		drawTitle(myPlaylist->nowPlayingSong());
	
	myPlaylist->reloadTotalLength();
	myPlaylist->reloadRemaining();
	
	if (isVisible(myBrowser))
		markSongsInPlaylist(myBrowser->proxySongList());
	if (isVisible(mySearcher))
		markSongsInPlaylist(mySearcher->proxySongList());
	if (isVisible(myLibrary))
	{
		markSongsInPlaylist(myLibrary->songsProxyList());
		myLibrary->Songs.refresh();
	}
	if (isVisible(myPlaylistEditor))
	{
		markSongsInPlaylist(myPlaylistEditor->contentProxyList());
		myPlaylistEditor->Content.refresh();
	}
}

void Status::Changes::storedPlaylists()
{
	myPlaylistEditor->requestPlaylistsUpdate();
	myPlaylistEditor->requestContentsUpdate();
	if (myBrowser->CurrentDir() == "/")
	{
		myBrowser->GetDirectory("/");
		if (isVisible(myBrowser))
			myBrowser->refresh();
	}
}

void Status::Changes::database()
{
	if (isVisible(myBrowser))
		myBrowser->GetDirectory(myBrowser->CurrentDir());
	else
		myBrowser->main().clear();
#	ifdef HAVE_TAGLIB_H
	myTagEditor->Dirs->clear();
#	endif // HAVE_TAGLIB_H
	myLibrary->requestTagsUpdate();
	myLibrary->requestAlbumsUpdate();
	myLibrary->requestSongsUpdate();
}

void Status::Changes::playerState()
{
	switch (m_status.playerState())
	{
		case MPD::psPlay:
			drawTitle(myPlaylist->nowPlayingSong());
			myPlaylist->reloadRemaining();
			break;
		case MPD::psStop:
			windowTitle("ncmpcpp " VERSION);
			if (Progressbar::isUnlocked())
				Progressbar::draw(0, 0);
			myPlaylist->reloadRemaining();
			if (Config.design == Design::Alternative)
			{
				*wHeader << NC::XY(0, 0) << wclrtoeol << NC::XY(0, 1) << wclrtoeol;
				mixer();
				flags();
			}
#			ifdef ENABLE_VISUALIZER
			if (isVisible(myVisualizer))
				myVisualizer->main().clear();
#			endif // ENABLE_VISUALIZER
			break;
		default:
			break;
	}
	
	std::string state = playerStateToString(m_status.playerState());
	if (Config.design == Design::Alternative)
	{
		*wHeader << NC::XY(0, 1) << NC::Format::Bold << state << NC::Format::NoBold;
		wHeader->refresh();
	}
	else if (Statusbar::isUnlocked() && Config.statusbar_visibility)
	{
		*wFooter << NC::XY(0, 1);
		if (state.empty())
			*wFooter << wclrtoeol;
		else
			*wFooter << NC::Format::Bold << state << NC::Format::NoBold;
	}
	
	// needed for immediate display after starting
	// player from stopped state or seeking
	elapsedTime(false);
}

void Status::Changes::songID()
{
	// update information about current song
	myPlaylist->reloadRemaining();
	playing_song_scroll_begin = 0;
	first_line_scroll_begin = 0;
	second_line_scroll_begin = 0;
	if (m_status.playerState() != MPD::psStop)
	{
		GNUC_UNUSED int res;
		if (!Config.execute_on_song_change.empty())
			res = system(Config.execute_on_song_change.c_str());
		
#		ifdef HAVE_CURL_CURL_H
		if (Config.fetch_lyrics_in_background)
			Lyrics::DownloadInBackground(myPlaylist->nowPlayingSong());
#		endif // HAVE_CURL_CURL_H
		
		drawTitle(myPlaylist->nowPlayingSong());
		
		if (Config.autocenter_mode && !myPlaylist->main().isFiltered())
			myPlaylist->main().highlight(Status::get().currentSongPosition());
		
		if (Config.now_playing_lyrics && isVisible(myLyrics) && myLyrics->previousScreen() == myPlaylist)
			myLyrics->ReloadNP = 1;
	}
	elapsedTime(false);
}

void Status::Changes::elapsedTime(bool update_elapsed)
{
	if (update_elapsed)
		m_elapsed_time = Mpd.getStatus().elapsedTime();
	const auto &st = m_status;
	
	if (st.playerState() == MPD::psStop)
	{
		if (Statusbar::isUnlocked() && Config.statusbar_visibility)
			*wFooter << NC::XY(0, 1) << wclrtoeol;
		return;
	}
	
	std::string ps = playerStateToString(st.playerState());
	MPD::Song np = myPlaylist->nowPlayingSong();
	drawTitle(np);
	
	std::string tracklength;
	switch (Config.design)
	{
		case Design::Classic:
			if (Statusbar::isUnlocked() && Config.statusbar_visibility)
			{
				if (Config.display_bitrate && st.kbps())
				{
					tracklength += " [";
					tracklength += boost::lexical_cast<std::string>(st.kbps());
					tracklength += " kbps]";
				}
				tracklength += " [";
				if (st.totalTime())
				{
					if (Config.display_remaining_time)
					{
						tracklength += "-";
						tracklength += MPD::Song::ShowTime(st.totalTime()-m_elapsed_time);
					}
					else
						tracklength += MPD::Song::ShowTime(m_elapsed_time);
					tracklength += "/";
					tracklength += MPD::Song::ShowTime(st.totalTime());
					tracklength += "]";
				}
				else
				{
					tracklength += MPD::Song::ShowTime(m_elapsed_time);
					tracklength += "]";
				}
				NC::WBuffer np_song;
				stringToBuffer(ToWString(Charset::utf8ToLocale(np.toString(Config.song_status_format, Config.tags_separator, "$"))), np_song);
				*wFooter << NC::XY(0, 1) << wclrtoeol << NC::Format::Bold << ps << NC::Format::NoBold;
				writeCyclicBuffer(np_song, *wFooter, playing_song_scroll_begin, wFooter->getWidth()-ps.length()-tracklength.length(), L" ** ");
				*wFooter << NC::Format::Bold << NC::XY(wFooter->getWidth()-tracklength.length(), 1) << tracklength << NC::Format::NoBold;
			}
			break;
		case Design::Alternative:
			if (Config.display_remaining_time)
			{
				tracklength = "-";
				tracklength += MPD::Song::ShowTime(st.totalTime()-m_elapsed_time);
			}
			else
				tracklength = MPD::Song::ShowTime(m_elapsed_time);
			if (st.totalTime())
			{
				tracklength += "/";
				tracklength += MPD::Song::ShowTime(st.totalTime());
			}
			// bitrate here doesn't look good, but it can be moved somewhere else later
			if (Config.display_bitrate && st.kbps())
			{
				tracklength += " ";
				tracklength += boost::lexical_cast<std::string>(st.kbps());
				tracklength += " kbps";
			}

			NC::WBuffer first, second;
			stringToBuffer(ToWString(Charset::utf8ToLocale(np.toString(Config.new_header_first_line, Config.tags_separator, "$"))), first);
			stringToBuffer(ToWString(Charset::utf8ToLocale(np.toString(Config.new_header_second_line, Config.tags_separator, "$"))), second);

			size_t first_len = wideLength(first.str());
			size_t first_margin = (std::max(tracklength.length()+1, VolumeState.length()))*2;
			size_t first_start = first_len < COLS-first_margin ? (COLS-first_len)/2 : tracklength.length()+1;

			size_t second_len = wideLength(second.str());
			size_t second_margin = (std::max(ps.length(), size_t(8))+1)*2;
			size_t second_start = second_len < COLS-second_margin ? (COLS-second_len)/2 : ps.length()+1;

			if (!Global::SeekingInProgress)
				*wHeader << NC::XY(0, 0) << wclrtoeol << tracklength;
			*wHeader << NC::XY(first_start, 0);
			writeCyclicBuffer(first, *wHeader, first_line_scroll_begin, COLS-tracklength.length()-VolumeState.length()-1, L" ** ");

			*wHeader << NC::XY(0, 1) << wclrtoeol << NC::Format::Bold << ps << NC::Format::NoBold;
			*wHeader << NC::XY(second_start, 1);
			writeCyclicBuffer(second, *wHeader, second_line_scroll_begin, COLS-ps.length()-8-2, L" ** ");

			*wHeader << NC::XY(wHeader->getWidth()-VolumeState.length(), 0) << Config.volume_color << VolumeState << NC::Color::End;

			flags();
	}
	if (Progressbar::isUnlocked())
		Progressbar::draw(m_elapsed_time, st.totalTime());
}

void Status::Changes::repeat(bool show_msg)
{
	m_repeat = m_status.repeat() ? 'r' : 0;
	if (show_msg)
		Statusbar::printf("Repeat mode is %1%", !m_repeat ? "off" : "on");
}

void Status::Changes::random(bool show_msg)
{
	m_random = m_status.random() ? 'z' : 0;
	if (show_msg)
		Statusbar::printf("Random mode is %1%", !m_random ? "off" : "on");
}

void Status::Changes::single(bool show_msg)
{
	m_single = m_status.single() ? 's' : 0;
	if (show_msg)
		Statusbar::printf("Single mode is %1%", !m_single ? "off" : "on");
}

void Status::Changes::consume(bool show_msg)
{
	m_consume = m_status.consume() ? 'c' : 0;
	if (show_msg)
		Statusbar::printf("Consume mode is %1%", !m_consume ? "off" : "on");
}

void Status::Changes::crossfade(bool show_msg)
{
	int crossfade = m_status.crossfade();
	m_crossfade = crossfade ? 'x' : 0;
	if (show_msg)
		Statusbar::printf("Crossfade set to %1% seconds", crossfade);
}

void Status::Changes::dbUpdateState(bool show_msg)
{
	m_db_updating = m_status.updateID() ? 'U' : 0;
	if (show_msg)
		Statusbar::printf("Database update %1%", m_status.updateID() ? "started" : "finished");
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

			// this is done by raw ncurses because creating another
			// window only for handling this is quite silly
			attrset(A_BOLD|COLOR_PAIR(int(Config.state_line_color)));
			mvhline(1, 0, 0, COLS);
			if (!switch_state.empty())
			{
				mvprintw(1, COLS-switch_state.length()-3, "[");
				attroff(COLOR_PAIR(int(Config.state_line_color)));
				attron(COLOR_PAIR(int(Config.state_flags_color)));
				mvprintw(1, COLS-switch_state.length()-2, "%s", switch_state.c_str());
				attroff(COLOR_PAIR(int(Config.state_flags_color)));
				attron(COLOR_PAIR(int(Config.state_line_color)));
				mvprintw(1, COLS-2, "]");
			}
			attrset(0);
			refresh();
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
			*wHeader << NC::XY(COLS-switch_state.length(), 1) << NC::Format::Bold << Config.state_flags_color << switch_state << NC::Color::End << NC::Format::NoBold;
			if (!Config.header_visibility) // in this case also draw separator
			{
				*wHeader << NC::Format::Bold << NC::Color::Black;
				mvwhline(wHeader->raw(), 2, 0, 0, COLS);
				*wHeader << NC::Color::End << NC::Format::NoBold;
			}
			wHeader->refresh();
			break;
	}
}

void Status::Changes::mixer()
{
	if (!Config.display_volume_level || (!Config.header_visibility && Config.design == Design::Classic))
		return;
	
	switch (Config.design)
	{
		case Design::Classic:
			VolumeState = " " "Volume" ": ";
			break;
		case Design::Alternative:
			VolumeState = " " "Vol" ": ";
			break;
	}
	if (m_status.volume() < 0)
		VolumeState += "n/a";
	else
	{
		VolumeState += boost::lexical_cast<std::string>(m_status.volume());
		VolumeState += "%";
	}
	*wHeader << Config.volume_color;
	*wHeader << NC::XY(wHeader->getWidth()-VolumeState.length(), 0) << VolumeState;
	*wHeader << NC::Color::End;
	wHeader->refresh();
}

void Status::Changes::outputs()
{
#	ifdef ENABLE_OUTPUTS
	myOutputs->FetchList();
#	endif // ENABLE_OUTPUTS
}
