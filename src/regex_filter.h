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

#ifndef NCMPCPP_REGEX_FILTER_H
#define NCMPCPP_REGEX_FILTER_H

#include <boost/regex.hpp>
#include "menu.h"

template <typename T> struct RegexFilter
{
	typedef NC::Menu<T> MenuT;
	typedef typename NC::Menu<T>::Item Item;
	typedef std::function<bool(const boost::regex &, const T &)> FilterFunction;
	
	RegexFilter(boost::regex rx, FilterFunction filter)
	: m_rx(rx), m_filter(filter) { }
	
	bool operator()(const Item &item) {
		return m_filter(m_rx, item.value());
	}
	
	static std::string currentFilter(MenuT &menu)
	{
		std::string filter;
		auto rf = menu.getFilter().template target< RegexFilter<T> >();
		if (rf)
			filter = rf->m_rx.str();
		return filter;
	}
	
private:
	boost::regex m_rx;
	FilterFunction m_filter;
};

template <typename T> struct RegexItemFilter
{
	typedef NC::Menu<T> MenuT;
	typedef typename NC::Menu<T>::Item Item;
	typedef std::function<bool(const boost::regex &, const Item &)> FilterFunction;
	
	RegexItemFilter(boost::regex rx, FilterFunction filter)
	: m_rx(rx), m_filter(filter) { }
	
	bool operator()(const Item &item) {
		return m_filter(m_rx, item);
	}
	
	static std::string currentFilter(MenuT &menu)
	{
		std::string filter;
		auto rf = menu.getFilter().template target< RegexItemFilter<T> >();
		if (rf)
			filter = rf->m_rx.str();
		return filter;
	}
	
private:
	boost::regex m_rx;
	FilterFunction m_filter;
};

#endif // NCMPCPP_REGEX_FILTER_H
