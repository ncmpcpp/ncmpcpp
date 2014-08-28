/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_HELPERS_H
#define NCMPCPP_HELPERS_H

#include "interfaces.h"
#include "mpdpp.h"
#include "screen.h"
#include "settings.h"
#include "status.h"
#include "utility/string.h"
#include "utility/wide_string.h"

inline HasColumns *hasColumns(BaseScreen *screen)
{
	return dynamic_cast<HasColumns *>(screen);
}

inline HasSongs *hasSongs(BaseScreen *screen)
{
	return dynamic_cast<HasSongs *>(screen);
}

inline ProxySongList proxySongList(BaseScreen *screen)
{
	auto pl = ProxySongList();
	auto hs = hasSongs(screen);
	if (hs)
		pl = hs->proxySongList();
	return pl;
}

inline MPD::Song *currentSong(BaseScreen *screen)
{
	MPD::Song *ptr = 0;
	auto pl = proxySongList(screen);
	if (pl)
		ptr = pl.currentSong();
	return ptr;
}

template <typename Iterator>
bool hasSelected(Iterator first, Iterator last)
{
	for (; first != last; ++first)
		if (first->isSelected())
			return true;
	return false;
}

template <typename Iterator>
std::vector<Iterator> getSelected(Iterator first, Iterator last)
{
	std::vector<Iterator> result;
	for (; first != last; ++first)
		if (first->isSelected())
			result.push_back(first);
	return result;
}

/// @return selected range within given range or original range if no item is selected
template <typename Iterator>
std::pair<Iterator, Iterator> getSelectedRange(Iterator first, Iterator second)
{
	auto result = std::make_pair(first, second);
	if (hasSelected(first, second))
	{
		while (!result.first->isSelected())
			++result.first;
		while (!(result.second-1)->isSelected())
			--result.second;
	}
	return result;
}

template <typename T>
void selectCurrentIfNoneSelected(NC::Menu<T> &m)
{
	if (!hasSelected(m.begin(), m.end()))
		m.current().setSelected(true);
}

template <typename Iterator>
std::vector<Iterator> getSelectedOrCurrent(Iterator first, Iterator last, Iterator current)
{
	std::vector<Iterator> result = getSelected(first, last);
	if (result.empty())
		result.push_back(current);
	return result;
}

template <typename Iterator>
void reverseSelectionHelper(Iterator first, Iterator last)
{
	for (; first != last; ++first)
		first->setSelected(!first->isSelected());
}

template <typename T, typename F>
void withUnfilteredMenu(NC::Menu<T> &m, F action)
{
	bool is_filtered = m.isFiltered();
	auto cleanup = [&]() {
		if (is_filtered)
			m.showFiltered();
	};
	m.showAll();
	try
	{
		action();
	}
	catch (...)
	{
		cleanup();
		throw;
	}
	cleanup();
}

template <typename T, typename F>
void withUnfilteredMenuReapplyFilter(NC::Menu<T> &m, F action)
{
	auto cleanup = [&]() {
		if (m.getFilter())
		{
			m.applyCurrentFilter(m.begin(), m.end());
			if (m.empty())
			{
				m.clearFilter();
				m.clearFilterResults();
			}
		}
	};
	m.showAll();
	try
	{
		action();
	}
	catch (...)
	{
		cleanup();
		throw;
	}
	cleanup();
}

template <typename F>
void moveSelectedItemsUp(NC::Menu<MPD::Song> &m, F swap_fun)
{
	if (m.choice() > 0)
		selectCurrentIfNoneSelected(m);
	auto list = getSelected(m.begin(), m.end());
	auto begin = m.begin();
	if (!list.empty() && list.front() != m.begin())
	{
		Mpd.StartCommandsList();
		for (auto it = list.begin(); it != list.end(); ++it)
			swap_fun(&Mpd, *it - begin, *it - begin - 1);
		Mpd.CommitCommandsList();
		if (list.size() > 1)
		{
			for (auto it = list.begin(); it != list.end(); ++it)
			{
				(*it)->setSelected(false);
				(*it-1)->setSelected(true);
			}
			m.highlight(list[(list.size())/2] - begin - 1);
		}
		else
		{
			// if we move only one item, do not select it. however, if single item
			// was selected prior to move, it'll deselect it. oh well.
			list[0]->setSelected(false);
			m.scroll(NC::Scroll::Up);
		}
	}
}

template <typename F>
void moveSelectedItemsDown(NC::Menu<MPD::Song> &m, F swap_fun)
{
	if (m.choice() < m.size()-1)
		selectCurrentIfNoneSelected(m);
	auto list = getSelected(m.rbegin(), m.rend());
	auto begin = m.begin() + 1; // reverse iterators add 1, so we need to cancel it
	if (!list.empty() && list.front() != m.rbegin())
	{
		Mpd.StartCommandsList();
		for (auto it = list.begin(); it != list.end(); ++it)
			swap_fun(&Mpd, it->base() - begin, it->base() - begin + 1);
		Mpd.CommitCommandsList();
		if (list.size() > 1)
		{
			for (auto it = list.begin(); it != list.end(); ++it)
			{
				(*it)->setSelected(false);
				(*it-1)->setSelected(true);
			}
			m.highlight(list[(list.size())/2].base() - begin + 1);
		}
		else
		{
			// if we move only one item, do not select it. however, if single item
			// was selected prior to move, it'll deselect it. oh well.
			list[0]->setSelected(false);
			m.scroll(NC::Scroll::Down);
		}
	}
}

template <typename F>
void moveSelectedItemsTo(NC::Menu<MPD::Song> &m, F move_fun)
{
	auto cur_ptr = &m.current().value();
	withUnfilteredMenu(m, [&]() {
		// this is kinda shitty, but there is no other way to know
		// what position current item has in unfiltered menu.
		ptrdiff_t pos = 0;
		for (auto it = m.begin(); it != m.end(); ++it, ++pos)
			if (&it->value() == cur_ptr)
				break;
		auto begin = m.begin();
		auto list = getSelected(m.begin(), m.end());
		// we move only truly selected items
		if (list.empty())
			return;
		// we can't move to the middle of selected items
		//(this also handles case when list.size() == 1)
		if (pos >= (list.front() - begin) && pos <= (list.back() - begin))
			return;
		int diff = pos - (list.front() - begin);
		Mpd.StartCommandsList();
		if (diff > 0) // move down
		{
			pos -= list.size();
			size_t i = list.size()-1;
			for (auto it = list.rbegin(); it != list.rend(); ++it, --i)
				move_fun(&Mpd, *it - begin, pos+i);
			Mpd.CommitCommandsList();
			i = list.size()-1;
			for (auto it = list.rbegin(); it != list.rend(); ++it, --i)
			{
				(*it)->setSelected(false);
				m[pos+i].setSelected(true);
			}
		}
		else if (diff < 0) // move up
		{
			size_t i = 0;
			for (auto it = list.begin(); it != list.end(); ++it, ++i)
				move_fun(&Mpd, *it - begin, pos+i);
			Mpd.CommitCommandsList();
			i = 0;
			for (auto it = list.begin(); it != list.end(); ++it, ++i)
			{
				(*it)->setSelected(false);
				m[pos+i].setSelected(true);
			}
		}
	});
}

template <typename F>
void deleteSelectedSongs(NC::Menu<MPD::Song> &m, F delete_fun)
{
	selectCurrentIfNoneSelected(m);
	// ok, this is tricky. we need to operate on whole playlist
	// to get positions right, but at the same time we need to
	// ignore all songs that are not filtered. we use the fact
	// that both ranges share the same values, ie. we can compare
	// pointers to check whether an item belongs to filtered range.
	NC::Menu<MPD::Song>::Iterator begin;
	NC::Menu<MPD::Song>::ReverseIterator real_begin, real_end;
	withUnfilteredMenu(m, [&]() {
		// obtain iterators for unfiltered range
		begin = m.begin() + 1; // cancel reverse iterator's offset
		real_begin = m.rbegin();
		real_end = m.rend();
	});
	// get iterator to filtered range
	auto cur_filtered = m.rbegin();
	Mpd.StartCommandsList();
	for (auto it = real_begin; it != real_end; ++it)
	{
		// current iterator belongs to filtered range, proceed
		if (&*it == &*cur_filtered)
		{
			if (it->isSelected())
			{
				it->setSelected(false);
				delete_fun(Mpd, it.base() - begin);
			}
			++cur_filtered;
		}
	}
	Mpd.CommitCommandsList();
}

template <typename F>
void cropPlaylist(NC::Menu<MPD::Song> &m, F delete_fun)
{
	reverseSelectionHelper(m.begin(), m.end());
	deleteSelectedSongs(m, delete_fun);
}

template <typename F, typename G>
void clearPlaylist(NC::Menu<MPD::Song> &m, F delete_fun, G clear_fun)
{
	if (m.isFiltered())
	{
		for (auto it = m.begin(); it != m.end(); ++it)
			it->setSelected(true);
		deleteSelectedSongs(m, delete_fun);
	}
	else
		clear_fun(Mpd);
}

template <typename ItemT>
std::function<void (ItemT)> vectorMoveInserter(std::vector<ItemT> &v)
{
	return [&](ItemT item) { v.push_back(std::move(item)); };
}

template <typename Iterator> std::string getSharedDirectory(Iterator first, Iterator last)
{
	assert(first != last);
	std::string result = first->getDirectory();
	while (++first != last)
	{
		result = getSharedDirectory(result, first->getDirectory());
		if (result == "/")
			break;
	}
	return result;
}

template <typename Iterator>
void stringToBuffer(Iterator first, Iterator last, NC::BasicBuffer<typename Iterator::value_type> &buf)
{
	for (auto it = first; it != last; ++it)
	{
		if (*it == '$')
		{
			if (++it == last)
			{
				buf << '$';
				break;
			}
			else if (isdigit(*it))
			{
				buf << NC::Color(*it-'0');
			}
			else
			{
				switch (*it)
				{
					case 'b':
						buf << NC::Format::Bold;
						break;
					case 'u':
						buf << NC::Format::Underline;
						break;
					case 'a':
						buf << NC::Format::AltCharset;
						break;
					case 'r':
						buf << NC::Format::Reverse;
						break;
					case '/':
						if (++it == last)
						{
							buf << '$' << '/';
							break;
						}
						switch (*it)
						{
							case 'b':
								buf << NC::Format::NoBold;
								break;
							case 'u':
								buf << NC::Format::NoUnderline;
								break;
							case 'a':
								buf << NC::Format::NoAltCharset;
								break;
							case 'r':
								buf << NC::Format::NoReverse;
								break;
							default:
								buf << '$' << *--it;
								break;
						}
						break;
					default:
						buf << *--it;
						break;
				}
			}
		}
		else if (*it == MPD::Song::FormatEscapeCharacter)
		{
			// treat '$' as a normal character if song format escape char is prepended to it
			if (++it == last || *it != '$')
				--it;
			buf << *it;
		}
		else
			buf << *it;
	}
}

template <typename CharT>
void stringToBuffer(const std::basic_string<CharT> &s, NC::BasicBuffer<CharT> &buf)
{
	stringToBuffer(s.begin(), s.end(), buf);
}

template <typename CharT>
NC::BasicBuffer<CharT> stringToBuffer(const std::basic_string<CharT> &s)
{
	NC::BasicBuffer<CharT> result;
	stringToBuffer(s, result);
	return result;
}

template <typename T> void ShowTime(T &buf, size_t length, bool short_names)
{
	const unsigned MINUTE = 60;
	const unsigned HOUR = 60*MINUTE;
	const unsigned DAY = 24*HOUR;
	const unsigned YEAR = 365*DAY;
	
	unsigned years = length/YEAR;
	if (years)
	{
		buf << years << (short_names ? "y" : (years == 1 ? " year" : " years"));
		length -= years*YEAR;
		if (length)
			buf << ", ";
	}
	unsigned days = length/DAY;
	if (days)
	{
		buf << days << (short_names ? "d" : (days == 1 ? " day" : " days"));
		length -= days*DAY;
		if (length)
			buf << ", ";
	}
	unsigned hours = length/HOUR;
	if (hours)
	{
		buf << hours << (short_names ? "h" : (hours == 1 ? " hour" : " hours"));
		length -= hours*HOUR;
		if (length)
			buf << ", ";
	}
	unsigned minutes = length/MINUTE;
	if (minutes)
	{
		buf << minutes << (short_names ? "m" : (minutes == 1 ? " minute" : " minutes"));
		length -= minutes*MINUTE;
		if (length)
			buf << ", ";
	}
	if (length)
		buf << length << (short_names ? "s" : (length == 1 ? " second" : " seconds"));
}

template <typename BufferT> void ShowTag(BufferT &buf, const std::string &tag)
{
	if (tag.empty())
		buf << Config.empty_tags_color << Config.empty_tag << NC::Color::End;
	else
		buf << tag;
}

template <typename SongIterator>
bool addSongsToPlaylist(SongIterator first, SongIterator last, bool play, int position)
{
	bool result = true;
	auto addSongNoError = [&](SongIterator song) -> int {
		try
		{
			return Mpd.AddSong(*song, position);
		}
		catch (MPD::ServerError &e)
		{
			Status::handleServerError(e);
			result = false;
			return -1;
		}
	};

	if (last-first >= 1)
	{
		int id;
		while (true)
		{
			id = addSongNoError(first);
			if (id >= 0)
				break;
			++first;
			if (first == last)
				return result;
		}

		if (position == -1)
		{
			++first;
			for(; first != last; ++first)
				addSongNoError(first);
		}
		else
		{
			++position;
			--last;
			for (; first != last; --last)
				addSongNoError(last);
		}
		if (play)
			Mpd.PlayID(id);
	}

	return result;
}

inline const char *withErrors(bool success)
{
	return success ? "" : " " "(with errors)";
}

bool addSongToPlaylist(const MPD::Song &s, bool play, int position = -1);

std::string Timestamp(time_t t);

void markSongsInPlaylist(ProxySongList pl);

std::wstring Scroller(const std::wstring &str, size_t &pos, size_t width);
void writeCyclicBuffer(const NC::WBuffer &buf, NC::Window &w, size_t &start_pos,
                       size_t width, const std::wstring &separator);

#endif // NCMPCPP_HELPERS_H
