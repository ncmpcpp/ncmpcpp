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

#include <algorithm>

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "menu.h"
#include "playlist.h"
#include "song.h"
#include "status.h"

using Global::MainHeight;
using Global::MainStartY;

Playlist *myPlaylist = new Playlist;

bool Playlist::ReloadTotalLength = 0;
bool Playlist::ReloadRemaining = 0;

const size_t Playlist::SortOptions = 10;
const size_t Playlist::SortDialogWidth = 30;
size_t Playlist::SortDialogHeight;

Menu< std::pair<std::string, MPD::Song::GetFunction> > *Playlist::SortDialog = 0;

void Playlist::Init()
{
	static Display::ScreenFormat sf = { this, &Config.song_list_format };
	
	Items = new Menu<MPD::Song>(0, MainStartY, COLS, MainHeight, Config.columns_in_playlist && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, brNone);
	Items->CyclicScrolling(Config.use_cyclic_scrolling);
	Items->CenteredCursor(Config.centered_cursor);
	Items->HighlightColor(Config.main_highlight_color);
	Items->SetSelectPrefix(&Config.selected_item_prefix);
	Items->SetSelectSuffix(&Config.selected_item_suffix);
	Items->SetItemDisplayer(Config.columns_in_playlist ? Display::SongsInColumns : Display::Songs);
	Items->SetItemDisplayerUserData(&sf);
	Items->SetGetStringFunction(Config.columns_in_playlist ? SongInColumnsToString : SongToString);
	Items->SetGetStringFunctionUserData(&Config.song_list_format_dollar_free);
	
	if (!SortDialog)
	{
		SortDialogHeight = std::min(int(MainHeight), 17);
		
		SortDialog = new Menu< std::pair<std::string, MPD::Song::GetFunction> >((COLS-SortDialogWidth)/2, (MainHeight-SortDialogHeight)/2+MainStartY, SortDialogWidth, SortDialogHeight, "Sort songs by...", Config.main_color, Config.window_border);
		SortDialog->CyclicScrolling(Config.use_cyclic_scrolling);
		SortDialog->CenteredCursor(Config.centered_cursor);
		SortDialog->SetItemDisplayer(Display::Pairs);
		
		SortDialog->AddOption(std::make_pair("Artist", &MPD::Song::GetArtist));
		SortDialog->AddOption(std::make_pair("Album", &MPD::Song::GetAlbum));
		SortDialog->AddOption(std::make_pair("Disc", &MPD::Song::GetDisc));
		SortDialog->AddOption(std::make_pair("Track", &MPD::Song::GetTrack));
		SortDialog->AddOption(std::make_pair("Genre", &MPD::Song::GetGenre));
		SortDialog->AddOption(std::make_pair("Date", &MPD::Song::GetDate));
		SortDialog->AddOption(std::make_pair("Composer", &MPD::Song::GetComposer));
		SortDialog->AddOption(std::make_pair("Performer", &MPD::Song::GetPerformer));
		SortDialog->AddOption(std::make_pair("Title", &MPD::Song::GetTitle));
		SortDialog->AddOption(std::make_pair("Filename", &MPD::Song::GetFile));
		SortDialog->AddSeparator();
		SortDialog->AddOption(std::make_pair("Sort", static_cast<MPD::Song::GetFunction>(0)));
		SortDialog->AddOption(std::make_pair("Cancel", static_cast<MPD::Song::GetFunction>(0)));
	}
	
	w = Items;
	isInitialized = 1;
}

void Playlist::SwitchTo()
{
	using Global::myScreen;
	using Global::myLockedScreen;
	using Global::myInactiveScreen;
	
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	itsScrollBegin = 0;
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	Items->Window::Clear();
	EnableHighlighting();
	if (w != Items) // even if sorting window is active, background has to be refreshed anyway
		Items->Display();
	Global::RedrawHeader = 1;
}

void Playlist::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	Items->Resize(width, MainHeight);
	Items->MoveTo(x_offset, MainStartY);

	Items->SetTitle(Config.columns_in_playlist && Config.titles_visibility ? Display::Columns(Items->GetWidth()) : "");
	if (w == SortDialog) // if sorting window is active, playlist needs refreshing
		Items->Display();
	
	SortDialogHeight = std::min(int(MainHeight), 17);
	if (Items->GetWidth() >= SortDialogWidth && MainHeight >= 5)
	{
		SortDialog->Resize(SortDialogWidth, SortDialogHeight);
		SortDialog->MoveTo(x_offset+(width-SortDialogWidth)/2, (MainHeight-SortDialogHeight)/2+MainStartY);
	}
	else // if screen is too low to display sorting window, fall back to items list
		w = Items;
	
	hasToBeResized = 0;
}

std::basic_string<my_char_t> Playlist::Title()
{
	std::basic_string<my_char_t> result = U("Playlist ");
	if (ReloadTotalLength || ReloadRemaining)
		itsBufferedStats = TotalLength();
	result += Scroller(TO_WSTRING(itsBufferedStats), itsScrollBegin, COLS-result.length()-(Config.new_design ? 2 : Global::VolumeState.length()));
	return result;
}

void Playlist::EnterPressed()
{
	if (w == Items)
	{
		if (!Items->Empty())
			Mpd.PlayID(Items->Current().GetID());
	}
	else if (w == SortDialog)
	{
		size_t pos = SortDialog->Choice();
		
		size_t beginning = 0;
		size_t end = Items->Size();
		if (Items->hasSelected())
		{
			std::vector<size_t> list;
			Items->GetSelected(list);
			beginning = *list.begin();
			end = *list.rbegin()+1;
		}
		
		if (pos > SortOptions)
		{
			if (pos == SortOptions+2) // cancel
			{
				w = Items;
				return;
			}
		}
		else
		{
			ShowMessage("Move tag types up and down to adjust sort order");
			return;
		}
		
		MPD::SongList playlist;
		playlist.reserve(end-beginning);
		for (size_t i = beginning; i < end; ++i)
			playlist.push_back(&(*Items)[i]);
		
		ShowMessage("Sorting...");
		Mpd.StartCommandsList();
		QuickSort(playlist.begin(), playlist.end(), playlist.begin());
		if (Mpd.CommitCommandsList())
			ShowMessage("Playlist sorted");
		w = Items;
	}
}

void Playlist::SpacePressed()
{
	if (w == Items && !Items->Empty())
	{
		Items->Select(Items->Choice(), !Items->isSelected());
		Items->Scroll(wDown);
	}
}

void Playlist::ReadKey(int &key)
{
	w->ReadKey(key);
	UpdateTimer();
}

void Playlist::MouseButtonPressed(MEVENT me)
{
	if (w == Items && !Items->Empty() && Items->hasCoords(me.x, me.y))
	{
		if (size_t(me.y) < Items->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Items->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				EnterPressed();
		}
		else
			Screen<Window>::MouseButtonPressed(me);
	}
	else if (w == SortDialog && SortDialog->hasCoords(me.x, me.y))
	{
		if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
		{
			SortDialog->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				EnterPressed();
		}
		else
			Screen<Window>::MouseButtonPressed(me);
	}
}

MPD::Song *Playlist::CurrentSong()
{
	return w == Items && !Items->Empty() ? &Items->Current() : 0;
}

void Playlist::GetSelectedSongs(MPD::SongList &v)
{
	if (Items->Empty())
		return;
	std::vector<size_t> selected;
	Items->GetSelected(selected);
	if (selected.empty())
		selected.push_back(Items->Choice());
	for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
		v.push_back(new MPD::Song(Items->at(*it)));
}

void Playlist::ApplyFilter(const std::string &s)
{
	if (w == Items)
		Items->ApplyFilter(s, 0, REG_ICASE | Config.regex_type);
}

bool Playlist::isFiltered()
{
	if (Items->isFiltered())
	{
		ShowMessage("Function currently unavailable due to filtered playlist");
		return true;
	}
	return false;
}

void Playlist::MoveSelectedItems(Movement where)
{
	if (Items->Empty() || isFiltered())
		return;

	// remove search results as we may move them to different positions, but
	// search rememebers positions and may point to wrong ones after that.
	Items->Search("");
	
	switch (where)
	{
		case mUp:
		{
			if (myPlaylist->Items->hasSelected())
			{
				std::vector<size_t> list;
				myPlaylist->Items->GetSelected(list);
				if (list.front() > 0)
				{
					Mpd.StartCommandsList();
					std::vector<size_t>::const_iterator it = list.begin();
					for (; it != list.end(); ++it)
						Mpd.Move(*it-1, *it);
					if (Mpd.CommitCommandsList())
					{
						myPlaylist->Items->Select(list.back(), false);
						myPlaylist->Items->Select(list.front()-1, true);
						myPlaylist->Items->Highlight(list[(list.size()-1)/2]-1);
					}
				}
			}
			else
			{
				size_t pos = myPlaylist->Items->Choice();
				if (pos > 0)
				{
					if (Mpd.Move(pos-1, pos))
						myPlaylist->Items->Scroll(wUp);
				}
			}
			break;
		}
		case mDown:
		{
			if (Items->hasSelected())
			{
				std::vector<size_t> list;
				Items->GetSelected(list);
					
				if (list.back() < Items->Size()-1)
				{
					Mpd.StartCommandsList();
					std::vector<size_t>::const_reverse_iterator it = list.rbegin();
					for (; it != list.rend(); ++it)
						Mpd.Move(*it, *it+1);
					if (Mpd.CommitCommandsList())
					{
						Items->Select(list.front(), false);
						Items->Select(list.back()+1, true);
						Items->Highlight(list[(list.size()-1)/2]+1);
					}
				}
			}
			else
			{
				size_t pos = Items->Choice();
				if (pos < Items->Size()-1)
				{
					if (Mpd.Move(pos, pos+1))
						Items->Scroll(wDown);
				}
			}
			break;
		}
	}
}

void Playlist::Sort()
{
	if (isFiltered())
		return;
	if (Items->GetWidth() < SortDialogWidth || MainHeight < 5)
		ShowMessage("Screen is too small to display dialog window");
	else
	{
		SortDialog->Reset();
		w = SortDialog;
	}
}

void Playlist::Reverse()
{
	if (isFiltered())
		return;
	ShowMessage("Reversing playlist order...");
	size_t beginning = -1, end = -1;
	for (size_t i = 0; i < Items->Size(); ++i)
	{
		if (Items->isSelected(i))
		{
			if (beginning == size_t(-1))
				beginning = i;
			end = i;
		}
	}
	if (beginning == size_t(-1)) // no selected items
	{
		beginning = 0;
		end = Items->Size();
	}
	Mpd.StartCommandsList();
	for (size_t i = beginning, j = end-1; i < (beginning+end)/2; ++i, --j)
		Mpd.Swap(i, j);
	if (Mpd.CommitCommandsList())
		 ShowMessage("Playlist reversed");
}

void Playlist::AdjustSortOrder(Movement where)
{
	switch (where)
	{
		case mUp:
		{
			size_t pos = SortDialog->Choice();
			if (pos > 0 && pos < SortOptions)
			{
				SortDialog->Swap(pos, pos-1);
				SortDialog->Scroll(wUp);
			}
			break;
		}
		case mDown:
		{
			size_t pos = SortDialog->Choice();
			if (pos < SortOptions-1)
			{
				SortDialog->Swap(pos, pos+1);
				SortDialog->Scroll(wDown);
			}
			break;
		}
	}
}

void Playlist::EnableHighlighting()
{
	Items->Highlighting(1);
	UpdateTimer();
}

void Playlist::QuickSort(MPD::SongList::iterator first, MPD::SongList::iterator last, MPD::SongList::iterator begin)
{
	if (last-first > 1)
	{
		MPD::SongList::iterator pivot = first+rand()%(last-first);
		IterSwap(pivot, last-1, begin);
		pivot = last-1;
		
		MPD::SongList::iterator tmp = first;
		for (MPD::SongList::iterator i = first; i != pivot; ++i)
			if (SongComp(*i, *pivot))
				IterSwap(i, tmp++, begin);
		IterSwap(tmp, pivot, begin);
		
		QuickSort(first, tmp, begin);
		QuickSort(tmp+1, last, begin);
	}
}

std::string Playlist::TotalLength()
{
	std::ostringstream result;
	
	if (ReloadTotalLength)
	{
		itsTotalLength = 0;
		for (size_t i = 0; i < Items->Size(); ++i)
			itsTotalLength += (*Items)[i].GetTotalLength();
		ReloadTotalLength = 0;
	}
	if (Config.playlist_show_remaining_time && ReloadRemaining && !Items->isFiltered())
	{
		itsRemainingTime = 0;
		for (size_t i = NowPlaying; i < Items->Size(); ++i)
			itsRemainingTime += (*Items)[i].GetTotalLength();
		ReloadRemaining = 0;
	}
	
	result << '(' << Items->Size() << (Items->Size() == 1 ? " item" : " items");
	
	if (Items->isFiltered())
	{
		Items->ShowAll();
		size_t real_size = Items->Size();
		Items->ShowFiltered();
		if (Items->Size() != real_size)
			result << " (out of " << Mpd.GetPlaylistLength() << ")";
	}
	
	if (itsTotalLength)
	{
		result << ", length: ";
		ShowTime(result, itsTotalLength, Config.playlist_shorten_total_times);
	}
	if (Config.playlist_show_remaining_time && itsRemainingTime && !Items->isFiltered() && Items->Size() > 1)
	{
		result << " :: remaining: ";
		ShowTime(result, itsRemainingTime, Config.playlist_shorten_total_times);
	}
	result << ')';
	return result.str();
}

const MPD::Song *Playlist::NowPlayingSong()
{
	bool was_filtered = Items->isFiltered();
	Items->ShowAll();
	const MPD::Song *s = isPlaying() ? &Items->at(NowPlaying) : 0;
	if (was_filtered)
		Items->ShowFiltered();
	return s;
}

std::string Playlist::SongToString(const MPD::Song &s, void *data)
{
	return s.toString(*static_cast<std::string *>(data));
}

std::string Playlist::SongInColumnsToString(const MPD::Song &s, void *)
{
	return s.toString(Config.song_in_columns_to_string_format);
}

bool Playlist::Add(const MPD::Song &s, bool in_playlist, bool play, int position)
{
	if (Config.ncmpc_like_songs_adding && in_playlist)
	{
		unsigned hash = s.GetHash();
		if (play)
		{
			for (size_t i = 0; i < Items->Size(); ++i)
			{
				if (Items->at(i).GetHash() == hash)
				{
					Mpd.Play(i);
					break;
				}
			}
			return true;
		}
		else
		{
			Mpd.StartCommandsList();
			for (size_t i = 0; i < Items->Size(); ++i)
			{
				if ((*Items)[i].GetHash() == hash)
				{
					Mpd.Delete(i);
					Items->DeleteOption(i);
					i--;
				}
			}
			Mpd.CommitCommandsList();
			return false;
		}
	}
	else
	{
		int id = Mpd.AddSong(s, position);
		if (id >= 0)
		{
			ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format_no_colors).c_str());
			if (play)
				Mpd.PlayID(id);
			return true;
		}
		else
			return false;
	}
}

bool Playlist::Add(const MPD::SongList &l, bool play, int position)
{
	if (l.empty())
		return false;
	
	Mpd.StartCommandsList();
	MPD::SongList::const_iterator it = l.begin();
	if (position < 0)
	{
		for (; it != l.end(); ++it)
			if (Mpd.AddSong(**it) < 0)
				break;
	}
	else
	{
		MPD::SongList::const_reverse_iterator j = l.rbegin();
		for (; j != l.rend(); ++j)
			if (Mpd.AddSong(**j, position) < 0)
				break;
	}
	if (!Mpd.CommitCommandsList())
		return false;
	if (play)
		PlayNewlyAddedSongs();
	return true;
}

void Playlist::PlayNewlyAddedSongs()
{
	bool is_filtered = Items->isFiltered();
	Items->ShowAll();
	size_t old_size = Items->Size();
	Mpd.UpdateStatus();
	if (old_size < Items->Size())
		Mpd.Play(old_size);
	if (is_filtered)
		Items->ShowFiltered();
}

void Playlist::SetSelectedItemsPriority(int prio)
{
	std::vector<size_t> list;
	myPlaylist->Items->GetSelected(list);
	if (list.empty())
		list.push_back(Items->Choice());
	Mpd.StartCommandsList();
	for (std::vector<size_t>::const_iterator it = list.begin(); it != list.end(); ++it)
		Mpd.SetPriority((*Items)[*it], prio);
	if (Mpd.CommitCommandsList())
		ShowMessage("Priority set");
}
