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
#include <cassert>

template <typename T>
struct RegexFilter
{
	typedef NC::Menu<T> MenuT;
	typedef typename NC::Menu<T>::Item Item;
	typedef std::function<bool(const boost::regex &, const T &)> FilterFunction;
	
	RegexFilter() { }
	RegexFilter(boost::regex rx, FilterFunction filter)
	: m_rx(std::move(rx)), m_filter(std::move(filter)) { }

	void clear()
	{
		m_filter = nullptr;
	}

	bool operator()(const Item &item) const {
		assert(defined());
		return m_filter(m_rx, item.value());
	}

	bool defined() const
	{
		return m_filter.operator bool();
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
	
	RegexItemFilter() { }
	RegexItemFilter(boost::regex rx, FilterFunction filter)
	: m_rx(std::move(rx)), m_filter(std::move(filter)) { }
	
	void clear()
	{
		m_filter = nullptr;
	}

	bool operator()(const Item &item) {
		return m_filter(m_rx, item);
	}
	
	bool defined() const
	{
		return m_filter.operator bool();
	}

private:
	boost::regex m_rx;
	FilterFunction m_filter;
};

#endif // NCMPCPP_REGEX_FILTER_H
