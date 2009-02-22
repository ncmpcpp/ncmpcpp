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

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "menu.h"
#include "playlist.h"
#include "song.h"
#include "status.h"
#include "tag_editor.h"

using namespace Global;
using std::vector;

Playlist *myPlaylist = new Playlist;

bool Playlist::ReloadTotalLength = 0;
bool Playlist::ReloadRemaining = 0;

bool Playlist::BlockNowPlayingUpdate = 0;
bool Playlist::BlockUpdate = 0;
bool Playlist::BlockRefreshing = 0;

Menu< std::pair<std::string, MPD::Song::GetFunction> > *Playlist::SortDialog;

const size_t Playlist::SortOptions = 10;

const size_t Playlist::SortDialogWidth = 30;
const size_t Playlist::SortDialogHeight = 17;

void Playlist::Init()
{
	w = new Menu<MPD::Song>(0, main_start_y, COLS, main_height, Config.columns_in_playlist ? Display::Columns(Config.song_columns_list_format) : "", Config.main_color, brNone);
	w->SetTimeout(ncmpcpp_window_timeout);
	w->HighlightColor(Config.main_highlight_color);
	w->SetSelectPrefix(&Config.selected_item_prefix);
	w->SetSelectSuffix(&Config.selected_item_suffix);
	w->SetItemDisplayer(Config.columns_in_playlist ? Display::SongsInColumns : Display::Songs);
	w->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
	w->SetGetStringFunction(Config.columns_in_playlist ? SongInColumnsToString : SongToString);
	w->SetGetStringFunctionUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
	
	SortDialog = new Menu< std::pair<std::string, MPD::Song::GetFunction> >((COLS-SortDialogWidth)/2, (LINES-SortDialogHeight)/2, SortDialogWidth, SortDialogHeight, "Sort songs by...", Config.main_color, Config.window_border);
	SortDialog->SetTimeout(ncmpcpp_window_timeout);
	SortDialog->SetItemDisplayer(Display::Pairs);
	
	SortDialog->AddOption(std::make_pair("Artist", &MPD::Song::GetArtist));
	SortDialog->AddOption(std::make_pair("Album", &MPD::Song::GetAlbum));
	SortDialog->AddOption(std::make_pair("Disc", &MPD::Song::GetDisc));
	SortDialog->AddOption(std::make_pair("Track", &MPD::Song::GetTrack));
	SortDialog->AddOption(std::make_pair("Genre", &MPD::Song::GetGenre));
	SortDialog->AddOption(std::make_pair("Year", &MPD::Song::GetYear));
	SortDialog->AddOption(std::make_pair("Composer", &MPD::Song::GetComposer));
	SortDialog->AddOption(std::make_pair("Performer", &MPD::Song::GetPerformer));
	SortDialog->AddOption(std::make_pair("Title", &MPD::Song::GetTitle));
	SortDialog->AddOption(std::make_pair("Filename", &MPD::Song::GetFile));
	SortDialog->AddSeparator();
	SortDialog->AddOption(std::make_pair("Sort", (MPD::Song::GetFunction)0));
	SortDialog->AddOption(std::make_pair("Cancel", (MPD::Song::GetFunction)0));
}

void Playlist::SwitchTo()
{
	if (myScreen == this)
		return;
	
	itsScrollBegin = 0;
	
	if (hasToBeResized)
		Resize();
	
	CLEAR_FIND_HISTORY;
	myScreen = this;
	w->Window::Clear();
	redraw_header = 1;
}

void Playlist::Resize()
{
	w->Resize(COLS, main_height);
	w->SetTitle(Config.columns_in_playlist ? Display::Columns(Config.song_columns_list_format) : "");
	SortDialog->MoveTo((COLS-SortDialogWidth)/2, (LINES-SortDialogHeight)/2);
	hasToBeResized = 0;
}

std::string Playlist::Title()
{
	std::string result = "Playlist ";
	if (ReloadTotalLength || ReloadRemaining)
		itsBufferedStats = TotalLength();
	result += TO_STRING(Scroller(itsBufferedStats, w->GetWidth()-result.length()-volume_state.length(), itsScrollBegin));
	return result;
}

void Playlist::EnterPressed()
{
	if (!w->Empty())
		Mpd->PlayID(w->Current().GetID());
}

void Playlist::SpacePressed()
{
	w->SelectCurrent();
	w->Scroll(wDown);
}

MPD::Song *Playlist::CurrentSong()
{
	return !w->Empty() ? &w->Current() : 0;
}

void Playlist::GetSelectedSongs(MPD::SongList &v)
{
	vector<size_t> selected;
	w->GetSelected(selected);
	for (vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); it++)
	{
		v.push_back(new MPD::Song(w->at(*it)));
	}
}

void Playlist::Sort()
{
	if (w->GetWidth() < SortDialogWidth || w->GetHeight() < SortDialogHeight)
	{
		ShowMessage("Screen is too small to display dialog window!");
		return;
	}
	
	int input;
	
	SortDialog->Reset();
	SortDialog->Display();
	
	BlockRefreshing = 1;
	while (1)
	{
		TraceMpdStatus();
		SortDialog->Refresh();
		SortDialog->ReadKey(input);
		
		if (Keypressed(input, Key.Up))
		{
			SortDialog->Scroll(wUp);
		}
		else if (Keypressed(input, Key.Down))
		{
			SortDialog->Scroll(wDown);
		}
		else if (Keypressed(input, Key.PageUp))
		{
			SortDialog->Scroll(wPageUp);
		}
		else if (Keypressed(input, Key.PageDown))
		{
			SortDialog->Scroll(wPageDown);
		}
		else if (Keypressed(input, Key.Home))
		{
			SortDialog->Scroll(wHome);
		}
		else if (Keypressed(input, Key.End))
		{
			SortDialog->Scroll(wEnd);
		}
		else if (Keypressed(input, Key.MvSongUp))
		{
			size_t pos = SortDialog->Choice();
			if (pos > 0 && pos < SortOptions)
			{
				SortDialog->Swap(pos, pos-1);
				SortDialog->Scroll(wUp);
			}
		}
		else if (Keypressed(input, Key.MvSongDown))
		{
			size_t pos = SortDialog->Choice();
			if (pos < SortOptions-1)
			{
				SortDialog->Swap(pos, pos+1);
				SortDialog->Scroll(wDown);
			}
		}
		else if (Keypressed(input, Key.Enter))
		{
			size_t pos = SortDialog->Choice();
			if (pos > SortOptions)
			{
				BlockRefreshing = 0;
				if (pos == SortOptions+1) // sort
					break;
				else if (pos == SortOptions+2) //  cancel
					return;
			}
			else
			{
				ShowMessage("Move tag types up and down to adjust sort order");
			}
		}
	}
	
	MPD::SongList playlist, cmp;
	
	playlist.reserve(w->Size());
	
	for (size_t i = 0; i < w->Size(); i++)
	{
		(*w)[i].SetPosition(i);
		playlist.push_back(&(*w)[i]);
	}
	
	cmp = playlist;
	sort(playlist.begin(), playlist.end(), Playlist::Sorting);
	
	if (playlist == cmp)
	{
		ShowMessage("Playlist is already sorted");
		return;
	}
	
	ShowMessage("Sorting playlist...");
	do
	{
		for (size_t i = 0; i < playlist.size(); i++)
		{
			if (playlist[i]->GetPosition() > int(i))
			{
				Mpd->Swap(playlist[i]->GetPosition(), i);
				std::swap(cmp[playlist[i]->GetPosition()], cmp[i]);
			}
			cmp[i]->SetPosition(i);
		}
	}
	while (playlist != cmp);
	ShowMessage("Playlist sorted!");
}

void Playlist::FixPositions(size_t beginning)
{
	bool was_filtered = w->isFiltered();
	w->ShowAll();
	for (size_t i = beginning; i < w->Size(); i++)
	{
		(*w)[i].SetPosition(i);
	}
	if (was_filtered)
		w->ShowFiltered();
}

bool Playlist::Sorting(MPD::Song *a, MPD::Song *b)
{
	for (size_t i = 0; i < SortOptions; i++)
	{
		std::string sa = (a->*(*SortDialog)[i].second)();
		std::string sb = (b->*(*SortDialog)[i].second)();
		ToLower(sa);
		ToLower(sb);
		if (sa != sb)
			return sa < sb;
	}
	return a->GetPosition() < b->GetPosition();
}

std::string Playlist::TotalLength()
{
	std::ostringstream result;
	
	if (ReloadTotalLength)
	{
		itsTotalLength = 0;
		for (size_t i = 0; i < w->Size(); i++)
			itsTotalLength += (*w)[i].GetTotalLength();
		ReloadTotalLength = 0;
	}
	if (Config.playlist_show_remaining_time && ReloadRemaining && !w->isFiltered())
	{
		itsRemainingTime = 0;
		for (size_t i = NowPlaying; i < w->Size(); i++)
			itsRemainingTime += (*w)[i].GetTotalLength();
		ReloadRemaining = 0;
	}
	
	result << '(' << w->Size() << (w->Size() == 1 ? " item" : " items");
	
	if (w->isFiltered())
	{
		w->ShowAll();
		size_t real_size = w->Size();
		w->ShowFiltered();
		if (w->Size() != real_size)
			result << " (out of " << Mpd->GetPlaylistLength() << ")";
	}
	
	if (itsTotalLength)
	{
		result << ", length: ";
		ShowTime(result, itsTotalLength);
	}
	if (Config.playlist_show_remaining_time && itsRemainingTime && !w->isFiltered() && w->Size() > 1)
	{
		result << " :: remaining: ";
		ShowTime(result, itsRemainingTime);
	}
	result << ')';
	return result.str();
}

void Playlist::ShowTime(std::ostringstream &result, size_t length)
{
	const int MINUTE = 60;
	const int HOUR = 60*MINUTE;
	const int DAY = 24*HOUR;
	const int YEAR = 365*DAY;
	
	int years = length/YEAR;
	if (years)
	{
		result << years << (years == 1 ? " year" : " years");
		length -= years*YEAR;
		if (length)
			result << ", ";
	}
	int days = length/DAY;
	if (days)
	{
		result << days << (days == 1 ? " day" : " days");
		length -= days*DAY;
		if (length)
			result << ", ";
	}
	int hours = length/HOUR;
	if (hours)
	{
		result << hours << (hours == 1 ? " hour" : " hours");
		length -= hours*HOUR;
		if (length)
			result << ", ";
	}
	int minutes = length/MINUTE;
	if (minutes)
	{
		result << minutes << (minutes == 1 ? " minute" : " minutes");
		length -= minutes*MINUTE;
		if (length)
			result << ", ";
	}
	if (length)
		result << length << (length == 1 ? " second" : " seconds");
}

const MPD::Song *Playlist::NowPlayingSong()
{
	bool was_filtered = w->isFiltered();
	w->ShowAll();
	const MPD::Song *s = isPlaying() ? &w->at(NowPlaying) : 0;
	if (was_filtered)
		w->ShowFiltered();
	return s;
}

std::string Playlist::SongToString(const MPD::Song &s, void *data)
{
	return s.toString(*static_cast<std::string *>(data));
}

std::string Playlist::SongInColumnsToString(const MPD::Song &s, void *data)
{
	std::string result;
	std::string fmt = *static_cast<std::string *>(data);
	for (std::string i = GetLineValue(fmt, '{', '}', 1); !i.empty(); i = GetLineValue(fmt, '{', '}', 1))
	{
		result += "%";
		result += i;
		result += " ";
	}
	return s.toString(result);
}

