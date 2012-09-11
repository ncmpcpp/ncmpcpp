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

#include <cstring>
#include <sys/time.h>

#include <iostream>
#include <stdexcept>

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

using Global::myScreen;
using Global::myLockedScreen;
using Global::myInactiveScreen;

using Global::wFooter;
using Global::wHeader;

using Global::Timer;
using Global::VolumeState;

#ifndef USE_PDCURSES
void WindowTitle(const std::string &status)
{
	if (strcmp(getenv("TERM"), "linux") && Config.set_window_title)
		std::cout << "\033]0;" << status << "\7";
}
#endif // !USE_PDCURSES

void DrawNowPlayingTitle(MPD::Song &np)
{
	if (np.empty())
		np = myPlaylist->nowPlayingSong();
	if (!np.empty())
		WindowTitle(np.toString(Config.song_window_title_format));
}

void TraceMpdStatus()
{
	static timeval past = { 0, 0 };
	
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
	
	ApplyToVisibleWindows(&BasicScreen::Update);
	
	if (isVisible(myPlaylist) && myPlaylist->ActiveWindow() == myPlaylist->Items
	&&  Timer.tv_sec == myPlaylist->Timer().tv_sec+Config.playlist_disable_highlight_delay
	&&  myPlaylist->Items->isHighlighted()
	&&  Config.playlist_disable_highlight_delay)
	{
		myPlaylist->Items->setHighlighting(false);
		myPlaylist->Items->refresh();
	}
	
	Statusbar::tryRedraw();
}

void NcmpcppErrorCallback(MPD::Connection *, int errorid, const char *msg, void *)
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
		myBrowser->Refresh();
	}
	else
		Statusbar::msg("MPD: %s", msg);
}

void NcmpcppStatusChanged(MPD::Connection *, MPD::StatusChanges changed, void *)
{
	static size_t playing_song_scroll_begin = 0;
	static size_t first_line_scroll_begin = 0;
	static size_t second_line_scroll_begin = 0;
	static std::string player_state;
	
	MPD::Song np;
	
	int sx = wFooter->getX();
	int sy = wFooter->getY();
	
	if (changed.Playlist)
	{
		myPlaylist->Items->clearSearchResults();
		withUnfilteredMenuReapplyFilter(*myPlaylist->Items, []() {
			size_t playlist_length = Mpd.GetPlaylistLength();
			if (playlist_length < myPlaylist->Items->size())
			{
				auto it = myPlaylist->Items->begin()+playlist_length;
				auto end = myPlaylist->Items->end();
				for (; it != end; ++it)
					myPlaylist->unregisterHash(it->value().getHash());
				myPlaylist->Items->resizeList(playlist_length);
			}
			
			auto songs = Mpd.GetPlaylistChanges(Mpd.GetOldPlaylistID());
			for (auto s = songs.begin(); s != songs.end(); ++s)
			{
				size_t pos = s->getPosition();
				if (pos < myPlaylist->Items->size())
				{
					// if song's already in playlist, replace it with a new one
					MPD::Song &old_s = (*myPlaylist->Items)[pos].value();
					myPlaylist->unregisterHash(old_s.getHash());
					old_s = *s;
				}
				else // otherwise just add it to playlist
					myPlaylist->Items->addItem(*s);
				myPlaylist->registerHash(s->getHash());
			}
		});
		
		DrawNowPlayingTitle(np);
		
		Playlist::ReloadTotalLength = true;
		Playlist::ReloadRemaining = true;
		
		if (isVisible(myBrowser))
			markSongsInPlaylist(myBrowser->getProxySongList());
		if (isVisible(mySearcher))
			markSongsInPlaylist(mySearcher->getProxySongList());
		if (isVisible(myLibrary))
			markSongsInPlaylist(myLibrary->songsProxyList());
		if (isVisible(myPlaylistEditor))
			markSongsInPlaylist(myPlaylistEditor->contentProxyList());
	}
	if (changed.StoredPlaylists)
	{
		if (myPlaylistEditor->Main())
		{
			myPlaylistEditor->requestPlaylistsUpdate();
			myPlaylistEditor->requestContentsUpdate();
		}
		if (myBrowser->Main() && myBrowser->CurrentDir() == "/")
		{
			myBrowser->GetDirectory("/");
			if (isVisible(myBrowser))
				myBrowser->Refresh();
		}
	}
	if (changed.Database)
	{
		if (myBrowser->Main())
		{
			if (isVisible(myBrowser))
				myBrowser->GetDirectory(myBrowser->CurrentDir());
			else
				myBrowser->Main()->clear();
		}
#		ifdef HAVE_TAGLIB_H
		if (myTagEditor->Main())
		{
			myTagEditor->Dirs->clear();
		}
#		endif // HAVE_TAGLIB_H
		if (myLibrary->Main())
		{
			if (myLibrary->Columns() == 2)
				myLibrary->Albums->clear();
			else
				myLibrary->Tags->clear();
		}
		changed.DBUpdating = 1;
	}
	if (changed.PlayerState)
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
				DrawNowPlayingTitle(np);
				player_state = Config.new_design ? "[playing]" : "Playing: ";
				Playlist::ReloadRemaining = true;
				if (Mpd.GetOldState() == MPD::psStop) // show track info in status immediately
					changed.ElapsedTime = 1;
				break;
			}
			case MPD::psPause:
			{
				player_state = Config.new_design ? "[paused] " : "[Paused] ";
				break;
			}
			case MPD::psStop:
			{
				WindowTitle("ncmpcpp " VERSION);
				if (Progressbar::isUnlocked())
					Progressbar::draw(0, 0);
				Playlist::ReloadRemaining = true;
				if (Config.new_design)
				{
					*wHeader << NC::XY(0, 0) << wclrtoeol << NC::XY(0, 1) << wclrtoeol;
					player_state = "[stopped]";
					changed.Volume = 1;
					changed.StatusFlags = 1;
				}
				else
					player_state.clear();
#				ifdef ENABLE_VISUALIZER
				if (isVisible(myVisualizer))
					myVisualizer->Main()->clear();
#				endif // ENABLE_VISUALIZER
				break;
			}
		}
#		ifdef ENABLE_VISUALIZER
		if (myScreen == myVisualizer)
			wFooter->setTimeout(state == MPD::psPlay ? Visualizer::WindowTimeout : 500);
#		endif // ENABLE_VISUALIZER
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
	if (changed.SongID)
	{
		if (Mpd.isPlaying())
		{
			GNUC_UNUSED int res;
			if (!Config.execute_on_song_change.empty())
				res = system(Config.execute_on_song_change.c_str());
			
#			ifdef HAVE_CURL_CURL_H
			if (Config.fetch_lyrics_in_background)
				Lyrics::DownloadInBackground(myPlaylist->nowPlayingSong());
#			endif // HAVE_CURL_CURL_H
			
			DrawNowPlayingTitle(np);
			
			if (Config.autocenter_mode && !myPlaylist->Items->isFiltered())
				myPlaylist->Items->highlight(Mpd.GetCurrentlyPlayingSongPos());
			
			if (Config.now_playing_lyrics && isVisible(myLyrics) && Global::myOldScreen == myPlaylist)
				myLyrics->ReloadNP = 1;
		}
		Playlist::ReloadRemaining = true;
		playing_song_scroll_begin = 0;
		first_line_scroll_begin = 0;
		second_line_scroll_begin = 0;
	}
	if (changed.ElapsedTime || changed.SongID || Global::RedrawStatusbar)
	{
		if (Mpd.isPlaying())
		{
			DrawNowPlayingTitle(np);
			
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
				String2Buffer(ToWString(utf_to_locale_cpy(np.toString(Config.new_header_first_line, "$"))), first);
				String2Buffer(ToWString(utf_to_locale_cpy(np.toString(Config.new_header_second_line, "$"))), second);
				
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
				
				changed.StatusFlags = 1;
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
				String2Buffer(ToWString(utf_to_locale_cpy(np.toString(Config.song_status_format, "$"))), np_song);
				*wFooter << NC::XY(0, 1) << wclrtoeol << NC::fmtBold << player_state << NC::fmtBoldEnd;
				np_song.write(*wFooter, playing_song_scroll_begin, wFooter->getWidth()-player_state.length()-tracklength.length(), L" ** ");
				*wFooter << NC::fmtBold << NC::XY(wFooter->getWidth()-tracklength.length(), 1) << tracklength << NC::fmtBoldEnd;
			}
			if (Progressbar::isUnlocked())
				Progressbar::draw(Mpd.GetElapsedTime(), Mpd.GetTotalTime());
			Global::RedrawStatusbar = false;
		}
		else
		{
			 if (Statusbar::isUnlocked() && Config.statusbar_visibility)
				*wFooter << NC::XY(0, 1) << wclrtoeol;
		}
	}
	
	static char mpd_repeat;
	static char mpd_random;
	static char mpd_single;
	static char mpd_consume;
	static char mpd_crossfade;
	static char mpd_db_updating;
	
	if (changed.Repeat)
	{
		mpd_repeat = Mpd.GetRepeat() ? 'r' : 0;
		Statusbar::msg("Repeat mode is %s", !mpd_repeat ? "off" : "on");
	}
	if (changed.Random)
	{
		mpd_random = Mpd.GetRandom() ? 'z' : 0;
		Statusbar::msg("Random mode is %s", !mpd_random ? "off" : "on");
	}
	if (changed.Single)
	{
		mpd_single = Mpd.GetSingle() ? 's' : 0;
		Statusbar::msg("Single mode is %s", !mpd_single ? "off" : "on");
	}
	if (changed.Consume)
	{
		mpd_consume = Mpd.GetConsume() ? 'c' : 0;
		Statusbar::msg("Consume mode is %s", !mpd_consume ? "off" : "on");
	}
	if (changed.Crossfade)
	{
		int crossfade = Mpd.GetCrossfade();
		mpd_crossfade = crossfade ? 'x' : 0;
		Statusbar::msg("Crossfade set to %d seconds", crossfade);
	}
	if (changed.DBUpdating)
	{
		// mpd-0.{14,15} doesn't support idle notification that dbupdate had
		// finished and nothing changed, so we need to switch it off for them.
		if (!Mpd.SupportsIdle() || Mpd.Version() > 15)
			mpd_db_updating = Mpd.GetDBIsUpdating() ? 'U' : 0;
		Statusbar::msg(Mpd.GetDBIsUpdating() ? "Database update started!" : "Database update finished!");
		if (changed.Database && myScreen == mySelectedItemsAdder)
		{
			myScreen->SwitchTo(); // switch to previous screen
			Statusbar::msg("Database has changed, you need to select your item(s) once again");
		}
	}
	if (changed.StatusFlags && (Config.header_visibility || Config.new_design))
	{
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
	if (changed.Volume && Config.display_volume_level && (Config.header_visibility || Config.new_design))
	{
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
	if (changed.Outputs)
	{
#		ifdef ENABLE_OUTPUTS
		myOutputs->FetchList();
#		endif // ENABLE_OUTPUTS
	}
	wFooter->goToXY(sx, sy);
	if (changed.PlayerState || (changed.ElapsedTime && (!Config.new_design || Mpd.GetState() == MPD::psPlay)))
		wFooter->refresh();
	if (changed.Playlist || changed.Database || changed.PlayerState || changed.SongID)
		ApplyToVisibleWindows(&BasicScreen::RefreshWindow);
}

void DrawHeader()
{
	if (!Config.header_visibility)
		return;
	if (Config.new_design)
	{
		std::wstring title = myScreen->Title();
		*wHeader << NC::XY(0, 3) << wclrtoeol;
		*wHeader << NC::fmtBold << Config.alternative_ui_separator_color;
		mvwhline(wHeader->raw(), 2, 0, 0, COLS);
		mvwhline(wHeader->raw(), 4, 0, 0, COLS);
		*wHeader << NC::XY((COLS-wideLength(title))/2, 3);
		*wHeader << Config.header_color << title << NC::clEnd;
		*wHeader << NC::clEnd << NC::fmtBoldEnd;
	}
	else
	{
		*wHeader << NC::XY(0, 0) << wclrtoeol << NC::fmtBold << myScreen->Title() << NC::fmtBoldEnd;
		*wHeader << Config.volume_color;
		*wHeader << NC::XY(wHeader->getWidth()-VolumeState.length(), 0) << VolumeState;
		*wHeader << NC::clEnd;
	}
	wHeader->refresh();
}
