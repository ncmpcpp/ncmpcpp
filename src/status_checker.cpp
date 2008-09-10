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

#include "helpers.h"
#include "search_engine.h"
#include "settings.h"
#include "status_checker.h"

extern MPDConnection *Mpd;
extern ncmpcpp_config Config;

extern Menu<Song> *mPlaylist;
extern Menu<Item> *mBrowser;
extern Menu<string> *mLibArtists;
extern Menu<Song> *mLibSongs;
extern Menu<Song> *mPlaylistEditor;

#ifdef HAVE_TAGLIB_H
extern Menu<string> *mEditorAlbums;
#endif // HAVE_TAGLIB_H

extern Window *wHeader;
extern Window *wFooter;

extern SongList vSearched;

extern time_t timer;

extern int now_playing;
extern int playing_song_scroll_begin;

extern int lock_statusbar_delay;

extern string browsed_dir;

extern string player_state;
extern string volume_state;
extern string switch_state;

extern string mpd_repeat;
extern string mpd_random;
extern string mpd_crossfade;
extern string mpd_db_updating;

extern NcmpcppScreen current_screen;

extern bool dont_change_now_playing;
extern bool allow_statusbar_unlock;
extern bool block_progressbar_update;
extern bool block_statusbar_update;
extern bool block_playlist_update;
extern bool block_found_item_list_update;

extern bool redraw_screen;

bool header_update_status = 0;
bool repeat_one_allowed = 0;

long long playlist_old_id = -1;

int old_playing;

time_t time_of_statusbar_lock;
time_t now;

string playlist_stats;
string volume_state;
string switch_state;
string player_state;

string mpd_repeat;
string mpd_random;
string mpd_crossfade;
string mpd_db_updating;

void TraceMpdStatus()
{
	Mpd->UpdateStatus();
	now = time(NULL);
	
	if (now == timer+Config.playlist_disable_highlight_delay && current_screen == csPlaylist)
		mPlaylist->Highlighting(!Config.playlist_disable_highlight_delay);

	if (lock_statusbar_delay > 0)
	{
		if (now >= time_of_statusbar_lock+lock_statusbar_delay)
			lock_statusbar_delay = 0;
	}
	
	if (!lock_statusbar_delay)
	{
		lock_statusbar_delay = -1;
		
		if (Config.statusbar_visibility)
			block_statusbar_update = !allow_statusbar_unlock;
		else
			block_progressbar_update = !allow_statusbar_unlock;
		
		MPDStatusChanges changes;
		switch (Mpd->GetState())
		{
			case psStop:
				changes.PlayerState = 1;
				break;
			case psPlay: case psPause:
				changes.ElapsedTime = 1; // restore status
				break;
		}
		NcmpcppStatusChanged(Mpd, changes, NULL);
	}
	//wHeader->WriteXY(0,1, IntoStr(now_playing), 1);
}

void NcmpcppErrorCallback(MPDConnection *Mpd, int errorid, string msg, void *data)
{
	if (errorid == MPD_ACK_ERROR_PERMISSION)
	{
		wFooter->SetGetStringHelper(NULL);
		wFooter->WriteXY(0, Config.statusbar_visibility, "Password: ", 1);
		string password = wFooter->GetString();
		Mpd->SetPassword(password);
		Mpd->SendPassword();
		Mpd->UpdateStatus();
		wFooter->SetGetStringHelper(TraceMpdStatus);
	}
	else
		ShowMessage(msg);
}

void NcmpcppStatusChanged(MPDConnection *Mpd, MPDStatusChanges changed, void *data)
{
	int sx, sy;
	wFooter->DisableBB();
	wFooter->AutoRefresh(0);
	wFooter->Bold(1);
	wFooter->GetXY(sx, sy);
	
	if ((now_playing != Mpd->GetCurrentSongPos() || changed.SongID) && !dont_change_now_playing)
	{
		old_playing = now_playing;
		now_playing = Mpd->GetCurrentSongPos();
		mPlaylist->BoldOption(old_playing, 0);
		mPlaylist->BoldOption(now_playing, 1);
	}
	
	if (changed.Playlist)
	{
		playlist_old_id = Mpd->GetOldPlaylistID();
		
		if (!block_playlist_update)
		{
			SongList list;
			int playlist_length = Mpd->GetPlaylistLength();
			if (playlist_length != mPlaylist->Size())
			{
				if (playlist_length < mPlaylist->Size())
				{
					mPlaylist->Clear(playlist_length < mPlaylist->GetHeight() && current_screen == csPlaylist);
					Mpd->GetPlaylistChanges(-1, list);
				}
				else
					Mpd->GetPlaylistChanges(playlist_old_id, list);
				
				for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					if (now_playing != (*it)->GetPosition())
						mPlaylist->AddOption(**it);
					else
						mPlaylist->AddBoldOption(**it);
				}
				
				if (current_screen == csPlaylist)
				{
					if (!playlist_length || mPlaylist->Size() < mPlaylist->GetHeight())
						mPlaylist->Window::Clear();
					mPlaylist->Refresh(1);
				}
			}
			else
			{
			//	mPlaylist->BoldOption(old_playing+1, 0);
			//	mPlaylist->BoldOption(now_playing+1, 1);
				
				Mpd->GetPlaylistChanges(-1, list);
				
				for (int i = 0; i < mPlaylist->Size(); i++)
				{
					if (*list[i] != mPlaylist->at(i))
						mPlaylist->UpdateOption(i, *list[i]);
				}
			}
			FreeSongList(list);
		}
		
		if (mPlaylist->Empty())
		{
			playlist_stats.clear();
			mPlaylist->Reset();
			ShowMessage("Cleared playlist!");
		}
		else
			playlist_stats = "(" + IntoStr(mPlaylist->Size()) + (mPlaylist->Size() == 1 ? " item" : " items") + TotalPlaylistLength() + ")";
		
		if (current_screen == csBrowser)
		{
			UpdateItemList(mBrowser);
		}
		else if (current_screen == csSearcher && !block_found_item_list_update)
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
	if (changed.Database)
	{
		GetDirectory(browsed_dir);
#		ifdef HAVE_TAGLIB_H
		mEditorAlbums->Clear(0);
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
				mvwhline(wFooter->RawWin(), 0, 0, 0, wFooter->GetWidth());
				wFooter->SetColor(Config.statusbar_color);
				mPlaylist->BoldOption(old_playing, 0);
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
			if (!mPlaylist->Empty())
			{
				if (Config.repeat_one_mode && repeat_one_allowed && (old_playing+1 == now_playing || !now_playing))
				{
					std::swap<int>(now_playing,old_playing);
					Mpd->Play(now_playing);
				}
				if (old_playing >= 0)
					mPlaylist->BoldOption(old_playing, 0);
				mPlaylist->BoldOption(now_playing, 1);
				if (Config.autocenter_mode)
					mPlaylist->Highlight(now_playing);
				repeat_one_allowed = 0;
			}
			if (!Mpd->GetElapsedTime())
				mvwhline(wFooter->RawWin(), 0, 0, 0, wFooter->GetWidth());
		}
		playing_song_scroll_begin = 0;
		
		if (Mpd->GetState() == psPlay)
			changed.ElapsedTime = 1;
	}
	if (changed.ElapsedTime)
	{
		Song s = Mpd->GetCurrentSong();
		if (!player_state.empty() && !s.Empty())
		{
			WindowTitle(DisplaySong(s, &Config.song_window_title_format));
			
			int elapsed = Mpd->GetElapsedTime();
			
			// 'repeat one' mode check - be sure that we deal with item with known length
			if (Mpd->GetCurrentSong().GetTotalLength() && elapsed == Mpd->GetCurrentSong().GetTotalLength()-1)
				repeat_one_allowed = 1;
			
			if (!block_statusbar_update && Config.statusbar_visibility)
			{
				string tracklength;
				if (s.GetTotalLength())
					tracklength = " [" + ShowTime(elapsed) + "/" + s.GetLength() + "]";
				else
					tracklength = " [" + ShowTime(elapsed) + "]";
				my_string_t playing_song = TO_WSTRING(DisplaySong(s, &Config.song_status_format));
				
				int max_length_without_scroll = wFooter->GetWidth()-player_state.length()-tracklength.length();
				
				wFooter->WriteXY(0, 1, player_state);
				wFooter->Bold(0);
				if (playing_song.length() > max_length_without_scroll)
				{
#					ifdef UTF8_ENABLED
					playing_song += L" ** ";
#					else
					playing_song += " ** ";
#					endif
					const int scrollsize = max_length_without_scroll+playing_song.length();
					my_string_t part = playing_song.substr(playing_song_scroll_begin++, scrollsize);
					if (part.length() < scrollsize)
						part += playing_song.substr(0, scrollsize-part.length());
					wFooter->WriteXY(player_state.length(), 1, part);
					if (playing_song_scroll_begin >= playing_song.length())
						playing_song_scroll_begin = 0;
				}
				else
					wFooter->WriteXY(player_state.length(), 1, DisplaySong(s, &Config.song_status_format), 1);
				wFooter->Bold(1);
				
				wFooter->WriteXY(wFooter->GetWidth()-tracklength.length(), 1, tracklength);
			}
			if (!block_progressbar_update)
			{
				double progressbar_size = (double)elapsed/(s.GetTotalLength());
				int howlong = wFooter->GetWidth()*progressbar_size;
				wFooter->SetColor(Config.progressbar_color);
				mvwhline(wFooter->RawWin(), 0, 0, 0, wFooter->GetWidth());
				if (s.GetTotalLength())
				{
					mvwhline(wFooter->RawWin(), 0, 0, '=',howlong);
					mvwaddch(wFooter->RawWin(), 0, howlong, '>');
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
	if (changed.Repeat)
	{
		mpd_repeat = (Mpd->GetRepeat() ? "r" : "");
		ShowMessage("Repeat is " + (string)(mpd_repeat.empty() ? "off" : "on"));
		header_update_status = 1;

	}
	if (changed.Random)
	{
		mpd_random = Mpd->GetRandom() ? "z" : "";
		ShowMessage("Random is " + (string)(mpd_random.empty() ? "off" : "on"));
		header_update_status = 1;
	}
	if (changed.Crossfade)
	{
		int crossfade = Mpd->GetCrossfade();
		mpd_crossfade = crossfade ? "x" : "";
		ShowMessage("Crossfade set to " + IntoStr(crossfade) + " seconds");
		header_update_status = 1;
	}
	if (changed.DBUpdating)
	{
		mpd_db_updating = Mpd->GetDBIsUpdating() ? "U" : "";
		ShowMessage(mpd_db_updating.empty() ? "Database update finished!" : "Database update started!");
		header_update_status = 1;
	}
	if (header_update_status && Config.header_visibility)
	{
		switch_state = mpd_repeat + mpd_random + mpd_crossfade + mpd_db_updating;
		
		wHeader->DisableBB();
		wHeader->Bold(1);
		wHeader->SetColor(Config.state_line_color);
		mvwhline(wHeader->RawWin(), 1, 0, 0, wHeader->GetWidth());
		if (!switch_state.empty())
		{
			wHeader->WriteXY(wHeader->GetWidth()-switch_state.length()-3, 1, "[");
			wHeader->SetColor(Config.state_flags_color);
			wHeader->WriteXY(wHeader->GetWidth()-switch_state.length()-2, 1, switch_state);
			wHeader->SetColor(Config.state_line_color);
			wHeader->WriteXY(wHeader->GetWidth()-2, 1, "]");
		}
		wHeader->Refresh();
		wHeader->SetColor(Config.header_color);
		wHeader->Bold(0);
		wHeader->EnableBB();
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
	if (current_screen == csPlaylist)
		mPlaylist->Refresh();
	wFooter->Bold(0);
	wFooter->GotoXY(sx, sy);
	wFooter->Refresh();
	wFooter->AutoRefresh(1);
	wFooter->EnableBB();
}

