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

#include "helpers/song_iterator_maker.h"
#include "screens/song_info.h"
#include "utility/functional.h"

SongIterator SongMenu::currentS()
{
	return makeSongIterator(current());
}

ConstSongIterator SongMenu::currentS() const
{
	return makeConstSongIterator(current());
}

SongIterator SongMenu::beginS()
{
	return makeSongIterator(begin());
}

ConstSongIterator SongMenu::beginS() const
{
	return makeConstSongIterator(begin());
}

SongIterator SongMenu::endS()
{
	return makeSongIterator(end());
}

ConstSongIterator SongMenu::endS() const
{
	return makeConstSongIterator(end());
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
