/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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
#include "curses/menu.h"
#include "song.h"
#include "utility/const.h"

struct SongProperties
{
	enum class State { Undefined, Const, Mutable };

	SongProperties()
		: m_state(State::Undefined)
	{ }

	SongProperties &assign(NC::List::Properties *properties_, MPD::Song *song_)
	{
		m_state = State::Mutable;
		m_properties = properties_;
		m_song = song_;
		return *this;
	}

	SongProperties &assign(const NC::List::Properties *properties_, const MPD::Song *song_)
	{
		m_state = State::Const;
		m_const_properties = properties_;
		m_const_song = song_;
		return *this;
	}

	const NC::List::Properties &properties() const
	{
		assert(m_state != State::Undefined);
		return *m_const_properties;
	}
	const MPD::Song *song() const
	{
		assert(m_state != State::Undefined);
		return m_const_song;
	}

	NC::List::Properties &properties()
	{
		assert(m_state == State::Mutable);
		return *m_properties;
	}
	MPD::Song *song()
	{
		assert(m_state == State::Mutable);
		return m_song;
	}

private:
	State m_state;

	union {
		NC::List::Properties *m_properties;
		const NC::List::Properties *m_const_properties;
	};
	union {
		MPD::Song *m_song;
		const MPD::Song *m_const_song;
	};
};

template <Const const_>
using SongIteratorT = boost::range_detail::any_iterator<
	typename std::conditional<
		const_ == Const::Yes,
		const SongProperties,
		SongProperties>::type,
	boost::random_access_traversal_tag,
	typename std::conditional<
		const_ == Const::Yes,
		const SongProperties &,
		SongProperties &>::type,
	std::ptrdiff_t
	>;

typedef SongIteratorT<Const::No> SongIterator;
typedef SongIteratorT<Const::Yes> ConstSongIterator;

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

	virtual SongIterator currentS() override;
	virtual ConstSongIterator currentS() const override;
	virtual SongIterator beginS() override;
	virtual ConstSongIterator beginS() const override;
	virtual SongIterator endS() override;
	virtual ConstSongIterator endS() const override;

	virtual std::vector<MPD::Song> getSelectedSongs() override;
};

#endif // NCMPCPP_SONG_LIST_H
