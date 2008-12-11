/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "browser.h"
#include "helpers.h"
#include "search_engine.h"
#include "settings.h"
#include "status_checker.h"

#define UPDATE_WINDOW_TITLE WindowTitle(Mpd->GetCurrentSong().toString(Config.song_window_title_format))

using namespace MPD;

extern Connection *Mpd;

extern Menu<Song> *mPlaylist;
extern Menu<Item> *mBrowser;
extern Menu<string> *mLibArtists;
extern Menu<Song> *mLibSongs;
extern Menu<Song> *mPlaylistEditor;

#ifdef HAVE_TAGLIB_H
extern Menu<string> *mEditorAlbums;
extern Menu<string> *mEditorDirs;
#endif // HAVE_TAGLIB_H

extern Window *wHeader;
extern Window *wFooter;

extern time_t timer;

extern int now_playing;
extern int lock_statusbar_delay;

extern string browsed_dir;

extern NcmpcppScreen current_screen;
extern NcmpcppScreen prev_screen;

extern bool dont_change_now_playing;
extern bool block_progressbar_update;
extern bool block_playlist_update;
extern bool block_item_list_update;

extern bool redraw_screen;
extern bool redraw_header;
extern bool reload_lyrics;

int old_playing;

time_t time_of_statusbar_lock;

string playlist_stats;
string volume_state;
string switch_state;

bool block_statusbar_update = 0;
bool allow_statusbar_unlock = 1;
bool header_update_status = 0;
bool repeat_one_allowed = 0;

//time_t past, now = time(NULL)-1;

void TraceMpdStatus()
{
	//past = time(NULL);
	//if (past == now+1)
	//{
		Mpd->UpdateStatus();
		time_t now = time(NULL);
	//}
	
	if (current_screen == csPlaylist && now == timer+Config.playlist_disable_highlight_delay)
		mPlaylist->Highlighting(!Config.playlist_disable_highlight_delay);

	if (lock_statusbar_delay > 0)
	{
		if (now >= time_of_statusbar_lock+lock_statusbar_delay)
		{
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
	//wHeader->WriteXY(0,1, IntoStr(now_playing), 1);
}

void NcmpcppErrorCallback(Connection *Mpd, int errorid, const char *msg, void *)
{
	if (errorid == MPD_ACK_ERROR_PERMISSION)
	{
		wFooter->SetGetStringHelper(NULL);
		Statusbar() << "Password: ";
		string password = wFooter->GetString();
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
	
	if ((now_playing != Mpd->GetCurrentSongPos() || changed.SongID) && !dont_change_now_playing)
	{
		old_playing = now_playing;
		now_playing = Mpd->GetCurrentSongPos();
		try
		{
			mPlaylist->BoldOption(old_playing, 0);
			mPlaylist->BoldOption(now_playing, 1);
		}
		catch (std::out_of_range) { }
	}
	
	if (changed.Playlist)
	{
		if (!block_playlist_update)
		{
			SongList list;
			size_t playlist_length = Mpd->GetPlaylistLength();
			if (playlist_length != mPlaylist->Size())
			{
				if (playlist_length < mPlaylist->Size())
				{
					mPlaylist->Clear(playlist_length < mPlaylist->GetHeight() && current_screen == csPlaylist);
					Mpd->GetPlaylistChanges(-1, list);
				}
				else
					Mpd->GetPlaylistChanges(Mpd->GetOldPlaylistID(), list);
				
				mPlaylist->Reserve(playlist_length);
				for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					mPlaylist->AddOption(**it, now_playing == (*it)->GetPosition());
					mPlaylist->Back().CopyPtr(0);
					(*it)->NullMe();
				}
				
				if (current_screen == csPlaylist)
				{
					if (!playlist_length || mPlaylist->Size() < mPlaylist->GetHeight())
						mPlaylist->Window::Clear();
					mPlaylist->Refresh();
				}
			}
			else
			{
				Mpd->GetPlaylistChanges(-1, list);
				
				for (size_t i = 0; i < mPlaylist->Size(); i++)
				{
					if (*list[i] != mPlaylist->at(i))
					{
						mPlaylist->at(i) = *list[i];
						mPlaylist->at(i).CopyPtr(0);
						list[i]->NullMe();
					}
				}
			}
			FreeSongList(list);
		}
		
		if (current_screen == csPlaylist)
			redraw_header = 1;
		
		if (mPlaylist->Empty())
		{
			playlist_stats.clear();
			mPlaylist->Reset();
			ShowMessage("Cleared playlist!");
		}
		else
			playlist_stats = "(" + IntoStr(mPlaylist->Size()) + (mPlaylist->Size() == 1 ? " item" : " items") + TotalPlaylistLength() + ")";
		
		if (!block_item_list_update)
		{
			if (current_screen == csBrowser)
			{
				UpdateItemList(mBrowser);
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
		GetDirectory(browsed_dir);
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
				mPlaylist->BoldOption(now_playing, 1);
				UPDATE_WINDOW_TITLE;
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
					mPlaylist->BoldOption(old_playing, 0);
				}
				catch (std::out_of_range) { }
				now_playing = -1;
				player_state.clear();
				break;
			}
		}
		if (!block_statusbar_update && Config.statusbar_visibility)
			wFooter->WriteXY(0, 1, player_state, player_state.empty());
	}
	if (changed.SongID)
	{
		if (!mPlaylist->Empty() && now_playing >= 0)
		{
			if (Config.repeat_one_mode && repeat_one_allowed)
			{
				std::swap(now_playing, old_playing);
				Mpd->Play(now_playing);
			}
			if (old_playing >= 0)
				mPlaylist->BoldOption(old_playing, 0);
			mPlaylist->BoldOption(now_playing, 1);
			if (Config.autocenter_mode)
				mPlaylist->Highlight(now_playing);
			repeat_one_allowed = 0;
			
			if (!Mpd->GetElapsedTime())
				mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
			
			if (Config.now_playing_lyrics && !Config.repeat_one_mode && current_screen == csLyrics && prev_screen == csPlaylist)
				reload_lyrics = 1;
		}
		playing_song_scroll_begin = 0;
		
		if (Mpd->GetState() == psPlay)
		{
			UPDATE_WINDOW_TITLE;
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
			
			if (!block_statusbar_update && Config.statusbar_visibility)
			{
				string tracklength;
				if (s.GetTotalLength())
					tracklength = " [" + Song::ShowTime(elapsed) + "/" + s.GetLength() + "]";
				else
					tracklength = " [" + Song::ShowTime(elapsed) + "]";
				my_string_t playing_song = TO_WSTRING(s.toString(Config.song_status_format));
				
				const size_t max_length_without_scroll = wFooter->GetWidth()-player_state.length()-tracklength.length();
				
				wFooter->WriteXY(0, 1, player_state);
				wFooter->Bold(0);
				if (playing_song.length() > max_length_without_scroll)
				{
#					ifdef _UTF8
					playing_song += L" ** ";
#					else
					playing_song += " ** ";
#					endif
					const size_t scrollsize = max_length_without_scroll+playing_song.length();
					my_string_t part = playing_song.substr(playing_song_scroll_begin++, scrollsize);
					if (part.length() < scrollsize)
						part += playing_song.substr(0, scrollsize-part.length());
					wFooter->WriteXY(player_state.length(), 1, part);
					if (playing_song_scroll_begin >= playing_song.length())
						playing_song_scroll_begin = 0;
				}
				else
					wFooter->WriteXY(player_state.length(), 1, s.toString(Config.song_status_format), 1);
				wFooter->Bold(1);
				
				wFooter->WriteXY(wFooter->GetWidth()-tracklength.length(), 1, tracklength);
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
				wFooter->WriteXY(0, 1, "", 1);
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
		
		wHeader->Bold(1);
		wHeader->SetColor(Config.state_line_color);
		mvwhline(wHeader->Raw(), 1, 0, 0, wHeader->GetWidth());
		if (!switch_state.empty())
		{
			wHeader->WriteXY(wHeader->GetWidth()-switch_state.length()-3, 1, "[");
			wHeader->SetColor(Config.state_flags_color);
			wHeader->WriteXY(wHeader->GetWidth()-switch_state.length()-2, 1, switch_state);
			wHeader->SetColor(Config.state_line_color);
			wHeader->WriteXY(wHeader->GetWidth()-2, 1, "]");
		}
		wHeader->SetColor(Config.header_color);
		wHeader->Bold(0);
		header_update_status = 0;
	}
	if ((changed.Volume) && Config.header_visibility)
	{
		int vol = Mpd->GetVolume();
		volume_state = " Volume: " + IntoStr(vol) + "%";
		wHeader->SetColor(Config.volume_color);
		wHeader->WriteXY(wHeader->GetWidth()-volume_state.length(), 0, volume_state);
		wHeader->SetColor(Config.header_color);
	}
	wHeader->Refresh();
	if (current_screen == csPlaylist)
		mPlaylist->Refresh();
	wFooter->Bold(0);
	wFooter->GotoXY(sx, sy);
	wFooter->Refresh();
}

