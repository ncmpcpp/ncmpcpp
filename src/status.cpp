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
#include <iostream>
#include <stdexcept>

#include "browser.h"
#include "charset.h"
#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "media_library.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "settings.h"
#include "status.h"
#include "tag_editor.h"

using namespace Global;
using namespace MPD;

std::string Global::VolumeState;

bool Global::UpdateStatusImmediately = 0;
bool Global::RedrawStatusbar = 0;

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

void StatusbarGetStringHelper(const std::wstring &)
{
	TraceMpdStatus();
}

void StatusbarApplyFilterImmediately(const std::wstring &ws)
{
	static std::wstring cmp;
	if (cmp != ws)
	{
		myScreen->ApplyFilter(ToString(ws));
		cmp = ws;
	}
	myScreen->RefreshWindow();
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
	if (Mpd.GetState() < psPlay)
		Statusbar() << wclrtoeol;
}

void TraceMpdStatus()
{
	static timeval past, now;
	
	gettimeofday(&now, 0);
	if ((Mpd.Connected()
	&&   (/*(now.tv_sec == past.tv_sec && now.tv_usec >= past.tv_usec+500000) || */now.tv_sec > past.tv_sec))
	||  UpdateStatusImmediately
	   )
	{
		Mpd.UpdateStatus();
		BlockItemListUpdate = 0;
		Playlist::BlockUpdate = 0;
		UpdateStatusImmediately = 0;
		gettimeofday(&past, 0);
	}
	wFooter->Refresh();
	
	if (myScreen == myPlaylist && now.tv_sec == myPlaylist->Timer()+Config.playlist_disable_highlight_delay)
		myPlaylist->Main()->Highlighting(!Config.playlist_disable_highlight_delay);
	
	if (lock_statusbar_delay > 0)
	{
		if (now.tv_sec >= time_of_statusbar_lock+lock_statusbar_delay)
		{
			lock_statusbar_delay = -1;
			
			if (Config.statusbar_visibility)
				block_statusbar_update = !allow_statusbar_unlock;
			else
				block_progressbar_update = !allow_statusbar_unlock;
			
			if (Mpd.GetState() < psPlay && !block_statusbar_update)
				Statusbar() << wclrtoeol;
		}
	}
}

void NcmpcppErrorCallback(Connection *, int errorid, const char *msg, void *)
{
	if (errorid == MPD_ACK_ERROR_PERMISSION)
	{
		wFooter->SetGetStringHelper(NULL);
		Statusbar() << "Password: ";
		std::string password = wFooter->GetString(-1, 0, 1);
		Mpd.SetPassword(password);
		Mpd.SendPassword();
		Mpd.UpdateStatus();
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
	static int elapsed;
	static MPD::Song np;
	
	int sx, sy;
	*wFooter << fmtBold;
	wFooter->GetXY(sx, sy);
	
	if ((myPlaylist->NowPlaying != Mpd.GetCurrentSongPos() || changed.SongID) && !Playlist::BlockNowPlayingUpdate)
	{
		myPlaylist->OldPlaying = myPlaylist->NowPlaying;
		myPlaylist->NowPlaying = Mpd.GetCurrentSongPos();
		bool was_filtered = myPlaylist->Main()->isFiltered();
		myPlaylist->Main()->ShowAll();
		try
		{
			myPlaylist->Main()->BoldOption(myPlaylist->OldPlaying, 0);
		}
		catch (std::out_of_range) { }
		try
		{
			myPlaylist->Main()->BoldOption(myPlaylist->NowPlaying, 1);
		}
		catch (std::out_of_range) { }
		if (was_filtered)
			myPlaylist->Main()->ShowFiltered();
	}
	
	if (changed.Playlist)
	{
		if (!Playlist::BlockUpdate)
		{
			np = Mpd.GetCurrentSong();
			if (Mpd.GetState() > psStop)
				WindowTitle(utf_to_locale_cpy(np.toString(Config.song_window_title_format)));
			
			bool was_filtered = myPlaylist->Main()->isFiltered();
			myPlaylist->Main()->ShowAll();
			SongList list;
			
			size_t playlist_length = Mpd.GetPlaylistLength();
			if (playlist_length < myPlaylist->Main()->Size())
				myPlaylist->Main()->ResizeList(playlist_length);
			
			Mpd.GetPlaylistChanges(Mpd.GetOldPlaylistID(), list);
			myPlaylist->Main()->Reserve(playlist_length);
			for (SongList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				int pos = (*it)->GetPosition();
				if (pos < int(myPlaylist->Main()->Size()))
				{
					// if song's already in playlist, replace it with a new one
					myPlaylist->Main()->at(pos) = **it;
				}
				else
				{
					// otherwise just add it to playlist
					myPlaylist->Main()->AddOption(**it, myPlaylist->NowPlaying == pos);
				}
				myPlaylist->Main()->at(pos).CopyPtr(0);
				(*it)->NullMe();
			}
			
			if (myScreen == myPlaylist)
			{
				if (!playlist_length || myPlaylist->Main()->Size() < myPlaylist->Main()->GetHeight())
					myPlaylist->Main()->Window::Clear();
				myPlaylist->Main()->Refresh();
			}
			if (was_filtered)
			{
				myPlaylist->ApplyFilter(myPlaylist->Main()->GetFilter());
				if (myPlaylist->Main()->Empty())
					myPlaylist->Main()->ShowAll();
			}
			FreeSongList(list);
		}
		
		Playlist::ReloadTotalLength = 1;
		Playlist::ReloadRemaining = 1;
		
		if (myScreen == myPlaylist)
			RedrawHeader = 1;
		
		if (myPlaylist->Main()->Empty())
		{
			myPlaylist->Main()->Reset();
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
			myBrowser->GetDirectory(myBrowser->CurrentDir());
#		ifdef HAVE_TAGLIB_H
		if (myTagEditor->Main())
		{
			myTagEditor->Albums->Clear(0);
			myTagEditor->Dirs->Clear(0);
		}
#		endif // HAVE_TAGLIB_H
		if (myLibrary->Main())
		{
			if (myLibrary->Columns() == 2)
			{
				myLibrary->Albums->Clear();
				myLibrary->Songs->Clear(0);
			}
			else
				myLibrary->Artists->Clear(0);
		}
		if (myPlaylistEditor->Main())
			myPlaylistEditor->Content->Clear(0);
	}
	if (changed.PlayerState)
	{
		PlayerState mpd_state = Mpd.GetState();
		switch (mpd_state)
		{
			case psUnknown:
			{
				player_state = "[unknown]";
				break;
			}
			case psPlay:
			{
				WindowTitle(utf_to_locale_cpy(np.toString(Config.song_window_title_format)));
				player_state = Config.new_design ? "[playing]" : "Playing: ";
				Playlist::ReloadRemaining = 1;
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
				wFooter->SetColor(Config.progressbar_color);
				mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
				wFooter->SetColor(Config.statusbar_color);
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
		else if (!block_statusbar_update && Config.statusbar_visibility)
		{
			*wFooter << XY(0, 1);
			if (player_state.empty())
				*wFooter << wclrtoeol;
			else
				*wFooter << player_state;
		}
	}
	if (changed.SongID)
	{
		if (myPlaylist->isPlaying())
		{
			np = Mpd.GetCurrentSong();
			
			if (!Config.execute_on_song_change.empty())
				system(np.toString(Config.execute_on_song_change).c_str());
			if (Mpd.GetState() > psStop)
				WindowTitle(utf_to_locale_cpy(np.toString(Config.song_window_title_format)));
			if (Config.autocenter_mode && !myPlaylist->Main()->isFiltered())
				myPlaylist->Main()->Highlight(myPlaylist->NowPlaying);
			
			if (!Mpd.GetElapsedTime())
				mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
			
			if (Config.now_playing_lyrics && !Mpd.GetSingle() && myScreen == myLyrics && myOldScreen == myPlaylist)
				Lyrics::Reload = 1;
		}
		Playlist::ReloadRemaining = 1;
		
		playing_song_scroll_begin = 0;
		first_line_scroll_begin = 0;
		second_line_scroll_begin = 0;
		
		if (Mpd.GetState() == psPlay)
			changed.ElapsedTime = 1;
	}
	static time_t now, past = 0;
	time(&now);
	if (((now > past || changed.SongID) && Mpd.GetState() > psStop) || RedrawStatusbar)
	{
		time(&past);
		if (np.Empty())
		{
			np = Mpd.GetCurrentSong();
			WindowTitle(utf_to_locale_cpy(np.toString(Config.song_window_title_format)));
		}
		if (!np.Empty() && Mpd.GetState() > psStop)
		{
			int mpd_elapsed = Mpd.GetElapsedTime();
			if (elapsed < mpd_elapsed-2 || elapsed+1 > mpd_elapsed)
				elapsed = mpd_elapsed;
			else if (Mpd.GetState() == psPlay && !RedrawStatusbar)
				elapsed++;
			
			std::string tracklength;
			if (Config.new_design)
			{
				tracklength = Song::ShowTime(elapsed);
				if (np.GetTotalLength())
				{
					tracklength += "/";
					tracklength += np.GetLength();
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
				
				if (!block_progressbar_update) // if blocked, seeking in progress
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
				if (np.GetTotalLength())
				{
					tracklength = " [";
					tracklength += Song::ShowTime(elapsed);
					tracklength += "/";
					tracklength += np.GetLength();
					tracklength += "]";
				}
				else
				{
					tracklength = " [";
					tracklength += Song::ShowTime(elapsed);
					tracklength += "]";
				}
				*wFooter << XY(0, 1) << wclrtoeol << player_state
				<< fmtBoldEnd
				<< Scroller(utf_to_locale_cpy(np.toString(Config.song_status_format)), wFooter->GetWidth()-player_state.length()-tracklength.length(), playing_song_scroll_begin)
				<< fmtBold
				<< XY(wFooter->GetWidth()-tracklength.length(), 1) << tracklength;
			}
			if (!block_progressbar_update)
			{
				double progressbar_size = elapsed/double(np.GetTotalLength());
				int howlong = wFooter->GetWidth()*progressbar_size;
				wFooter->SetColor(Config.progressbar_color);
				mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
				if (np.GetTotalLength())
				{
					mvwhline(wFooter->Raw(), 0, 0, '=',howlong);
					mvwaddch(wFooter->Raw(), 0, howlong, '>');
				}
				wFooter->SetColor(Config.statusbar_color);
			}
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
	if (changed.StatusFlags && Config.header_visibility)
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
	if (changed.Volume && Config.header_visibility)
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
		wHeader->SetColor(Config.volume_color);
		*wHeader << XY(wHeader->GetWidth()-VolumeState.length(), 0) << VolumeState;
		wHeader->SetColor(Config.header_color);
		wHeader->Refresh();
	}
	if (myScreen == myPlaylist && !Playlist::BlockRefreshing)
		myPlaylist->Main()->Refresh();
	*wFooter << fmtBoldEnd;
	wFooter->GotoXY(sx, sy);
	wFooter->Refresh();
}

Window &Statusbar()
{
	*wFooter << XY(0, Config.statusbar_visibility) << wclrtoeol;
	return *wFooter;
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
