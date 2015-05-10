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

#ifndef NCMPCPP_HELPERS_SONG_ITERATOR_MAKER_H
#define NCMPCPP_HELPERS_SONG_ITERATOR_MAKER_H

#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include "menu.h"
#include "song_list.h"

template <typename ItemT, typename TransformT>
SongIterator makeSongIterator_(typename NC::Menu<ItemT>::Iterator it, TransformT &&map)
{
	return SongIterator(boost::make_zip_iterator(boost::make_tuple(
		typename NC::Menu<ItemT>::PropertiesIterator(it),
		boost::make_transform_iterator(it, std::forward<TransformT>(map))
	)));
}

template <typename ItemT, typename TransformT>
ConstSongIterator makeConstSongIterator_(typename NC::Menu<ItemT>::ConstIterator it, TransformT &&map)
{
	return ConstSongIterator(boost::make_zip_iterator(boost::make_tuple(
		typename NC::Menu<ItemT>::ConstPropertiesIterator(it),
		boost::make_transform_iterator(it, std::forward<TransformT>(map))
	)));
}

#endif // NCMPCPP_HELPERS_SONG_ITERATOR_MAKER_H
