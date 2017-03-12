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

#ifndef NCMPCPP_HELPERS_SONG_ITERATOR_MAKER_H
#define NCMPCPP_HELPERS_SONG_ITERATOR_MAKER_H

#include <boost/iterator/transform_iterator.hpp>
#include "curses/menu.h"
#include "song_list.h"

template <typename SongT>
struct SongPropertiesExtractor
{
	template <typename ItemT>
	auto &operator()(ItemT &item) const
	{
		return m_cache.assign(&item.properties(), &item.value());
	}

private:
	mutable SongProperties m_cache;
};

template <typename IteratorT>
SongIterator makeSongIterator(IteratorT it)
{
	typedef SongPropertiesExtractor<
		typename IteratorT::value_type::Type
		> Extractor;
	static_assert(
		std::is_convertible<
		  typename std::result_of<Extractor(typename IteratorT::reference)>::type,
		  SongProperties &
		>::value, "invalid result type of SongPropertiesExtractor");
	return SongIterator(boost::make_transform_iterator(it, Extractor{}));
}

template <typename ConstIteratorT>
ConstSongIterator makeConstSongIterator(ConstIteratorT it)
{
	typedef SongPropertiesExtractor<
		typename ConstIteratorT::value_type::Type
		> Extractor;
	static_assert(
		std::is_convertible<
		  typename std::result_of<Extractor(typename ConstIteratorT::reference)>::type,
		  const SongProperties &
		>::value, "invalid result type of SongPropertiesExtractor");
	return ConstSongIterator(boost::make_transform_iterator(it, Extractor{}));
}

#endif // NCMPCPP_HELPERS_SONG_ITERATOR_MAKER_H
