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

#include "helpers/song_iterator_maker.h"
#include "song_info.h"
#include "utility/functional.h"

namespace {

template <bool Const>
struct SongExtractor
{
	typedef SongExtractor type;

	typedef typename NC::Menu<MPD::Song>::Item MenuItem;
	typedef typename std::conditional<Const, const MenuItem, MenuItem>::type Item;
	typedef typename std::conditional<Const, const MPD::Song, MPD::Song>::type Song;

	Song *operator()(Item &item) const
	{
		return &item.value();
	}
};

}

SongIterator SongMenu::currentS()
{
	return makeSongIterator_<MPD::Song>(current(), SongExtractor<false>());
}

ConstSongIterator SongMenu::currentS() const
{
	return makeConstSongIterator_<MPD::Song>(current(), SongExtractor<true>());
}

SongIterator SongMenu::beginS()
{
	return makeSongIterator_<MPD::Song>(begin(), SongExtractor<false>());
}

ConstSongIterator SongMenu::beginS() const
{
	return makeConstSongIterator_<MPD::Song>(begin(), SongExtractor<true>());
}

SongIterator SongMenu::endS()
{
	return makeSongIterator_<MPD::Song>(end(), SongExtractor<false>());
}

ConstSongIterator SongMenu::endS() const
{
	return makeConstSongIterator_<MPD::Song>(end(), SongExtractor<true>());
}

std::vector<MPD::Song> SongMenu::getSelectedSongs()
{
	std::vector<MPD::Song> result;
	for (auto it = begin(); it != end(); ++it)
		if (it->isSelected())
			result.push_back(it->value());
	if (result.empty() && !empty())
		result.push_back(current()->value());
	return result;
}
