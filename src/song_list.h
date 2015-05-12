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

#ifndef NCMPCPP_SONG_LIST_H
#define NCMPCPP_SONG_LIST_H

#include <boost/range/detail/any_iterator.hpp>
#include <boost/tuple/tuple.hpp>
#include "menu.h"
#include "song.h"

template <typename ValueT>
using SongIteratorT = boost::range_detail::any_iterator<
	ValueT,
	boost::random_access_traversal_tag,
	const ValueT, // const needed, see https://svn.boost.org/trac/boost/ticket/10493
	std::ptrdiff_t
>;

typedef SongIteratorT<boost::tuple<NC::List::Properties &, MPD::Song *>> SongIterator;
typedef SongIteratorT<boost::tuple<const NC::List::Properties &, const MPD::Song *>> ConstSongIterator;

namespace Bit {
const size_t Properties = 0;
const size_t Song = 1;
}

struct SongList
{
	virtual SongIterator currentS() = 0;
	virtual ConstSongIterator currentS() const = 0;
	virtual SongIterator beginS() = 0;
	virtual ConstSongIterator beginS() const = 0;
	virtual SongIterator endS() = 0;
	virtual ConstSongIterator endS() const = 0;

	virtual std::vector<MPD::Song> getSelectedSongs() = 0;
};

inline SongIterator begin(SongList &list) { return list.beginS(); }
inline ConstSongIterator begin(const SongList &list) { return list.beginS(); }
inline SongIterator end(SongList &list) { return list.endS(); }
inline ConstSongIterator end(const SongList &list) { return list.endS(); }

struct SongMenu: NC::Menu<MPD::Song>, SongList
{
	SongMenu() { }
	SongMenu(NC::Menu<MPD::Song> &&base)
	: NC::Menu<MPD::Song>(std::move(base)) { }

	virtual SongIterator currentS() OVERRIDE;
	virtual ConstSongIterator currentS() const OVERRIDE;
	virtual SongIterator beginS() OVERRIDE;
	virtual ConstSongIterator beginS() const OVERRIDE;
	virtual SongIterator endS() OVERRIDE;
	virtual ConstSongIterator endS() const OVERRIDE;

	virtual std::vector<MPD::Song> getSelectedSongs() OVERRIDE;
};

#endif // NCMPCPP_SONG_LIST_H