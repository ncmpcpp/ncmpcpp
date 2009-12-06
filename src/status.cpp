/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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
#include "settings.h"
#include "status.h"
#include "tag_editor.h"

using namespace Global;
using namespace MPD;

bool Global::UpdateStatusImmediately = 0;
bool Global::RedrawStatusbar = 0;

std::string Global::VolumeState;

timeval Global::Timer;

namespace
{
	time_t time_of_statusbar_lock;
	int lock_statusbar_delay = -1;
	
	bool block_statusbar_update = 0;
	bool block_progressbar_update = 0;
	bool allow_statusbar_unlock = 1;
}

#ifndef USE_PDCURSES
void WindowTitle(const std::string &status)
{
	if (strcmp(getenv("TERM"), "linux") && Config.set_window_title)
		std::cout << "\033]0;" << status << "\7";
}
#endif // !USE_PDCURSES

void StatusbarMPDCallback()
{
	Mpd.OrderDataFetching();
}

void StatusbarGetStringHelper(const std::wstring &)
{
	TraceMpdStatus();
}

void StatusbarApplyFilterImmediately(const std::wstring &ws)
{
	static std::wstring cmp;
	if (cmp != ws)
	{
		myScreen->ApplyFilter(ToString((cmp = ws)));
		myScreen->RefreshWindow();
	}
	TraceMpdStatus();
}

void LockProgressbar()
{
	block_progressbar_update = 1;
}

void UnlockProgressbar()
{
	block_progressbar_update = 0;
}

void LockStatusbar()
{
	if (Config.statusbar_visibility)
		block_statusbar_update = 1;
	else
		block_progressbar_update = 1;
	allow_statusbar_unlock = 0;
}

void UnlockStatusbar()
{
	allow_statusbar_unlock = 1;
	if (lock_statusbar_delay < 0)
	{
		if (Config.statusbar_visibility)
			block_statusbar_update = 0;
		else
			block_progressbar_update = 0;
	}
	if (!Mpd.isPlaying())
	{
		Statusbar() << wclrtoeol;
		wFooter->Refresh();
	}
}

void TraceMpdStatus()
{
	static timeval past = { 0, 0 };
	
	gettimeofday(&Global::Timer, 0);
	if (Mpd.Connected() && (Mpd.SupportsIdle() || Timer.tv_sec > past.tv_sec || UpdateStatusImmediately))
	{
		Mpd.UpdateStatus();
		BlockItemListUpdate = 0;
		Playlist::BlockUpdate = 0;
		UpdateStatusImmediately = 0;
		if (!Mpd.SupportsIdle())
		{
			gettimeofday(&past, 0);
		}
		else if (Config.display_bitrate && Global::Timer.tv_sec > past.tv_sec && Mpd.isPlaying())
		{
			// ncmpcpp doesn't fetch status constantly if mpd supports
			// idle mode so current song's bitrate is never updated.
			// we need to force ncmpcpp to fetch it.
			Mpd.OrderDataFetching();
			gettimeofday(&past, 0);
		}
	}
	
	myScreen->Update();
	
	if (myScreen->ActiveWindow() == myPlaylist->Items
	&&  Timer.tv_sec == myPlaylist->Timer()+Config.playlist_disable_highlight_delay
	&&  myPlaylist->Items->isHighlighted()
	&&  Config.playlist_disable_highlight_delay)
	{
		myPlaylist->Items->Highlighting(0);
		myPlaylist->Items->Refresh();
	}
	
	if (lock_statusbar_delay > 0)
	{
		if (Timer.tv_sec >= time_of_statusbar_lock+lock_statusbar_delay)
		{
			lock_statusbar_delay = -1;
			
			if (Config.statusbar_visibility)
				block_statusbar_update = !allow_statusbar_unlock;
			else
				block_progressbar_update = !allow_statusbar_unlock;
			
			if (Mpd.GetState() != psPlay && !block_statusbar_update && !block_progressbar_update)
			{
				if (Config.new_design)
					DrawProgressbar(Mpd.GetElapsedTime(), Mpd.GetTotalTime());
				else
					Statusbar() << wclrtoeol;
				wFooter->Refresh();
			}
		}
	}
}

void NcmpcppErrorCallback(Connection *, int errorid, const char *msg, void *)
{
	if (errorid == MPD_SERVER_ERROR_PERMISSION)
	{
		wFooter->SetGetStringHelper(0);
		Statusbar() << "Password: ";
		Mpd.SetPassword(wFooter->GetString(-1, 0, 1));
		if (Mpd.SendPassword())
			ShowMessage("Password accepted!");
		wFooter->SetGetStringHelper(StatusbarGetStringHelper);
	}
	else
		ShowMessage("%s", msg);
}

void NcmpcppStatusChanged(Connection *, StatusChanges changed, void *)
{
	static size_t playing_song_scroll_begin = 0;
	static size_t first_line_scroll_begin = 0;
	static size_t second_line_scroll_begin = 0;
	static std::string player_state;
	static MPD::Song np;
	
	int sx, sy;
	*wFooter << fmtBold;
	wFooter->GetXY(sx, sy);
	
	if (!Playlist::BlockNowPlayingUpdate)
		myPlaylist->NowPlaying = Mpd.GetCurrentSongPos();
	
	if (changed.Playlist)
	{
		if (!Playlist::BlockUpdate)
		{
			if (!(np = Mpd.GetCurrentSong()).Empty())
				WindowTitle(utf_to_locale_cpy(np.toString(Config.song_window_title_format)));
			
			bool was_filtered = myPlaylist->Items->isFiltered();
			myPlaylist->Items->ShowAll();
			SongList list;
			
			size_t playlist_length = Mpd.GetPlaylistLength();
			if (playlist_length < myPlaylist->Items->Size())
				myPlaylist->Items->ResizeList(playlist_length);
			
			Mpd.GetPlaylistChanges(Mpd.GetOldPlaylistID(), list);
			myPlaylist->Items->Reserve(playlist_length);
			for (SongList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				int pos = (*it)->GetPosition();
				if (pos < int(myPlaylist->Items->Size()))
				{
					// if song's already in playlist, replace it with a new one
					myPlaylist->Items->at(pos) = **it;
				}
				else
				{
					// otherwise just add it to playlist
					myPlaylist->Items->AddOption(**it);
				}
				myPlaylist->Items->at(pos).CopyPtr(0);
				(*it)->NullMe();
			}
			if (was_filtered)
			{
				myPlaylist->ApplyFilter(myPlaylist->Items->GetFilter());
				if (myPlaylist->Items->Empty())
					myPlaylist->Items->ShowAll();
			}
			FreeSongList(list);
		}
		
		Playlist::ReloadTotalLength = 1;
		Playlist::ReloadRemaining = 1;
		
		if (myPlaylist->Items->Empty())
		{
			myPlaylist->Items->Reset();
			myPlaylist->Items->Window::Clear();
			ShowMessage("Cleared playlist!");
		}
		
		if (!BlockItemListUpdate)
		{
			if (myScreen == myBrowser)
			{
				myBrowser->UpdateItemList();
			}
			else if (myScreen == mySearcher)
			{
				mySearcher->UpdateFoundList();
			}
			else if (myScreen == myLibrary)
			{
				UpdateSongList(myLibrary->Songs);
			}
			else if (myScreen == myPlaylistEditor)
			{
				UpdateSongList(myPlaylistEditor->Content);
			}
		}
	}
	if (changed.Database)
	{
		if (myBrowser->Main())
		{
			if (myScreen == myBrowser)
				myBrowser->GetDirectory(myBrowser->CurrentDir());
			else
				myBrowser->Main()->Clear();
		}
#		ifdef HAVE_TAGLIB_H
		if (myTagEditor->Main())
		{
			myTagEditor->Albums->Clear();
			myTagEditor->Dirs->Clear();
		}
#		endif // HAVE_TAGLIB_H
		if (myLibrary->Main())
		{
			if (myLibrary->Columns() == 2)
				myLibrary->Albums->Clear();
			else
				myLibrary->Artists->Clear();
		}
		if (myPlaylistEditor->Main())
			myPlaylistEditor->Content->Clear();
	}
	if (changed.PlayerState)
	{
		switch (Mpd.GetState())
		{
			case psUnknown:
			{
				player_state = "[unknown]";
				break;
			}
			case psPlay:
			{
				if (!np.Empty())
					WindowTitle(utf_to_locale_cpy(np.toString(Config.song_window_title_format)));
				player_state = Config.new_design ? "[playing]" : "Playing: ";
				Playlist::ReloadRemaining = 1;
				if (Mpd.GetOldState() == psStop) // show track info in status immediately
					changed.ElapsedTime = 1;
				break;
			}
			case psPause:
			{
				player_state = Config.new_design ? "[paused] " : "[Paused] ";
				break;
			}
			case psStop:
			{
				WindowTitle("ncmpc++ ver. "VERSION);
				if (!block_progressbar_update)
				{
					*wFooter << Config.progressbar_color;
					mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
					*wFooter << clEnd;
				}
				Playlist::ReloadRemaining = 1;
				myPlaylist->NowPlaying = -1;
				if (Config.new_design)
				{
					*wHeader << XY(0, 0) << wclrtoeol << XY(0, 1) << wclrtoeol;
					player_state = "[stopped]";
					changed.Volume = 1;
					changed.StatusFlags = 1;
				}
				else
					player_state.clear();
				break;
			}
		}
		if (Config.new_design)
		{
			*wHeader << XY(0, 1) << fmtBold << player_state << fmtBoldEnd;
			wHeader->Refresh();
		}
		else if (!block_statusbar_update && Config.statusbar_visibility && player_state.empty())
			*wFooter << XY(0, 1) << wclrtoeol;
	}
	if (changed.SongID)
	{
		if (myPlaylist->isPlaying())
		{
			if (!Config.execute_on_song_change.empty())
				system(Config.execute_on_song_change.c_str());
			
			if (Mpd.isPlaying() && !(np = Mpd.GetCurrentSong()).Empty())
				WindowTitle(utf_to_locale_cpy(np.toString(Config.song_window_title_format)));
			
			if (Config.autocenter_mode && !myPlaylist->Items->isFiltered())
				myPlaylist->Items->Highlight(myPlaylist->NowPlaying);
			
			if (Config.now_playing_lyrics && !Mpd.GetSingle() && myScreen == myLyrics && myOldScreen == myPlaylist)
				Lyrics::Reload = 1;
		}
		Playlist::ReloadRemaining = 1;
		playing_song_scroll_begin = 0;
		first_line_scroll_begin = 0;
		second_line_scroll_begin = 0;
	}
	if (changed.ElapsedTime || changed.SongID || RedrawStatusbar)
	{
		if (np.Empty() && !(np = Mpd.GetCurrentSong()).Empty())
			WindowTitle(utf_to_locale_cpy(np.toString(Config.song_window_title_format)));
		if (!np.Empty() && Mpd.isPlaying())
		{
			std::string tracklength;
			if (Config.new_design)
			{
				if (Config.display_remaining_time)
				{
					tracklength = "-";
					tracklength += Song::ShowTime(Mpd.GetTotalTime()-Mpd.GetElapsedTime());
				}
				else
					tracklength = Song::ShowTime(Mpd.GetElapsedTime());
				if (Mpd.GetTotalTime())
				{
					tracklength += "/";
					tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime());
				}
				// bitrate here doesn't look good, but it can be moved somewhere else later
				if (Config.display_bitrate && Mpd.GetBitrate())
				{
					tracklength += " ";
					tracklength += IntoStr(Mpd.GetBitrate());
					tracklength += " kbps";
				}
				
				basic_buffer<my_char_t> first, second;
				String2Buffer(TO_WSTRING(utf_to_locale_cpy(np.toString(Config.new_header_first_line))), first);
				String2Buffer(TO_WSTRING(utf_to_locale_cpy(np.toString(Config.new_header_second_line))), second);
				
				size_t first_len = Window::Length(first.Str());
				size_t first_margin = (std::max(tracklength.length()+1, VolumeState.length()))*2;
				size_t first_start = first_len < COLS-first_margin ? (COLS-first_len)/2 : tracklength.length()+1;
				
				size_t second_len = Window::Length(second.Str());
				size_t second_margin = (std::max(player_state.length(), size_t(8))+1)*2;
				size_t second_start = second_len < COLS-second_margin ? (COLS-second_len)/2 : player_state.length()+1;
				
				if (!SeekingInProgress)
					*wHeader << XY(0, 0) << wclrtoeol << tracklength;
				*wHeader << XY(first_start, 0);
				first.Write(*wHeader, first_line_scroll_begin, COLS-tracklength.length()-VolumeState.length()-1, U(" ** "));
				
				*wHeader << XY(0, 1) << wclrtoeol << fmtBold << player_state << fmtBoldEnd;
				*wHeader << XY(second_start, 1);
				second.Write(*wHeader, second_line_scroll_begin, COLS-player_state.length()-8-2, U(" ** "));
				
				*wHeader << XY(wHeader->GetWidth()-VolumeState.length(), 0) << Config.volume_color << VolumeState << clEnd;
				
				changed.StatusFlags = 1;
			}
			else if (!block_statusbar_update && Config.statusbar_visibility)
			{
				if (Config.display_bitrate && Mpd.GetBitrate())
				{
					tracklength += " [";
					tracklength += IntoStr(Mpd.GetBitrate());
					tracklength += " kbps]";
				}
				tracklength += " [";
				if (Mpd.GetTotalTime())
				{
					if (Config.display_remaining_time)
					{
						tracklength += "-";
						tracklength += Song::ShowTime(Mpd.GetTotalTime()-Mpd.GetElapsedTime());
					}
					else
						tracklength += Song::ShowTime(Mpd.GetElapsedTime());
					tracklength += "/";
					tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime());
					tracklength += "]";
				}
				else
				{
					tracklength += Song::ShowTime(Mpd.GetElapsedTime());
					tracklength += "]";
				}
				basic_buffer<my_char_t> np_song;
				String2Buffer(TO_WSTRING(utf_to_locale_cpy(np.toString(Config.song_status_format))), np_song);
				*wFooter << XY(0, 1) << wclrtoeol << player_state << fmtBoldEnd;
				np_song.Write(*wFooter, playing_song_scroll_begin, wFooter->GetWidth()-player_state.length()-tracklength.length(), U(" ** "));
				*wFooter << fmtBold << XY(wFooter->GetWidth()-tracklength.length(), 1) << tracklength;
			}
			if (!block_progressbar_update)
				DrawProgressbar(Mpd.GetElapsedTime(), Mpd.GetTotalTime());
			RedrawStatusbar = 0;
		}
		else
		{
			if (!block_statusbar_update && Config.statusbar_visibility)
				*wFooter << XY(0, 1) << wclrtoeol;
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
		ShowMessage("Repeat mode is %s", !mpd_repeat ? "off" : "on");
	}
	if (changed.Random)
	{
		mpd_random = Mpd.GetRandom() ? 'z' : 0;
		ShowMessage("Random mode is %s", !mpd_random ? "off" : "on");
	}
	if (changed.Single)
	{
		mpd_single = Mpd.GetSingle() ? 's' : 0;
		ShowMessage("Single mode is %s", !mpd_single ? "off" : "on");
	}
	if (changed.Consume)
	{
		mpd_consume = Mpd.GetConsume() ? 'c' : 0;
		ShowMessage("Consume mode is %s", !mpd_consume ? "off" : "on");
	}
	if (changed.Crossfade)
	{
		int crossfade = Mpd.GetCrossfade();
		mpd_crossfade = crossfade ? 'x' : 0;
		ShowMessage("Crossfade set to %d seconds", crossfade);
	}
	if (changed.DBUpdating)
	{
		mpd_db_updating = Mpd.GetDBIsUpdating() ? 'U' : 0;
		ShowMessage(!mpd_db_updating ? "Database update finished!" : "Database update started!");
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
			*wHeader << XY(COLS-switch_state.length(), 1) << fmtBold << Config.state_flags_color << switch_state << clEnd << fmtBoldEnd;
			if (Config.new_design && !Config.header_visibility) // in this case also draw separator
			{
				*wHeader << fmtBold << clBlack;
				mvwhline(wHeader->Raw(), 2, 0, 0, COLS);
				*wHeader << clEnd << fmtBoldEnd;
			}
			wHeader->Refresh();
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
	if (changed.Volume && (Config.header_visibility || Config.new_design))
	{
		VolumeState = Config.new_design ? " Vol: " : " Volume: ";
		int volume = Mpd.GetVolume();
		if (volume < 0)
			VolumeState += "n/a";
		else
		{
			VolumeState += IntoStr(volume);
			VolumeState += "%";
		}
		*wHeader << Config.volume_color;
		*wHeader << XY(wHeader->GetWidth()-VolumeState.length(), 0) << VolumeState;
		*wHeader << clEnd;
		wHeader->Refresh();
	}
	if (changed.Outputs)
	{
#		ifdef ENABLE_OUTPUTS
		myOutputs->FetchList();
#		endif // ENABLE_OUTPUTS
	}
	*wFooter << fmtBoldEnd;
	wFooter->GotoXY(sx, sy);
	if (changed.PlayerState || (changed.ElapsedTime && (!Config.new_design || Mpd.GetState() == psPlay)))
		wFooter->Refresh();
	if (changed.Playlist || changed.Database || changed.PlayerState || changed.SongID)
		myScreen->RefreshWindow();
}

Window &Statusbar()
{
	*wFooter << XY(0, Config.statusbar_visibility) << wclrtoeol;
	return *wFooter;
}

void DrawProgressbar(unsigned elapsed, unsigned time)
{
	unsigned howlong = time ? wFooter->GetWidth()*elapsed/time : 0;
	*wFooter << fmtBold << Config.progressbar_color;
	mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
	if (time)
	{
		unsigned pb_width = std::min(size_t(howlong), wFooter->GetWidth());
		for (unsigned i = 0; i < pb_width; ++i)
			*wFooter << Config.progressbar[0];
		if (howlong < wFooter->GetWidth())
			*wFooter << Config.progressbar[1];
	}
	*wFooter << clEnd << fmtBoldEnd;
}

void ShowMessage(const char *format, ...)
{
	if (MessagesAllowed && allow_statusbar_unlock)
	{
		time(&time_of_statusbar_lock);
		lock_statusbar_delay = Config.message_delay_time;
		if (Config.statusbar_visibility)
			block_statusbar_update = 1;
		else
			block_progressbar_update = 1;
		wFooter->GotoXY(0, Config.statusbar_visibility);
		*wFooter << fmtBoldEnd;
		va_list list;
		va_start(list, format);
		wmove(wFooter->Raw(), Config.statusbar_visibility, 0);
		vw_printw(wFooter->Raw(), format, list);
		wclrtoeol(wFooter->Raw());
		va_end(list);
		wFooter->Refresh();
	}
}
