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

#ifndef _HELPERS_H
#define _HELPERS_H

#include "interfaces.h"
#include "mpdpp.h"
#include "screen.h"
#include "settings.h"
#include "status.h"
#include "utility/wide_string.h"

inline HasColumns *hasColumns(BasicScreen *screen)
{
	return dynamic_cast<HasColumns *>(screen);
}

inline HasSongs *hasSongs(BasicScreen *screen)
{
	return dynamic_cast<HasSongs *>(screen);
}

inline std::shared_ptr<ProxySongList> proxySongList(BasicScreen *screen)
{
	auto ptr = nullProxySongList();
	auto hs = hasSongs(screen);
	if (hs)
		ptr = hs->getProxySongList();
	return ptr;
}

inline MPD::Song *currentSong(BasicScreen *screen)
{
	MPD::Song *ptr = 0;
	auto pl = proxySongList(screen);
	if (pl)
		ptr = pl->currentSong();
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
	m.showAll();
	action();
	if (is_filtered)
		m.showFiltered();
}

template <typename T, typename F>
void withUnfilteredMenuReapplyFilter(NC::Menu<T> &m, F action)
{
	m.showAll();
	action();
	if (m.getFilter())
	{
		m.applyCurrentFilter(m.begin(), m.end());
		if (m.empty())
		{
			m.clearFilter();
			m.clearFilterResults();
		}
	}
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
			swap_fun(Mpd, *it - begin, *it - begin - 1);
		if (Mpd.CommitCommandsList())
		{
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
				m.scroll(NC::wUp);
			}
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
			swap_fun(Mpd, it->base() - begin, it->base() - begin + 1);
		if (Mpd.CommitCommandsList())
		{
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
				m.scroll(NC::wDown);
			}
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
				move_fun(Mpd, *it - begin, pos+i);
			if (Mpd.CommitCommandsList())
			{
				i = list.size()-1;
				for (auto it = list.rbegin(); it != list.rend(); ++it, --i)
				{
					(*it)->setSelected(false);
					m[pos+i].setSelected(true);
				}
			}
		}
		else if (diff < 0) // move up
		{
			size_t i = 0;
			for (auto it = list.begin(); it != list.end(); ++it, ++i)
				move_fun(Mpd, *it - begin, pos+i);
			if (Mpd.CommitCommandsList())
			{
				i = 0;
				for (auto it = list.begin(); it != list.end(); ++it, ++i)
				{
					(*it)->setSelected(false);
					m[pos+i].setSelected(true);
				}
			}
		}
	});
}

template <typename F>
bool deleteSelectedSongs(NC::Menu<MPD::Song> &m, F delete_fun)
{
	bool result = false;
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
	if (Mpd.CommitCommandsList())
		result = true;
	return result;
}

template <typename F>
bool cropPlaylist(NC::Menu<MPD::Song> &m, F delete_fun)
{
	reverseSelectionHelper(m.begin(), m.end());
	return deleteSelectedSongs(m, delete_fun);
}

template <typename F, typename G>
bool clearPlaylist(NC::Menu<MPD::Song> &m, F delete_fun, G clear_fun)
{
	bool result = false;
	if (m.isFiltered())
	{
		for (auto it = m.begin(); it != m.end(); ++it)
			it->setSelected(true);
		result = deleteSelectedSongs(m, delete_fun);
	}
	else
		result = clear_fun(Mpd);
	return result;
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

template <typename T> struct StringConverter {
	const char *operator()(const char *s) { return s; }
};
template <> struct StringConverter<NC::WBuffer> {
	std::wstring operator()(const char *s) { return ToWString(s); }
};
template <> struct StringConverter<NC::Scrollpad> {
	std::wstring operator()(const char *s) { return ToWString(s); }
};

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
						buf << NC::fmtBold;
						break;
					case 'u':
						buf << NC::fmtUnderline;
						break;
					case 'a':
						buf << NC::fmtAltCharset;
						break;
					case 'r':
						buf << NC::fmtReverse;
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
								buf << NC::fmtBoldEnd;
								break;
							case 'u':
								buf << NC::fmtUnderlineEnd;
								break;
							case 'a':
								buf << NC::fmtAltCharsetEnd;
								break;
							case 'r':
								buf << NC::fmtReverseEnd;
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

template <typename T> void ShowTime(T &buf, size_t length, bool short_names)
{
	StringConverter<T> cnv;
	
	const unsigned MINUTE = 60;
	const unsigned HOUR = 60*MINUTE;
	const unsigned DAY = 24*HOUR;
	const unsigned YEAR = 365*DAY;
	
	unsigned years = length/YEAR;
	if (years)
	{
		buf << years << cnv(short_names ? "y" : (years == 1 ? " year" : " years"));
		length -= years*YEAR;
		if (length)
			buf << cnv(", ");
	}
	unsigned days = length/DAY;
	if (days)
	{
		buf << days << cnv(short_names ? "d" : (days == 1 ? " day" : " days"));
		length -= days*DAY;
		if (length)
			buf << cnv(", ");
	}
	unsigned hours = length/HOUR;
	if (hours)
	{
		buf << hours << cnv(short_names ? "h" : (hours == 1 ? " hour" : " hours"));
		length -= hours*HOUR;
		if (length)
			buf << cnv(", ");
	}
	unsigned minutes = length/MINUTE;
	if (minutes)
	{
		buf << minutes << cnv(short_names ? "m" : (minutes == 1 ? " minute" : " minutes"));
		length -= minutes*MINUTE;
		if (length)
			buf << cnv(", ");
	}
	if (length)
		buf << length << cnv(short_names ? "s" : (length == 1 ? " second" : " seconds"));
}

template <typename T> void ShowTag(T &buf, const std::string &tag)
{
	if (tag.empty())
		buf << Config.empty_tags_color << Config.empty_tag << NC::clEnd;
	else
		buf << tag;
}

bool addSongToPlaylist(const MPD::Song &s, bool play, size_t position = -1);
bool addSongsToPlaylist(const MPD::SongList &list, bool play, size_t position = -1);

std::string Timestamp(time_t t);

void markSongsInPlaylist(std::shared_ptr<ProxySongList> pl);

std::wstring Scroller(const std::wstring &str, size_t &pos, size_t width);

#endif
