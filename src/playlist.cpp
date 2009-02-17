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

bool Playlist::BlockNowPlayingUpdate = 0;
bool Playlist::BlockUpdate = 0;

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
}

void Playlist::SwitchTo()
{
	if (myScreen == this)
		return;
	
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
	hasToBeResized = 0;
}

std::string Playlist::Title()
{
	std::string result = "Playlist ";
	result += TotalLength();
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

std::string Playlist::TotalLength()
{
	std::ostringstream result;
	
	const int MINUTE = 60;
	const int HOUR = 60*MINUTE;
	const int DAY = 24*HOUR;
	const int YEAR = 365*DAY;
	int length = 0;
	
	for (size_t i = 0; i < w->Size(); i++)
		length += w->at(i).GetTotalLength();
	
	result << '(' << w->Size() << (w->Size() == 1 ? " item" : " items");
	
	if (w->isFiltered())
	{
		w->ShowAll();
		size_t real_size = w->Size();
		w->ShowFiltered();
		if (w->Size() != real_size)
			result << " (out of " << Mpd->GetPlaylistLength() << ")";
	}
	
	if (length)
	{
		result << ", length: ";
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
	result << ')';
	return result.str();
}

const MPD::Song &Playlist::NowPlayingSong()
{
	static MPD::Song null;
	return isPlaying() ? w->at(NowPlaying) : null;
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

