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
#include <sstream>

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "menu.h"
#include "playlist.h"
#include "regex_filter.h"
#include "song.h"
#include "status.h"
#include "utility/comparators.h"

using namespace std::placeholders;

using Global::MainHeight;
using Global::MainStartY;

Playlist *myPlaylist = new Playlist;

bool Playlist::ReloadTotalLength = 0;
bool Playlist::ReloadRemaining = false;

namespace {//

NC::Menu< std::pair<std::string, MPD::Song::GetFunction> > *SortDialog = 0;
size_t SortDialogHeight;

const size_t SortOptions = 10;
const size_t SortDialogWidth = 30;

std::string songToString(const MPD::Song &s);
bool playlistEntryMatcher(const Regex &rx, const MPD::Song &s);

}

void Playlist::Init()
{
	Items = new NC::Menu<MPD::Song>(0, MainStartY, COLS, MainHeight, Config.columns_in_playlist && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::brNone);
	Items->cyclicScrolling(Config.use_cyclic_scrolling);
	Items->centeredCursor(Config.centered_cursor);
	Items->setHighlightColor(Config.main_highlight_color);
	Items->setSelectedPrefix(Config.selected_item_prefix);
	Items->setSelectedSuffix(Config.selected_item_suffix);
	if (Config.columns_in_playlist)
		Items->setItemDisplayer(std::bind(Display::SongsInColumns, _1, this));
	else
		Items->setItemDisplayer(std::bind(Display::Songs, _1, this, Config.song_list_format));
	
	if (!SortDialog)
	{
		SortDialogHeight = std::min(int(MainHeight), 17);
		
		SortDialog = new NC::Menu< std::pair<std::string, MPD::Song::GetFunction> >((COLS-SortDialogWidth)/2, (MainHeight-SortDialogHeight)/2+MainStartY, SortDialogWidth, SortDialogHeight, "Sort songs by...", Config.main_color, Config.window_border);
		SortDialog->cyclicScrolling(Config.use_cyclic_scrolling);
		SortDialog->centeredCursor(Config.centered_cursor);
		SortDialog->setItemDisplayer(Display::Pair<std::string, MPD::Song::GetFunction>);
		
		SortDialog->addItem(std::make_pair("Artist", &MPD::Song::getArtist));
		SortDialog->addItem(std::make_pair("Album", &MPD::Song::getAlbum));
		SortDialog->addItem(std::make_pair("Disc", &MPD::Song::getDisc));
		SortDialog->addItem(std::make_pair("Track", &MPD::Song::getTrack));
		SortDialog->addItem(std::make_pair("Genre", &MPD::Song::getGenre));
		SortDialog->addItem(std::make_pair("Date", &MPD::Song::getDate));
		SortDialog->addItem(std::make_pair("Composer", &MPD::Song::getComposer));
		SortDialog->addItem(std::make_pair("Performer", &MPD::Song::getPerformer));
		SortDialog->addItem(std::make_pair("Title", &MPD::Song::getTitle));
		SortDialog->addItem(std::make_pair("Filename", &MPD::Song::getURI));
		SortDialog->addSeparator();
		SortDialog->addItem(std::make_pair("Sort", static_cast<MPD::Song::GetFunction>(0)));
		SortDialog->addItem(std::make_pair("Cancel", static_cast<MPD::Song::GetFunction>(0)));
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
	EnableHighlighting();
	if (w != Items) // even if sorting window is active, background has to be refreshed anyway
		Items->display();
	DrawHeader();
}

void Playlist::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	Items->resize(width, MainHeight);
	Items->moveTo(x_offset, MainStartY);

	Items->setTitle(Config.columns_in_playlist && Config.titles_visibility ? Display::Columns(Items->getWidth()) : "");
	if (w == SortDialog) // if sorting window is active, playlist needs refreshing
		Items->display();
	
	SortDialogHeight = std::min(int(MainHeight), 17);
	if (Items->getWidth() >= SortDialogWidth && MainHeight >= 5)
	{
		SortDialog->resize(SortDialogWidth, SortDialogHeight);
		SortDialog->moveTo(x_offset+(width-SortDialogWidth)/2, (MainHeight-SortDialogHeight)/2+MainStartY);
	}
	else // if screen is too low to display sorting window, fall back to items list
		w = Items;
	
	hasToBeResized = 0;
}

std::wstring Playlist::Title()
{
	std::wstring result = L"Playlist ";
	if (ReloadTotalLength || ReloadRemaining)
		itsBufferedStats = TotalLength();
	result += Scroller(ToWString(itsBufferedStats), itsScrollBegin, COLS-result.length()-(Config.new_design ? 2 : Global::VolumeState.length()));
	return result;
}

void Playlist::EnterPressed()
{
	if (w == Items)
	{
		if (!Items->empty())
			Mpd.PlayID(Items->current().value().getID());
	}
	else if (w == SortDialog)
	{
		size_t pos = SortDialog->choice();
		
		auto begin = Items->begin(), end = Items->end();
		// if songs are selected, sort range from first selected to last selected
		if (Items->hasSelected())
		{
			while (!begin->isSelected())
				++begin;
			while (!(end-1)->isSelected())
				--end;
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
		
		size_t start_pos = begin-Items->begin();
		MPD::SongList playlist;
		playlist.reserve(end-begin);
		for (; begin != end; ++begin)
			playlist.push_back(begin->value());
		
		LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
		std::function<void(MPD::SongList::iterator, MPD::SongList::iterator)> iter_swap, quick_sort;
		auto song_cmp = [&cmp](const MPD::Song &a, const MPD::Song &b) -> bool {
				for (size_t i = 0; i < SortOptions; ++i)
					if (int ret = cmp(a.getTags((*SortDialog)[i].value().second), b.getTags((*SortDialog)[i].value().second)))
						return ret < 0;
				return a.getPosition() < b.getPosition();
		};
		iter_swap = [&playlist, &start_pos](MPD::SongList::iterator a, MPD::SongList::iterator b) {
			std::iter_swap(a, b);
			Mpd.Swap(start_pos+a-playlist.begin(), start_pos+b-playlist.begin());
		};
		quick_sort = [this, &song_cmp, &quick_sort, &iter_swap](MPD::SongList::iterator first, MPD::SongList::iterator last) {
			if (last-first > 1)
			{
				MPD::SongList::iterator pivot = first+rand()%(last-first);
				iter_swap(pivot, last-1);
				pivot = last-1;
				
				MPD::SongList::iterator tmp = first;
				for (MPD::SongList::iterator i = first; i != pivot; ++i)
					if (song_cmp(*i, *pivot))
						iter_swap(i, tmp++);
				iter_swap(tmp, pivot);
				
				quick_sort(first, tmp);
				quick_sort(tmp+1, last);
			}
		};
		
		ShowMessage("Sorting...");
		Mpd.StartCommandsList();
		quick_sort(playlist.begin(), playlist.end());
		if (Mpd.CommitCommandsList())
			ShowMessage("Playlist sorted");
		w = Items;
	}
}

void Playlist::SpacePressed()
{
	if (w == Items && !Items->empty())
	{
		Items->current().setSelected(!Items->current().isSelected());
		Items->scroll(NC::wDown);
	}
}

void Playlist::MouseButtonPressed(MEVENT me)
{
	if (w == Items && !Items->empty() && Items->hasCoords(me.x, me.y))
	{
		if (size_t(me.y) < Items->size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Items->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				EnterPressed();
		}
		else
			Screen<NC::Window>::MouseButtonPressed(me);
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
			Screen<NC::Window>::MouseButtonPressed(me);
	}
}

/***********************************************************************/

bool Playlist::allowsFiltering()
{
	return true;
}

std::string Playlist::currentFilter()
{
	std::string filter;
	if (w == Items)
		filter = RegexFilter<MPD::Song>::currentFilter(*Items);
	return filter;
}

void Playlist::applyFilter(const std::string &filter)
{
	if (w == Items)
	{
		Items->showAll();
		auto rx = RegexFilter<MPD::Song>(filter, Config.regex_type, playlistEntryMatcher);
		Items->filter(Items->begin(), Items->end(), rx);
	}
}

/***********************************************************************/

bool Playlist::allowsSearching()
{
	return true;
}

bool Playlist::search(const std::string &constraint)
{
	bool result = false;
	if (w == Items)
	{
		auto rx = RegexFilter<MPD::Song>(constraint, Config.regex_type, playlistEntryMatcher);
		result = Items->search(Items->begin(), Items->end(), rx);
	}
	return result;
}

void Playlist::nextFound(bool wrap)
{
	if (w == Items)
		Items->nextFound(wrap);
}

void Playlist::prevFound(bool wrap)
{
	if (w == Items)
		Items->prevFound(wrap);
}

/***********************************************************************/

std::shared_ptr<ProxySongList> Playlist::getProxySongList()
{
	auto ptr = nullProxySongList();
	if (w == Items)
		ptr = mkProxySongList(*Items, [](NC::Menu<MPD::Song>::Item &item) {
			return &item.value();
		});
	return ptr;
}

bool Playlist::allowsSelection()
{
	return w == Items;
}

void Playlist::reverseSelection()
{
	reverseSelectionHelper(Items->begin(), Items->end());
}

MPD::SongList Playlist::getSelectedSongs()
{
	MPD::SongList result;
	if (w == Items)
	{
		for (auto it = Items->begin(); it != Items->end(); ++it)
			if (it->isSelected())
				result.push_back(it->value());
		if (result.empty() && !Items->empty())
			result.push_back(Items->current().value());
	}
	return result;
}

/***********************************************************************/

bool Playlist::isFiltered()
{
	if (Items->isFiltered())
	{
		ShowMessage("Function currently unavailable due to filtered playlist");
		return true;
	}
	return false;
}

void Playlist::Sort()
{
	if (isFiltered())
		return;
	if (Items->getWidth() < SortDialogWidth || MainHeight < 5)
		ShowMessage("Screen is too small to display dialog window");
	else
	{
		SortDialog->reset();
		w = SortDialog;
	}
}

void Playlist::Reverse()
{
	if (isFiltered())
		return;
	ShowMessage("Reversing playlist order...");
	size_t beginning = -1, end = -1;
	for (size_t i = 0; i < Items->size(); ++i)
	{
		if (Items->at(i).isSelected())
		{
			if (beginning == size_t(-1))
				beginning = i;
			end = i;
		}
	}
	if (beginning == size_t(-1)) // no selected items
	{
		beginning = 0;
		end = Items->size();
	}
	Mpd.StartCommandsList();
	for (size_t i = beginning, j = end-1; i < (beginning+end)/2; ++i, --j)
		Mpd.Swap(i, j);
	if (Mpd.CommitCommandsList())
		 ShowMessage("Playlist reversed");
}

void Playlist::EnableHighlighting()
{
	Items->setHighlighting(1);
	UpdateTimer();
}

void Playlist::UpdateTimer()
{
	itsTimer = Global::Timer;
}

bool Playlist::SortingInProgress()
{
	return w == SortDialog;
}

std::string Playlist::TotalLength()
{
	std::ostringstream result;
	
	if (ReloadTotalLength)
	{
		itsTotalLength = 0;
		for (size_t i = 0; i < Items->size(); ++i)
			itsTotalLength += (*Items)[i].value().getDuration();
		ReloadTotalLength = 0;
	}
	if (Config.playlist_show_remaining_time && ReloadRemaining && !Items->isFiltered())
	{
		itsRemainingTime = 0;
		for (size_t i = NowPlaying; i < Items->size(); ++i)
			itsRemainingTime += (*Items)[i].value().getDuration();
		ReloadRemaining = false;
	}
	
	result << '(' << Items->size() << (Items->size() == 1 ? " item" : " items");
	
	if (Items->isFiltered())
	{
		Items->showAll();
		size_t real_size = Items->size();
		Items->showFiltered();
		if (Items->size() != real_size)
			result << " (out of " << Mpd.GetPlaylistLength() << ")";
	}
	
	if (itsTotalLength)
	{
		result << ", length: ";
		ShowTime(result, itsTotalLength, Config.playlist_shorten_total_times);
	}
	if (Config.playlist_show_remaining_time && itsRemainingTime && !Items->isFiltered() && Items->size() > 1)
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
	Items->showAll();
	const MPD::Song *s = isPlaying() ? &Items->at(NowPlaying).value() : 0;
	if (was_filtered)
		Items->showFiltered();
	return s;
}

bool Playlist::Add(const MPD::Song &s, bool play, int position)
{
	if (Config.ncmpc_like_songs_adding && checkForSong(s))
	{
		size_t hash = s.getHash();
		if (play)
		{
			for (size_t i = 0; i < Items->size(); ++i)
			{
				if (Items->at(i).value().getHash() == hash)
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
			for (size_t i = 0; i < Items->size(); ++i)
				if ((*Items)[i].value().getHash() == hash)
					Mpd.Delete(i);
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
	if (position < 0)
	{
		for (auto it = l.begin(); it != l.end(); ++it)
			if (Mpd.AddSong(*it) < 0)
				break;
	}
	else
	{
		for (auto j = l.rbegin(); j != l.rend(); ++j)
			if (Mpd.AddSong(*j, position) < 0)
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
	Items->showAll();
	size_t old_size = Items->size();
	Mpd.UpdateStatus();
	if (old_size < Items->size())
		Mpd.Play(old_size);
	if (is_filtered)
		Items->showFiltered();
}

void Playlist::SetSelectedItemsPriority(int prio)
{
	auto list = getSelectedOrCurrent(Items->begin(), Items->end(), Items->currentI());
	Mpd.StartCommandsList();
	for (auto it = list.begin(); it != list.end(); ++it)
		Mpd.SetPriority((*it)->value(), prio);
	if (Mpd.CommitCommandsList())
		ShowMessage("Priority set");
}

bool Playlist::checkForSong(const MPD::Song &s)
{
	return itsSongHashes.find(s.getHash()) != itsSongHashes.end();
}

void Playlist::moveSortOrderDown()
{
	size_t pos = SortDialog->choice();
	if (pos < SortOptions-1)
	{
		SortDialog->Swap(pos, pos+1);
		SortDialog->scroll(NC::wDown);
	}
}

void Playlist::moveSortOrderUp()
{
	size_t pos = SortDialog->choice();
	if (pos > 0 && pos < SortOptions)
	{
		SortDialog->Swap(pos, pos-1);
		SortDialog->scroll(NC::wUp);
	}
}

void Playlist::registerHash(size_t hash)
{
	itsSongHashes[hash] += 1;
}

void Playlist::unregisterHash(size_t hash)
{
	auto it = itsSongHashes.find(hash);
	assert(it != itsSongHashes.end());
	if (it->second == 1)
		itsSongHashes.erase(it);
	else
		it->second -= 1;
}

namespace {//

std::string songToString(const MPD::Song &s)
{
	std::string result;
	if (Config.columns_in_playlist)
		result = s.toString(Config.song_in_columns_to_string_format);
	else
		result = s.toString(Config.song_list_format_dollar_free);
	return result;
}

bool playlistEntryMatcher(const Regex &rx, const MPD::Song &s)
{
	return rx.match(songToString(s));
}

}
