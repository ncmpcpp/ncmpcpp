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

#include <iostream>
#include <stdexcept>

#include "browser.h"
#include "charset.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "search_engine.h"
#include "settings.h"
#include "status_checker.h"

using namespace Global;
using namespace MPD;
using std::string;

string Global::volume_state;

bool Global::header_update_status = 0;

namespace
{
	time_t time_of_statusbar_lock;
	
	string switch_state;
	
	bool block_statusbar_update = 0;
	bool allow_statusbar_unlock = 1;
	bool repeat_one_allowed = 0;
	
	const string term_type = getenv("TERM") ? getenv("TERM") : "";
	
	void WindowTitle(const string &status)
	{
		if (term_type != "linux" && Config.set_window_title)
			std::cout << "\033]0;" << status << "\7";
	}
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
}

void TraceMpdStatus()
{
	if (Mpd->Connected())
		Mpd->UpdateStatus();
	time_t now = time(NULL);
	
	if (current_screen == csPlaylist && now == timer+Config.playlist_disable_highlight_delay)
		myPlaylist->Main()->Highlighting(!Config.playlist_disable_highlight_delay);
	
	if (lock_statusbar_delay > 0)
	{
		if (now >= time_of_statusbar_lock+lock_statusbar_delay)
		{
			lock_statusbar_delay = -1;
			
			if (Config.statusbar_visibility)
				block_statusbar_update = !allow_statusbar_unlock;
			else
				block_progressbar_update = !allow_statusbar_unlock;
			
			StatusChanges changes;
			switch (Mpd->GetState())
			{
				case psStop:
					changes.PlayerState = 1;
					NcmpcppStatusChanged(Mpd, changes, NULL);
					break;
				case psPause:
					changes.ElapsedTime = 1; // restore status
					NcmpcppStatusChanged(Mpd, changes, NULL);
					break;
				default:
					break;
			}
		}
	}
}

void NcmpcppErrorCallback(Connection *Mpd, int errorid, const char *msg, void *)
{
	if (errorid == MPD_ACK_ERROR_PERMISSION)
	{
		wFooter->SetGetStringHelper(NULL);
		Statusbar() << "Password: ";
		string password = wFooter->GetString(-1, 0, 1);
		Mpd->SetPassword(password);
		Mpd->SendPassword();
		Mpd->UpdateStatus();
		wFooter->SetGetStringHelper(TraceMpdStatus);
	}
	else
		ShowMessage("%s", msg);
}

void NcmpcppStatusChanged(Connection *Mpd, StatusChanges changed, void *)
{
	static size_t playing_song_scroll_begin = 0;
	static string player_state;
	
	int sx, sy;
	wFooter->Bold(1);
	wFooter->GetXY(sx, sy);
	
	if ((myPlaylist->NowPlaying != Mpd->GetCurrentSongPos() || changed.SongID) && !dont_change_now_playing)
	{
		myPlaylist->OldPlaying = myPlaylist->NowPlaying;
		myPlaylist->NowPlaying = Mpd->GetCurrentSongPos();
		try
		{
			myPlaylist->Main()->BoldOption(myPlaylist->OldPlaying, 0);
			myPlaylist->Main()->BoldOption(myPlaylist->NowPlaying, 1);
		}
		catch (std::out_of_range) { }
	}
	
	if (changed.Playlist)
	{
		if (!block_playlist_update)
		{
			SongList list;
			size_t playlist_length = Mpd->GetPlaylistLength();
			if (playlist_length != myPlaylist->Main()->Size())
			{
				if (playlist_length < myPlaylist->Main()->Size())
				{
					myPlaylist->Main()->Clear(playlist_length < myPlaylist->Main()->GetHeight() && current_screen == csPlaylist);
					Mpd->GetPlaylistChanges(-1, list);
				}
				else
					Mpd->GetPlaylistChanges(Mpd->GetOldPlaylistID(), list);
				
				myPlaylist->Main()->Reserve(playlist_length);
				for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
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
				
				if (current_screen == csPlaylist)
				{
					if (!playlist_length || myPlaylist->Main()->Size() < myPlaylist->Main()->GetHeight())
						myPlaylist->Main()->Window::Clear();
					myPlaylist->Main()->Refresh();
				}
			}
			else
			{
				Mpd->GetPlaylistChanges(-1, list);
				
				for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
				{
					if (*list[i] != myPlaylist->Main()->at(i))
					{
						myPlaylist->Main()->at(i) = *list[i];
						myPlaylist->Main()->at(i).CopyPtr(0);
						list[i]->NullMe();
					}
				}
			}
			FreeSongList(list);
		}
		
		if (current_screen == csPlaylist)
			redraw_header = 1;
		
		if (myPlaylist->Main()->Empty())
		{
			myPlaylist->Main()->Reset();
			ShowMessage("Cleared playlist!");
		}
		
		if (!block_item_list_update)
		{
			if (current_screen == csBrowser)
			{
				myBrowser->UpdateItemList();
			}
			else if (current_screen == csSearcher)
			{
				UpdateFoundList();
			}
			else if (current_screen == csLibrary)
			{
				UpdateSongList(mLibSongs);
			}
			else if (current_screen == csPlaylistEditor)
			{
				UpdateSongList(mPlaylistEditor);
			}
		}
	}
	if (changed.Database)
	{
		myBrowser->GetDirectory(myBrowser->CurrentDir());
#		ifdef HAVE_TAGLIB_H
		mEditorAlbums->Clear(0);
		mEditorDirs->Clear(0);
#		endif // HAVE_TAGLIB_H
		mLibArtists->Clear(0);
		mPlaylistEditor->Clear(0);
	}
	if (changed.PlayerState)
	{
		PlayerState mpd_state = Mpd->GetState();
		switch (mpd_state)
		{
			case psUnknown:
			{
				player_state = "[unknown]";
				break;
			}
			case psPlay:
			{
				player_state = "Playing: ";
				myPlaylist->Main()->BoldOption(myPlaylist->NowPlaying, 1);
				changed.ElapsedTime = 1;
				break;
			}
			case psPause:
			{
				player_state = "[Paused] ";
				break;
			}
			case psStop:
			{
				WindowTitle("ncmpc++ ver. "VERSION);
				wFooter->SetColor(Config.progressbar_color);
				mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
				wFooter->SetColor(Config.statusbar_color);
				try
				{
					myPlaylist->Main()->BoldOption(myPlaylist->OldPlaying, 0);
				}
				catch (std::out_of_range) { }
				myPlaylist->NowPlaying = -1;
				player_state.clear();
				break;
			}
		}
		if (!block_statusbar_update && Config.statusbar_visibility)
			wFooter->WriteXY(0, 1, player_state.empty(), "%s", player_state.c_str());
	}
	if (changed.SongID)
	{
		if (myPlaylist->isPlaying())
		{
			if (Config.repeat_one_mode && repeat_one_allowed)
			{
				std::swap(myPlaylist->NowPlaying, myPlaylist->OldPlaying);
				Mpd->Play(myPlaylist->NowPlaying);
			}
			try
			{
				myPlaylist->Main()->BoldOption(myPlaylist->OldPlaying, 0);
			}
			catch (std::out_of_range &) { }
			myPlaylist->Main()->BoldOption(myPlaylist->NowPlaying, 1);
			if (Config.autocenter_mode)
				myPlaylist->Main()->Highlight(myPlaylist->NowPlaying);
			repeat_one_allowed = 0;
			
			if (!Mpd->GetElapsedTime())
				mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
			
			if (Config.now_playing_lyrics && !Config.repeat_one_mode && current_screen == csLyrics && prev_screen == csPlaylist)
				reload_lyrics = 1;
		}
		playing_song_scroll_begin = 0;
		
		if (Mpd->GetState() == psPlay)
		{
			changed.ElapsedTime = 1;
		}
	}
	if (changed.ElapsedTime)
	{
		const Song &s = Mpd->GetCurrentSong();
		if (!player_state.empty() && !s.Empty())
		{
			int elapsed = Mpd->GetElapsedTime();
			
			// 'repeat one' mode check - be sure that we deal with item with known length
			if (s.GetTotalLength() && elapsed == s.GetTotalLength()-1)
				repeat_one_allowed = 1;
			
			WindowTitle(utf_to_locale_cpy(s.toString(Config.song_window_title_format)));
			
			if (!block_statusbar_update && Config.statusbar_visibility)
			{
				string tracklength;
				if (s.GetTotalLength())
				{
					tracklength = " [";
					tracklength += Song::ShowTime(elapsed);
					tracklength += "/";
					tracklength += s.GetLength();
					tracklength += "]";
				}
				else
				{
					tracklength = " [";
					tracklength += Song::ShowTime(elapsed);
					tracklength += "]";
				}
				wFooter->WriteXY(0, 1, 1, "%s", player_state.c_str());
				wFooter->Bold(0);
				*wFooter << Scroller(utf_to_locale_cpy(s.toString(Config.song_status_format)), wFooter->GetWidth()-player_state.length()-tracklength.length(), playing_song_scroll_begin);
				wFooter->Bold(1);
				
				wFooter->WriteXY(wFooter->GetWidth()-tracklength.length(), 1, 1, "%s", tracklength.c_str());
			}
			if (!block_progressbar_update)
			{
				double progressbar_size = (double)elapsed/(s.GetTotalLength());
				int howlong = wFooter->GetWidth()*progressbar_size;
				wFooter->SetColor(Config.progressbar_color);
				mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
				if (s.GetTotalLength())
				{
					mvwhline(wFooter->Raw(), 0, 0, '=',howlong);
					mvwaddch(wFooter->Raw(), 0, howlong, '>');
				}
				wFooter->SetColor(Config.statusbar_color);
			}
		}
		else
		{
			if (!block_statusbar_update && Config.statusbar_visibility)
				wFooter->WriteXY(0, 1, 1, "");
		}
	}
	
	static char mpd_repeat;
	static char mpd_random;
	static char mpd_crossfade;
	static char mpd_db_updating;
	
	if (changed.Repeat)
	{
		mpd_repeat = Mpd->GetRepeat() ? 'r' : 0;
		ShowMessage("Repeat is %s", !mpd_repeat ? "off" : "on");
		header_update_status = 1;

	}
	if (changed.Random)
	{
		mpd_random = Mpd->GetRandom() ? 'z' : 0;
		ShowMessage("Random is %s", !mpd_random ? "off" : "on");
		header_update_status = 1;
	}
	if (changed.Crossfade)
	{
		int crossfade = Mpd->GetCrossfade();
		mpd_crossfade = crossfade ? 'x' : 0;
		ShowMessage("Crossfade set to %d seconds", crossfade);
		header_update_status = 1;
	}
	if (changed.DBUpdating)
	{
		mpd_db_updating = Mpd->GetDBIsUpdating() ? 'U' : 0;
		ShowMessage(!mpd_db_updating ? "Database update finished!" : "Database update started!");
		header_update_status = 1;
	}
	if (header_update_status && Config.header_visibility)
	{
		switch_state.clear();
		if (mpd_repeat)
			switch_state += mpd_repeat;
		if (mpd_random)
			switch_state += mpd_random;
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
			attron(COLOR_PAIR(Config.state_line_color));
			mvprintw(1, COLS-2, "]");
		}
		attroff(A_BOLD|COLOR_PAIR(Config.state_line_color));
		refresh();
		header_update_status = 0;
	}
	if (changed.Volume && Config.header_visibility)
	{
		volume_state = " Volume: ";
		volume_state += IntoStr(Mpd->GetVolume());
		volume_state += "%";
		wHeader->SetColor(Config.volume_color);
		wHeader->WriteXY(wHeader->GetWidth()-volume_state.length(), 0, 1, "%s", volume_state.c_str());
		wHeader->SetColor(Config.header_color);
		wHeader->Refresh();
	}
	if (current_screen == csPlaylist)
		myPlaylist->Main()->Refresh();
	wFooter->Bold(0);
	wFooter->GotoXY(sx, sy);
	wFooter->Refresh();
}

void ShowMessage(const char *format, ...)
{
	if (messages_allowed)
	{
		time_of_statusbar_lock = time(NULL);
		lock_statusbar_delay = Config.message_delay_time;
		if (Config.statusbar_visibility)
			block_statusbar_update = 1;
		else
			block_progressbar_update = 1;
		wFooter->GotoXY(0, Config.statusbar_visibility);
		wFooter->Bold(0);
		va_list list;
		va_start(list, format);
		wmove(wFooter->Raw(), Config.statusbar_visibility, 0);
		vw_printw(wFooter->Raw(), format, list);
		wclrtoeol(wFooter->Raw());
		va_end(list);
		wFooter->Bold(1);
		wFooter->Refresh();
	}
}

