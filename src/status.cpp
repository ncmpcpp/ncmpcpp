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
std::string player_state;

char mpd_repeat;
char mpd_random;
char mpd_single;
char mpd_consume;
char mpd_crossfade;
char mpd_db_updating;

void drawTitle(const MPD::Song &np)
{
	assert(!np.empty());
	windowTitle(np.toString(Config.song_window_title_format, Config.tags_separator));
}

}

void Status::trace()
{
	gettimeofday(&Timer, 0);
	if (Mpd.Connected() && (Mpd.SupportsIdle() || Timer.tv_sec > past.tv_sec))
	{
		if (!Mpd.SupportsIdle())
		{
			past = Timer;
		}
		else if (Config.display_bitrate && Global::Timer.tv_sec > past.tv_sec && Mpd.isPlaying())
		{
			// ncmpcpp doesn't fetch status constantly if mpd supports
			// idle mode so current song's bitrate is never updated.
			// we need to force ncmpcpp to fetch it.
			Mpd.OrderDataFetching();
			past = Timer;
		}
		Mpd.UpdateStatus();
	}
	
	applyToVisibleWindows(&BaseScreen::update);
	
	if (isVisible(myPlaylist)
	&&  Timer.tv_sec == myPlaylist->Timer().tv_sec+Config.playlist_disable_highlight_delay
	&&  Timer.tv_usec > myPlaylist->Timer().tv_usec
	&&  myPlaylist->main().isHighlighted()
	&&  Config.playlist_disable_highlight_delay)
	{
		myPlaylist->main().setHighlighting(false);
		myPlaylist->main().refresh();
	}
	
	Statusbar::tryRedraw();
}

void Status::handleError(MPD::Connection * , int errorid, const char *msg, void *)
{
	// for errorid:
	// - 0-7 bits define MPD_ERROR_* codes, compare them with (0xff & errorid)
	// - 8-15 bits define MPD_SERVER_ERROR_* codes, compare them with (errorid >> 8)
	if ((errorid >> 8) == MPD_SERVER_ERROR_PERMISSION)
	{
		wFooter->setGetStringHelper(0);
		Statusbar::put() << "Password: ";
		Mpd.SetPassword(wFooter->getString(-1, 0, 1));
		if (Mpd.SendPassword())
			Statusbar::msg("Password accepted");
		wFooter->setGetStringHelper(Statusbar::Helpers::getString);
	}
	else if ((errorid >> 8) == MPD_SERVER_ERROR_NO_EXIST && myScreen == myBrowser)
	{
		myBrowser->GetDirectory(getParentDirectory(myBrowser->CurrentDir()));
		myBrowser->refresh();
	}
	else
		Statusbar::msg("MPD: %s", msg);
}

void Status::Changes::playlist()
{
	myPlaylist->main().clearSearchResults();
	withUnfilteredMenuReapplyFilter(myPlaylist->main(), []() {
		size_t playlist_length = Mpd.GetPlaylistLength();
		if (playlist_length < myPlaylist->main().size())
		{
			auto it = myPlaylist->main().begin()+playlist_length;
			auto end = myPlaylist->main().end();
			for (; it != end; ++it)
				myPlaylist->unregisterHash(it->value().getHash());
			myPlaylist->main().resizeList(playlist_length);
		}
		
		auto songs = Mpd.GetPlaylistChanges(Mpd.GetOldPlaylistID());
		for (auto s = songs.begin(); s != songs.end(); ++s)
		{
			size_t pos = s->getPosition();
			if (pos < myPlaylist->main().size())
			{
				// if song's already in playlist, replace it with a new one
				MPD::Song &old_s = myPlaylist->main()[pos].value();
				myPlaylist->unregisterHash(old_s.getHash());
				old_s = *s;
			}
			else // otherwise just add it to playlist
				myPlaylist->main().addItem(*s);
			myPlaylist->registerHash(s->getHash());
		}
	});
	
	if (Mpd.isPlaying())
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
	if (myLibrary->Columns() == 2)
		myLibrary->Albums.clear();
	else
		myLibrary->Tags.clear();
}

void Status::Changes::playerState()
{
	MPD::PlayerState state = Mpd.GetState();
	switch (state)
	{
		case MPD::psUnknown:
		{
			player_state = "[unknown]";
			break;
		}
		case MPD::psPlay:
		{
			drawTitle(myPlaylist->nowPlayingSong());
			player_state = Config.new_design ? "[playing]" : "Playing: ";
			Playlist::ReloadRemaining = true;
			if (Mpd.GetOldState() == MPD::psStop) // show track info in status immediately
				elapsedTime();
			break;
		}
		case MPD::psPause:
		{
			player_state = Config.new_design ? "[paused] " : "[Paused] ";
			break;
		}
		case MPD::psStop:
		{
			windowTitle("ncmpcpp " VERSION);
			if (Progressbar::isUnlocked())
				Progressbar::draw(0, 0);
			Playlist::ReloadRemaining = true;
			if (Config.new_design)
			{
				*wHeader << NC::XY(0, 0) << wclrtoeol << NC::XY(0, 1) << wclrtoeol;
				player_state = "[stopped]";
				mixer();
				flags();
			}
			else
				player_state.clear();
#			ifdef ENABLE_VISUALIZER
			if (isVisible(myVisualizer))
				myVisualizer->main().clear();
#			endif // ENABLE_VISUALIZER
			break;
		}
	}
	
#	ifdef ENABLE_VISUALIZER
	if (myScreen == myVisualizer)
		wFooter->setTimeout(state == MPD::psPlay ? Visualizer::WindowTimeout : 500);
#	endif // ENABLE_VISUALIZER
	
	if (Config.new_design)
	{
		*wHeader << NC::XY(0, 1) << NC::fmtBold << player_state << NC::fmtBoldEnd;
		wHeader->refresh();
	}
	else if (Statusbar::isUnlocked() && Config.statusbar_visibility)
	{
		*wFooter << NC::XY(0, 1);
		if (player_state.empty())
			*wFooter << wclrtoeol;
		else
			*wFooter << NC::fmtBold << player_state << NC::fmtBoldEnd;
	}
}

void Status::Changes::songID()
{
	Playlist::ReloadRemaining = true;
	playing_song_scroll_begin = 0;
	first_line_scroll_begin = 0;
	second_line_scroll_begin = 0;
	if (Mpd.isPlaying())
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
			myPlaylist->main().highlight(Mpd.GetCurrentlyPlayingSongPos());
		
		if (Config.now_playing_lyrics && isVisible(myLyrics) && myLyrics->previousScreen() == myPlaylist)
			myLyrics->ReloadNP = 1;
		
		elapsedTime();
	}
}

void Status::Changes::elapsedTime()
{
	if (!Mpd.isPlaying())
	{
		if (Statusbar::isUnlocked() && Config.statusbar_visibility)
			*wFooter << NC::XY(0, 1) << wclrtoeol;
		return;
	}
	
	MPD::Song np = myPlaylist->nowPlayingSong();
	drawTitle(np);
	
	std::string tracklength;
	if (Config.new_design)
	{
		if (Config.display_remaining_time)
		{
			tracklength = "-";
			tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime()-Mpd.GetElapsedTime());
		}
		else
			tracklength = MPD::Song::ShowTime(Mpd.GetElapsedTime());
		if (Mpd.GetTotalTime())
		{
			tracklength += "/";
			tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime());
		}
		// bitrate here doesn't look good, but it can be moved somewhere else later
		if (Config.display_bitrate && Mpd.GetBitrate())
		{
			tracklength += " ";
			tracklength += intTo<std::string>::apply(Mpd.GetBitrate());
			tracklength += " kbps";
		}
		
		NC::WBuffer first, second;
		stringToBuffer(ToWString(IConv::utf8ToLocale(np.toString(Config.new_header_first_line, Config.tags_separator, "$"))), first);
		stringToBuffer(ToWString(IConv::utf8ToLocale(np.toString(Config.new_header_second_line, Config.tags_separator, "$"))), second);
		
		size_t first_len = wideLength(first.str());
		size_t first_margin = (std::max(tracklength.length()+1, VolumeState.length()))*2;
		size_t first_start = first_len < COLS-first_margin ? (COLS-first_len)/2 : tracklength.length()+1;
		
		size_t second_len = wideLength(second.str());
		size_t second_margin = (std::max(player_state.length(), size_t(8))+1)*2;
		size_t second_start = second_len < COLS-second_margin ? (COLS-second_len)/2 : player_state.length()+1;
		
		if (!Global::SeekingInProgress)
			*wHeader << NC::XY(0, 0) << wclrtoeol << tracklength;
		*wHeader << NC::XY(first_start, 0);
		first.write(*wHeader, first_line_scroll_begin, COLS-tracklength.length()-VolumeState.length()-1, L" ** ");
		
		*wHeader << NC::XY(0, 1) << wclrtoeol << NC::fmtBold << player_state << NC::fmtBoldEnd;
		*wHeader << NC::XY(second_start, 1);
		second.write(*wHeader, second_line_scroll_begin, COLS-player_state.length()-8-2, L" ** ");
		
		*wHeader << NC::XY(wHeader->getWidth()-VolumeState.length(), 0) << Config.volume_color << VolumeState << NC::clEnd;
		
		flags();
	}
	else if (Statusbar::isUnlocked() && Config.statusbar_visibility)
	{
		if (Config.display_bitrate && Mpd.GetBitrate())
		{
			tracklength += " [";
			tracklength += intTo<std::string>::apply(Mpd.GetBitrate());
			tracklength += " kbps]";
		}
		tracklength += " [";
		if (Mpd.GetTotalTime())
		{
			if (Config.display_remaining_time)
			{
				tracklength += "-";
				tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime()-Mpd.GetElapsedTime());
			}
			else
				tracklength += MPD::Song::ShowTime(Mpd.GetElapsedTime());
			tracklength += "/";
			tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime());
			tracklength += "]";
		}
		else
		{
			tracklength += MPD::Song::ShowTime(Mpd.GetElapsedTime());
			tracklength += "]";
		}
		NC::WBuffer np_song;
		stringToBuffer(ToWString(IConv::utf8ToLocale(np.toString(Config.song_status_format, Config.tags_separator, "$"))), np_song);
		*wFooter << NC::XY(0, 1) << wclrtoeol << NC::fmtBold << player_state << NC::fmtBoldEnd;
		np_song.write(*wFooter, playing_song_scroll_begin, wFooter->getWidth()-player_state.length()-tracklength.length(), L" ** ");
		*wFooter << NC::fmtBold << NC::XY(wFooter->getWidth()-tracklength.length(), 1) << tracklength << NC::fmtBoldEnd;
	}
	if (Progressbar::isUnlocked())
		Progressbar::draw(Mpd.GetElapsedTime(), Mpd.GetTotalTime());
}

void Status::Changes::repeat()
{
	mpd_repeat = Mpd.GetRepeat() ? 'r' : 0;
	Statusbar::msg("Repeat mode is %s", !mpd_repeat ? "off" : "on");
}

void Status::Changes::random()
{
	mpd_random = Mpd.GetRandom() ? 'z' : 0;
	Statusbar::msg("Random mode is %s", !mpd_random ? "off" : "on");
}

void Status::Changes::single()
{
	mpd_single = Mpd.GetSingle() ? 's' : 0;
	Statusbar::msg("Single mode is %s", !mpd_single ? "off" : "on");
}

void Status::Changes::consume()
{
	mpd_consume = Mpd.GetConsume() ? 'c' : 0;
	Statusbar::msg("Consume mode is %s", !mpd_consume ? "off" : "on");
}

void Status::Changes::crossfade()
{
	int crossfade = Mpd.GetCrossfade();
	mpd_crossfade = crossfade ? 'x' : 0;
	Statusbar::msg("Crossfade set to %d seconds", crossfade);
}

void Status::Changes::dbUpdateState()
{
	mpd_db_updating = Mpd.GetDBIsUpdating() ? 'U' : 0;
	Statusbar::msg(Mpd.GetDBIsUpdating() ? "Database update started" : "Database update finished");
}

void Status::Changes::flags()
{
	if (!Config.header_visibility && !Config.new_design)
		return;
	
	std::string switch_state;
	if (Config.new_design)
	{
		switch_state += '[';
		switch_state += mpd_repeat ? mpd_repeat : '-';
		switch_state += mpd_random ? mpd_random : '-';
		switch_state += mpd_single ? mpd_single : '-';
		switch_state += mpd_consume ? mpd_consume : '-';
		switch_state += mpd_crossfade ? mpd_crossfade : '-';
		switch_state += mpd_db_updating ? mpd_db_updating : '-';
		switch_state += ']';
		*wHeader << NC::XY(COLS-switch_state.length(), 1) << NC::fmtBold << Config.state_flags_color << switch_state << NC::clEnd << NC::fmtBoldEnd;
		if (Config.new_design && !Config.header_visibility) // in this case also draw separator
		{
			*wHeader << NC::fmtBold << NC::clBlack;
			mvwhline(wHeader->raw(), 2, 0, 0, COLS);
			*wHeader << NC::clEnd << NC::fmtBoldEnd;
		}
		wHeader->refresh();
	}
	else
	{
		if (mpd_repeat)
			switch_state += mpd_repeat;
		if (mpd_random)
			switch_state += mpd_random;
		if (mpd_single)
			switch_state += mpd_single;
		if (mpd_consume)
			switch_state += mpd_consume;
		if (mpd_crossfade)
			switch_state += mpd_crossfade;
		if (mpd_db_updating)
			switch_state += mpd_db_updating;
		
		// this is done by raw ncurses because creating another
		// window only for handling this is quite silly
		attrset(A_BOLD|COLOR_PAIR(Config.state_line_color));
		mvhline(1, 0, 0, COLS);
		if (!switch_state.empty())
		{
			mvprintw(1, COLS-switch_state.length()-3, "[");
			attroff(COLOR_PAIR(Config.state_line_color));
			attron(COLOR_PAIR(Config.state_flags_color));
			mvprintw(1, COLS-switch_state.length()-2, "%s", switch_state.c_str());
			attroff(COLOR_PAIR(Config.state_flags_color));
			attron(COLOR_PAIR(Config.state_line_color));
			mvprintw(1, COLS-2, "]");
		}
		attroff(A_BOLD|COLOR_PAIR(Config.state_line_color));
		refresh();
	}
}

void Status::Changes::mixer()
{
	if (!Config.display_volume_level || (!Config.header_visibility && !Config.new_design))
		return;
	
	VolumeState = Config.new_design ? " Vol: " : " Volume: ";
	int volume = Mpd.GetVolume();
	if (volume < 0)
		VolumeState += "n/a";
	else
	{
		VolumeState += intTo<std::string>::apply(volume);
		VolumeState += "%";
	}
	*wHeader << Config.volume_color;
	*wHeader << NC::XY(wHeader->getWidth()-VolumeState.length(), 0) << VolumeState;
	*wHeader << NC::clEnd;
	wHeader->refresh();
}

void Status::Changes::outputs()
{
#	ifdef ENABLE_OUTPUTS
	myOutputs->FetchList();
#	endif // ENABLE_OUTPUTS
}

void Status::update(MPD::Connection *, MPD::StatusChanges changes, void *)
{
	if (changes.Playlist)
		Changes::playlist();
	if (changes.StoredPlaylists)
		Changes::storedPlaylists();
	if (changes.Database)
		Changes::database();
	if (changes.PlayerState)
		Changes::playerState();
	if (changes.SongID)
		Changes::songID();
	if (changes.ElapsedTime)
		Changes::elapsedTime();
	if (changes.Repeat)
		Changes::repeat();
	if (changes.Random)
		Changes::random();
	if (changes.Single)
		Changes::single();
	if (changes.Consume)
		Changes::consume();
	if (changes.Crossfade)
		Changes::crossfade();
	if (changes.DBUpdating)
		Changes::dbUpdateState();
	if (changes.StatusFlags)
		Changes::flags();
	if (changes.Volume)
		Changes::mixer();
	if (changes.Outputs)
		Changes::outputs();
	
	if (changes.PlayerState || (changes.ElapsedTime && (!Config.new_design || Mpd.GetState() == MPD::psPlay)))
		wFooter->refresh();
	if (changes.Playlist || changes.Database || changes.PlayerState || changes.SongID)
		applyToVisibleWindows(&BaseScreen::refreshWindow);
}
