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

#include <algorithm>

#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "mpdpp.h"
#include "status_checker.h"

using namespace Global;
using namespace MPD;
using std::string;

Window *Global::wPlaylistEditorActiveCol;
Menu<string> *Global::mPlaylistList;
Menu<Song> *Global::mPlaylistEditor;

namespace PlaylistEditor
{
	size_t left_col_width;
	size_t right_col_startx;
	size_t right_col_width;
}

void PlaylistEditor::Init()
{
	left_col_width = COLS/3-1;
	right_col_startx = left_col_width+1;
	right_col_width = COLS-left_col_width-1;
	
	mPlaylistList = new Menu<string>(0, main_start_y, left_col_width, main_height, "Playlists", Config.main_color, brNone);
	mPlaylistList->HighlightColor(Config.active_column_color);
	mPlaylistList->SetTimeout(ncmpcpp_window_timeout);
	mPlaylistList->SetItemDisplayer(Display::Generic);
	
	mPlaylistEditor = new Menu<Song>(right_col_startx, main_start_y, right_col_width, main_height, "Playlist's content", Config.main_color, brNone);
	mPlaylistEditor->HighlightColor(Config.main_highlight_color);
	mPlaylistEditor->SetTimeout(ncmpcpp_window_timeout);
	mPlaylistEditor->SetSelectPrefix(&Config.selected_item_prefix);
	mPlaylistEditor->SetSelectSuffix(&Config.selected_item_suffix);
	mPlaylistEditor->SetItemDisplayer(Display::Songs);
	mPlaylistEditor->SetItemDisplayerUserData(&Config.song_list_format);
	
	wPlaylistEditorActiveCol = mPlaylistList;
}

void PlaylistEditor::Resize()
{
	left_col_width = COLS/3-1;
	right_col_startx = left_col_width+1;
	right_col_width = COLS-left_col_width-1;
	
	mPlaylistList->Resize(left_col_width, main_height);
	mPlaylistEditor->Resize(right_col_width, main_height);
	
	mPlaylistEditor->MoveTo(right_col_startx, main_start_y);
}

void PlaylistEditor::Refresh()
{
	mPlaylistList->Display();
	mvvline(main_start_y, right_col_startx-1, 0, main_height);
	mPlaylistEditor->Display();
}

void PlaylistEditor::SwitchTo()
{
	if (current_screen != csPlaylistEditor
#	ifdef HAVE_TAGLIB_H
	&&  current_screen != csTinyTagEditor
#	endif // HAVE_TAGLIB_H
	   )
	{
		CLEAR_FIND_HISTORY;
		
		myPlaylist->Main()->Hide(); // hack, should be wCurrent, but it doesn't always have 100% width
		
//		redraw_screen = 1;
		redraw_header = 1;
		PlaylistEditor::Refresh();
		
		wCurrent = wPlaylistEditorActiveCol;
		current_screen = csPlaylistEditor;
		
		UpdateSongList(mPlaylistEditor);
	}
}

void PlaylistEditor::Update()
{
	if (mPlaylistList->Empty())
	{
		mPlaylistEditor->Clear(0);
		TagList list;
		Mpd->GetPlaylists(list);
		sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (TagList::iterator it = list.begin(); it != list.end(); it++)
		{
			utf_to_locale(*it);
			mPlaylistList->AddOption(*it);
		}
		mPlaylistList->Window::Clear();
		mPlaylistList->Refresh();
	}
	
	if (!mPlaylistList->Empty() && mPlaylistEditor->Empty())
	{
		mPlaylistEditor->Reset();
		SongList list;
		Mpd->GetPlaylistContent(locale_to_utf_cpy(mPlaylistList->Current()), list);
		if (!list.empty())
			mPlaylistEditor->SetTitle("Playlist's content (" + IntoStr(list.size()) + " item" + (list.size() == 1 ? ")" : "s)"));
		else
			mPlaylistEditor->SetTitle("Playlist's content");
		bool bold = 0;
		for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
		{
			for (size_t j = 0; j < myPlaylist->Main()->Size(); j++)
			{
				if ((*it)->GetHash() == myPlaylist->Main()->at(j).GetHash())
				{
					bold = 1;
					break;
				}
			}
			mPlaylistEditor->AddOption(**it, bold);
			bold = 0;
		}
		FreeSongList(list);
		mPlaylistEditor->Window::Clear();
		mPlaylistEditor->Display();
	}
	
	if (wCurrent == mPlaylistEditor && mPlaylistEditor->Empty())
	{
		mPlaylistEditor->HighlightColor(Config.main_highlight_color);
		mPlaylistList->HighlightColor(Config.active_column_color);
		wCurrent = wPlaylistEditorActiveCol = mPlaylistList;
	}
	
	if (mPlaylistEditor->Empty())
	{
		mPlaylistEditor->WriteXY(0, 0, 0, "Playlist is empty.");
		mPlaylistEditor->Refresh();
	}
}

void PlaylistEditor::EnterPressed(bool add_n_play)
{
	SongList list;
	
	if (wCurrent == mPlaylistList && !mPlaylistList->Empty())
	{
		Mpd->GetPlaylistContent(locale_to_utf_cpy(mPlaylistList->Current()), list);
		for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
			Mpd->QueueAddSong(**it);
		if (Mpd->CommitQueue())
		{
			ShowMessage("Loading playlist %s...", mPlaylistList->Current().c_str());
			Song &s = myPlaylist->Main()->at(myPlaylist->Main()->Size()-list.size());
			if (s.GetHash() == list[0]->GetHash())
			{
				if (add_n_play)
					Mpd->PlayID(s.GetID());
			}
			else
				ShowMessage("%s", message_part_of_songs_added);
		}
	}
	else if (wCurrent == mPlaylistEditor)
	{
		if (!mPlaylistEditor->Empty())
		{
			block_item_list_update = 1;
			if (Config.ncmpc_like_songs_adding && mPlaylistEditor->isBold())
			{
				long long hash = mPlaylistEditor->Current().GetHash();
				if (add_n_play)
				{
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd->Play(i);
							break;
						}
					}
				}
				else
				{
					block_playlist_update = 1;
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd->QueueDeleteSong(i);
							myPlaylist->Main()->DeleteOption(i);
							i--;
						}
					}
					Mpd->CommitQueue();
					mPlaylistEditor->BoldOption(mPlaylistEditor->Choice(), 0);
				}
			}
			else
			{
				Song &s = mPlaylistEditor->at(mPlaylistEditor->Choice());
				int id = Mpd->AddSong(s);
				if (id >= 0)
				{
					ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
					if (add_n_play)
						Mpd->PlayID(id);
					mPlaylistEditor->BoldOption(mPlaylistEditor->Choice(), 1);
				}
			}
		}
	}
	FreeSongList(list);
	if (!add_n_play)
		wCurrent->Scroll(wDown);
}

