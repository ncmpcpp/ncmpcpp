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

#ifndef _REGEX_FILTER
#define _REGEX_FILTER

#include "menu.h"

template <typename T> struct RegexFilter
{
	typedef NCurses::Menu<T> MenuT;
	typedef typename NCurses::Menu<T>::Item MenuItem;
	typedef std::function<bool(const Regex &, MenuT &menu, const MenuItem &)> FilterFunction;
	
	RegexFilter(const std::string &regex_, int cflags, FilterFunction custom_filter = 0)
	: m_rx(regex_, cflags), m_custom_filter(custom_filter) { }
	
	bool operator()(MenuT &menu, const MenuItem &item) {
		if (m_rx.regex().empty())
			return true;
		if (!m_rx.error().empty())
			return false;
		if (m_custom_filter)
			return m_custom_filter(m_rx, menu, item);
		return m_rx.match(menu.Stringify(item));
	}
	
	static std::string currentFilter(NCurses::Menu<T> &menu)
	{
		std::string filter;
		auto rf = menu.getFilter().template target< RegexFilter<T> >();
		if (rf)
			filter = rf->m_rx.regex();
		return filter;
	}
	
private:
	Regex m_rx;
	FilterFunction m_custom_filter;
};



#endif
