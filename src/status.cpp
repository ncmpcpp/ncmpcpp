/***************************************************************************
 *   Copyright (C) 2008-2013 by Andrzej Rybczak                            *
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

#include <sys/time.h>

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

timeval past = { 0, 0 };

size_t playing_song_scroll_begin = 0;
size_t first_line_scroll_begin = 0;
size_t second_line_scroll_begin = 0;

char m_repeat = 0;
char m_random = 0;
char m_single = 0;
char m_consume = 0;
char m_crossfade = 0;
char m_db_updating = 0;

MPD::Status m_status;
MPD::PlayerState m_player_state = MPD::psUnknown;
int m_volume = -1;
unsigned m_elapsed_time = 0;

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
			if (Config.new_design)
				result = "[playing]";
			else
				result = "Playing: ";
			break;
		case MPD::psPause:
			if (Config.new_design)
				result = "[paused] ";
			else
				result = "[Paused] ";
			break;
		case MPD::psStop:
			if (Config.new_design)
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
	Statusbar::msg("NCMPCPP: %s", e.what());
}

void Status::handleServerError(MPD::ServerError &e)
{
	if (e.code() == MPD_SERVER_ERROR_PERMISSION)
	{
		wFooter->setGetStringHelper(nullptr);
		Statusbar::put() << "Password: ";
		Mpd.SetPassword(wFooter->getString(-1, 0, 1));
		Mpd.SendPassword();
		Statusbar::msg("Password accepted");
		wFooter->setGetStringHelper(Statusbar::Helpers::getString);
	}
	else if (e.code() == MPD_SERVER_ERROR_NO_EXIST && myScreen == myBrowser)
	{
		myBrowser->GetDirectory(getParentDirectory(myBrowser->CurrentDir()));
		myBrowser->refresh();
	}
	Statusbar::msg("MPD: %s", e.what());
}

/*************************************************************************/

void Status::trace()
{
	gettimeofday(&Timer, 0);
	if (Mpd.Connected())
	{
		if (State::player() == MPD::psPlay && Global::Timer.tv_sec > past.tv_sec)
		{
			// update elapsed time/bitrate of the current song
			Status::Changes::elapsedTime(true);
			wFooter->refresh();
			past = Timer;
		}
		
		applyToVisibleWindows(&BaseScreen::update);
		
		if (isVisible(myPlaylist)
			&&  Timer.tv_sec == myPlaylist->Timer()+Config.playlist_disable_highlight_delay
			&&  myPlaylist->main().isHighlighted()
			&&  Config.playlist_disable_highlight_delay)
		{
			myPlaylist->main().setHighlighting(false);
			myPlaylist->main().refresh();
		}
		
		Statusbar::tryRedraw();
		
		Mpd.idle();
	}
}

void Status::update(int event)
{
	MPD::Status old_status = m_status;
	m_status = Mpd.getStatus();
	
	if (event & MPD_IDLE_DATABASE)
		Changes::database();
	if (event & MPD_IDLE_STORED_PLAYLIST)
		Changes::storedPlaylists();
	if (event & MPD_IDLE_PLAYLIST)
		Changes::playlist();
	if (event & MPD_IDLE_PLAYER)
	{
		Changes::playerState();
		if (old_status.empty() || old_status.currentSongID() != m_status.currentSongID())
			Changes::songID();
	}
	if (event & MPD_IDLE_MIXER)
		Changes::mixer();
	if (event & MPD_IDLE_OUTPUT)
		Changes::outputs();
	if (event & (MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS))
	{
		if (event & MPD_IDLE_UPDATE)
			Changes::dbUpdateState(!old_status.empty());
		if (event & MPD_IDLE_OPTIONS)
		{
			if (old_status.empty() || old_status.repeat() != m_status.repeat())
				Changes::repeat(!old_status.empty());
			if (old_status.empty() || old_status.random() != m_status.random())
				Changes::random(!old_status.empty());
			if (old_status.empty() || old_status.single() != m_status.single())
				Changes::single(!old_status.empty());
			if (old_status.empty() || old_status.consume() != m_status.consume())
				Changes::consume(!old_status.empty());
			if (old_status.empty() || old_status.crossfade() != m_status.crossfade())
				Changes::crossfade(!old_status.empty());
		}
		Changes::flags();
	}
	
	if (event & MPD_IDLE_PLAYER)
		wFooter->refresh();
	
	if (event & (MPD_IDLE_PLAYLIST | MPD_IDLE_DATABASE | MPD_IDLE_PLAYER))
		applyToVisibleWindows(&BaseScreen::refreshWindow);
}

/*************************************************************************/

bool Status::State::repeat()
{
	return m_repeat;
}

bool Status::State::random()
{
	return m_random;
}

bool Status::State::single()
{
	return m_single;
}

bool Status::State::consume()
{
	return m_consume;
}

bool Status::State::crossfade()
{
	return m_crossfade;
}

MPD::PlayerState Status::State::player()
{
	return m_player_state;
}

int Status::State::volume()
{
	return m_volume;
}

unsigned int Status::State::elapsedTime()
{
	return m_elapsed_time;
}

/*************************************************************************/

void Status::Changes::playlist()
{
	myPlaylist->main().clearSearchResults();
	withUnfilteredMenuReapplyFilter(myPlaylist->main(), []() {
		size_t playlist_length = m_status.playlistLength();
		if (playlist_length < myPlaylist->main().size())
		{
			auto it = myPlaylist->main().begin()+playlist_length;
			auto end = myPlaylist->main().end();
			for (; it != end; ++it)
				myPlaylist->unregisterHash(it->value().getHash());
			myPlaylist->main().resizeList(playlist_length);
		}
		
		Mpd.GetPlaylistChanges(myPlaylist->version(), [](MPD::Song &&s) {
			size_t pos = s.getPosition();
			if (pos < myPlaylist->main().size())
			{
				// if song's already in playlist, replace it with a new one
				MPD::Song &old_s = myPlaylist->main()[pos].value();
				myPlaylist->unregisterHash(old_s.getHash());
				old_s = s;
			}
			else // otherwise just add it to playlist
				myPlaylist->main().addItem(s);
			myPlaylist->registerHash(s.getHash());
		});
		
		myPlaylist->setStatus(m_status);
	});
	
	if (State::player() != MPD::psStop)
		drawTitle(myPlaylist->nowPlayingSong());
	
	Playlist::ReloadTotalLength = true;
	Playlist::ReloadRemaining = true;
	
	if (isVisible(myBrowser))
		markSongsInPlaylist(myBrowser->proxySongList());
	if (isVisible(mySearcher))
		markSongsInPlaylist(mySearcher->proxySongList());
	if (isVisible(myLibrary))
		markSongsInPlaylist(myLibrary->songsProxyList());
	if (isVisible(myPlaylistEditor))
		markSongsInPlaylist(myPlaylistEditor->contentProxyList());
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
	m_player_state = m_status.playerState();
	
	switch (State::player())
	{
		case MPD::psPlay:
			drawTitle(myPlaylist->nowPlayingSong());
			Playlist::ReloadRemaining = true;
			break;
		case MPD::psStop:
			windowTitle("ncmpcpp " VERSION);
			if (Progressbar::isUnlocked())
				Progressbar::draw(0, 0);
			Playlist::ReloadRemaining = true;
			if (Config.new_design)
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
	
#	ifdef ENABLE_VISUALIZER
	if (myScreen == myVisualizer)
		wFooter->setTimeout(State::player() == MPD::psPlay ? Visualizer::WindowTimeout : 500);
#	endif // ENABLE_VISUALIZER
	
	std::string state = playerStateToString(State::player());
	if (Config.new_design)
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
}

void Status::Changes::songID()
{
	// update information about current song
	myPlaylist->setStatus(m_status);
	Playlist::ReloadRemaining = true;
	playing_song_scroll_begin = 0;
	first_line_scroll_begin = 0;
	second_line_scroll_begin = 0;
	if (State::player() != MPD::psStop)
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
			myPlaylist->main().highlight(myPlaylist->currentSongPosition());
		
		if (Config.now_playing_lyrics && isVisible(myLyrics) && myLyrics->previousScreen() == myPlaylist)
			myLyrics->ReloadNP = 1;
	}
	elapsedTime(false);
}

void Status::Changes::elapsedTime(bool update_elapsed)
{
	MPD::Status st = m_status;
	if (update_elapsed)
	{
		st = Mpd.getStatus();
		m_elapsed_time = st.elapsedTime();
	}
	
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
	if (Config.new_design)
	{
		if (Config.display_remaining_time)
		{
			tracklength = "-";
			tracklength += MPD::Song::ShowTime(st.totalTime()-st.elapsedTime());
		}
		else
			tracklength = MPD::Song::ShowTime(st.elapsedTime());
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
	else if (Statusbar::isUnlocked() && Config.statusbar_visibility)
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
				tracklength += MPD::Song::ShowTime(st.totalTime()-st.elapsedTime());
			}
			else
				tracklength += MPD::Song::ShowTime(st.elapsedTime());
			tracklength += "/";
			tracklength += MPD::Song::ShowTime(st.totalTime());
			tracklength += "]";
		}
		else
		{
			tracklength += MPD::Song::ShowTime(st.elapsedTime());
			tracklength += "]";
		}
		NC::WBuffer np_song;
		stringToBuffer(ToWString(Charset::utf8ToLocale(np.toString(Config.song_status_format, Config.tags_separator, "$"))), np_song);
		*wFooter << NC::XY(0, 1) << wclrtoeol << NC::Format::Bold << ps << NC::Format::NoBold;
		writeCyclicBuffer(np_song, *wFooter, playing_song_scroll_begin, wFooter->getWidth()-ps.length()-tracklength.length(), L" ** ");
		*wFooter << NC::Format::Bold << NC::XY(wFooter->getWidth()-tracklength.length(), 1) << tracklength << NC::Format::NoBold;
	}
	if (Progressbar::isUnlocked())
		Progressbar::draw(st.elapsedTime(), st.totalTime());
}

void Status::Changes::repeat(bool show_msg)
{
	m_repeat = m_status.repeat() ? 'r' : 0;
	if (show_msg)
		Statusbar::msg("Repeat mode is %s", !m_repeat ? "off" : "on");
}

void Status::Changes::random(bool show_msg)
{
	m_random = m_status.random() ? 'z' : 0;
	if (show_msg)
		Statusbar::msg("Random mode is %s", !m_random ? "off" : "on");
}

void Status::Changes::single(bool show_msg)
{
	m_single = m_status.single() ? 's' : 0;
	if (show_msg)
		Statusbar::msg("Single mode is %s", !m_single ? "off" : "on");
}

void Status::Changes::consume(bool show_msg)
{
	m_consume = m_status.consume() ? 'c' : 0;
	if (show_msg)
		Statusbar::msg("Consume mode is %s", !m_consume ? "off" : "on");
}

void Status::Changes::crossfade(bool show_msg)
{
	int crossfade = m_status.crossfade();
	m_crossfade = crossfade ? 'x' : 0;
	if (show_msg)
		Statusbar::msg("Crossfade set to %d seconds", crossfade);
}

void Status::Changes::dbUpdateState(bool show_msg)
{
	m_db_updating = m_status.updateID() ? 'U' : 0;
	if (show_msg)
		Statusbar::msg("Database update %s", m_status.updateID() ? "started" : "finished");
}

void Status::Changes::flags()
{
	if (!Config.header_visibility && !Config.new_design)
		return;
	
	std::string switch_state;
	if (Config.new_design)
	{
		switch_state += '[';
		switch_state += m_repeat ? m_repeat : '-';
		switch_state += m_random ? m_random : '-';
		switch_state += m_single ? m_single : '-';
		switch_state += m_consume ? m_consume : '-';
		switch_state += m_crossfade ? m_crossfade : '-';
		switch_state += m_db_updating ? m_db_updating : '-';
		switch_state += ']';
		*wHeader << NC::XY(COLS-switch_state.length(), 1) << NC::Format::Bold << Config.state_flags_color << switch_state << NC::Color::End << NC::Format::NoBold;
		if (Config.new_design && !Config.header_visibility) // in this case also draw separator
		{
			*wHeader << NC::Format::Bold << NC::Color::Black;
			mvwhline(wHeader->raw(), 2, 0, 0, COLS);
			*wHeader << NC::Color::End << NC::Format::NoBold;
		}
		wHeader->refresh();
	}
	else
	{
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
		attroff(A_BOLD|COLOR_PAIR(int(Config.state_line_color)));
		refresh();
	}
}

void Status::Changes::mixer()
{
	m_volume = m_status.volume();
	if (!Config.display_volume_level || (!Config.header_visibility && !Config.new_design))
		return;
	
	VolumeState = Config.new_design ? " Vol: " : " Volume: ";
	if (State::volume() < 0)
		VolumeState += "n/a";
	else
	{
		VolumeState += boost::lexical_cast<std::string>(State::volume());
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
