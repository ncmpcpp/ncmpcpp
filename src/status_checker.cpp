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

#include "status_checker.h"
#include "helpers.h"
#include "settings.h"

extern MPDConnection *Mpd;

extern ncmpcpp_config Config;

extern Menu *mPlaylist;
extern Menu *mBrowser;
extern Menu *wCurrent;
extern Menu *mSearcher;
extern Menu *mLibArtists;
extern Menu *mLibAlbums;
extern Menu *mLibSongs;
extern Window *wHeader;
extern Window *wFooter;

extern SongList vPlaylist;
extern SongList vSearched;
extern ItemList vBrowser;

extern TagList vArtists;

extern time_t block_delay;
extern time_t timer;
extern time_t now;

extern int now_playing;
extern int playing_song_scroll_begin;

extern int block_statusbar_update_delay;

extern string browsed_dir;

extern string player_state;
extern string volume_state;
extern string switch_state;

extern string mpd_repeat;
extern string mpd_random;
extern string mpd_crossfade;
extern string mpd_db_updating;

extern NcmpcppScreen current_screen;

extern bool header_update_status;

extern bool dont_change_now_playing;
extern bool allow_statusbar_unblock;
extern bool block_progressbar_update;
extern bool block_statusbar_update;
extern bool block_playlist_update;
extern bool block_library_update;

extern bool redraw_me;

long long playlist_old_id = -1;

int old_playing;

string playlist_stats;

void TraceMpdStatus()
{
	Mpd->UpdateStatus();
	now = time(NULL);
	
	if (now == timer+Config.playlist_disable_highlight_delay && current_screen == csPlaylist)
		mPlaylist->Highlighting(!Config.playlist_disable_highlight_delay);

	if (block_statusbar_update_delay > 0)
	{
		if (now >= block_delay+block_statusbar_update_delay)
			block_statusbar_update_delay = 0;
	}
	
	if (!block_statusbar_update_delay)
	{
		block_statusbar_update_delay = -1;
		
		if (Config.statusbar_visibility)
			block_statusbar_update = !allow_statusbar_unblock;
		else
			block_progressbar_update = !allow_statusbar_unblock;
		
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
		wFooter->WriteXY(0, Config.statusbar_visibility, "Password: ", 1);
		string password = wFooter->GetString("");
		Mpd->SetPassword(password);
		Mpd->SendPassword();
		Mpd->UpdateStatus();
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
		mPlaylist->BoldOption(old_playing+1, 0);
		mPlaylist->BoldOption(now_playing+1, 1);
	}
	
	if (changed.Playlist)
	{
		playlist_old_id = Mpd->GetOldPlaylistID();
		
		if (!block_playlist_update)
		{
			SongList list;
			
			int playlist_length = Mpd->GetPlaylistLength();
			
			if (playlist_length != vPlaylist.size())
			{
				if (playlist_length < vPlaylist.size())
				{
					mPlaylist->Clear(playlist_length < mPlaylist->GetHeight() && current_screen == csPlaylist);
					FreeSongList(vPlaylist);
					Mpd->GetPlaylistChanges(-1, list);
				}
				else
					Mpd->GetPlaylistChanges(playlist_old_id, list);
				
				vPlaylist.reserve(playlist_length);
				
				for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					vPlaylist.push_back(*it);
					if (now_playing != (*it)->GetPosition())
						mPlaylist->AddOption(DisplaySong(**it));
					else
						mPlaylist->AddBoldOption(DisplaySong(**it));
				}
				
				if (current_screen == csPlaylist)
				{
					if (!playlist_length || mPlaylist->MaxChoice() < mPlaylist->GetHeight())
						mPlaylist->Hide();
					mPlaylist->Refresh(1);
				}
			}
			else
			{
				int i = 1;
				
				mPlaylist->BoldOption(old_playing+1, 0);
				mPlaylist->BoldOption(now_playing+1, 1);
				
				Mpd->GetPlaylistChanges(-1, list);
				
				SongList::iterator j = list.begin();
				
				for (SongList::iterator it = vPlaylist.begin(); it != vPlaylist.end(); it++, i++)
				{
					
					(*j)->GetEmptyFields(1);
					(*it)->GetEmptyFields(1);
					if (**it != **j)
					{
						(*it)->GetEmptyFields(0);
						(*j)->GetEmptyFields(0);
						Song *s = new Song(**j);
						**it = *s;
						mPlaylist->UpdateOption(i, DisplaySong(*s));
					}
					j++;
				}
				FreeSongList(list);
			}
		}
		
		if (vPlaylist.empty())
		{
			playlist_stats.clear();
			mPlaylist->Reset();
			ShowMessage("Cleared playlist!");
		}
		else
			playlist_stats = "(" + IntoStr(vPlaylist.size()) + (vPlaylist.size() == 1 ? " song" : " songs") + ", length: " + TotalPlaylistLength() + ")";
		
		if (current_screen == csBrowser)
		{
			bool bold = 0;
			for (int i = 0; i < vBrowser.size(); i++)
			{
				if (vBrowser[i].type == itSong)
				{
					for (SongList::const_iterator it = vPlaylist.begin(); it != vPlaylist.end(); it++)
					{
						if ((*it)->GetHash() == vBrowser[i].song->GetHash())
						{
							bold = 1;
							break;
						}
					}
					mBrowser->BoldOption(i+1, bold);
					bold = 0;
				}
			}
		}
		if (current_screen == csSearcher)
		{
			bool bold = 0;
			int i = search_engine_static_option;
			for (SongList::const_iterator it = vSearched.begin(); it != vSearched.end(); it++, i++)
			{
				for (SongList::const_iterator j = vPlaylist.begin(); j != vPlaylist.end(); j++)
				{
					if ((*j)->GetHash() == (*it)->GetHash())
					{
						bold = 1;
						break;
					}
				}
				mSearcher->BoldOption(i+1, bold);
				bold = 0;
			}
		}
		block_library_update = 0;
	}
	if (changed.Database)
	{
		GetDirectory(browsed_dir);
		if (!mLibArtists->Empty())
		{
			ShowMessage("Updating artists' list...");
			mLibArtists->Clear(0);
			vArtists.clear();
			Mpd->GetArtists(vArtists);
			sort(vArtists.begin(), vArtists.end(), CaseInsensitiveComparison);
			for (TagList::const_iterator it = vArtists.begin(); it != vArtists.end(); it++)
				mLibArtists->AddOption(*it);
			if (current_screen == csLibrary)
			{
				mLibArtists->Hide();
				mLibArtists->Display();
			}
			ShowMessage("List updated!");
		}
		block_library_update = 0;
	}
	if (changed.PlayerState)
	{
		PlayerState mpd_state = Mpd->GetState();
		switch (mpd_state)
		{
			case psPlay:
			{
				player_state = "Playing: ";
				mPlaylist->BoldOption(now_playing+1, 1);
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
				mPlaylist->BoldOption(old_playing+1, 0);
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
		if (!vPlaylist.empty() && now_playing >= 0)
		{
			if (!mPlaylist->Empty())
			{
				if (old_playing >= 0)
					mPlaylist->BoldOption(old_playing+1, 0);
				mPlaylist->BoldOption(now_playing+1, 1);
				if (Config.autocenter_mode)
					mPlaylist->Highlight(now_playing+1);
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
			WindowTitle(DisplaySong(s, Config.song_window_title_format));
			
			int elapsed = Mpd->GetElapsedTime();
			
			if (!block_statusbar_update && Config.statusbar_visibility)
			{
				string tracklength;
				if (s.GetTotalLength() > 0)
					tracklength = " [" + ShowTime(elapsed) + "/" + s.GetLength() + "]";
				else
					tracklength = " [" + ShowTime(elapsed) + "]";
				ncmpcpp_string_t playing_song = NCMPCPP_TO_WSTRING(OmitBBCodes(DisplaySong(s, Config.song_status_format)));
				
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
					ncmpcpp_string_t part = playing_song.substr(playing_song_scroll_begin++, scrollsize);
					if (part.length() < scrollsize)
						part += playing_song.substr(0, scrollsize-part.length());
					wFooter->WriteXY(player_state.length(), 1, part);
					if (playing_song_scroll_begin >= playing_song.length())
						playing_song_scroll_begin = 0;
				}
				else
					wFooter->WriteXY(player_state.length(), 1, OmitBBCodes(DisplaySong(s, Config.song_status_format)), 1);
				wFooter->Bold(1);
				
				wFooter->WriteXY(wFooter->GetWidth()-tracklength.length(), 1, tracklength);
			}
			if (!block_progressbar_update)
			{
				double progressbar_size = (double)elapsed/(s.GetMinutesLength()*60+s.GetSecondsLength());
				int howlong = wFooter->GetWidth()*progressbar_size;
				wFooter->SetColor(Config.progressbar_color);
				mvwhline(wFooter->RawWin(), 0, 0, 0, wFooter->GetWidth());
				mvwhline(wFooter->RawWin(), 0, 0, '=',howlong);
				mvwaddch(wFooter->RawWin(), 0, howlong, '>');
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
	wCurrent->Refresh();
	wFooter->Bold(0);
	wFooter->GotoXY(sx, sy);
	wFooter->Refresh();
	wFooter->AutoRefresh(1);
	wFooter->EnableBB();
}

