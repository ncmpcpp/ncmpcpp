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

#ifndef NCMPCPP_UTILITY_COMPARATORS_H
#define NCMPCPP_UTILITY_COMPARATORS_H

#include <string>
#include "runnable_item.h"
#include "mpdpp.h"
#include "settings.h"
#include "menu.h"

class LocaleStringComparison
{
	std::locale m_locale;
	bool m_ignore_the;
	
public:
	LocaleStringComparison(const std::locale &loc, bool ignore_the)
	: m_locale(loc), m_ignore_the(ignore_the) { }
	
	int operator()(const std::string &a, const std::string &b) const;
};

class LocaleBasedSorting
{
	LocaleStringComparison m_cmp;
	
public:
	LocaleBasedSorting(const std::locale &loc, bool ignore_the) : m_cmp(loc, ignore_the) { }
	
	bool operator()(const std::string &a, const std::string &b) const {
		return m_cmp(a, b) < 0;
	}
	
	bool operator()(const MPD::Song &a, const MPD::Song &b) const {
		return m_cmp(a.getName(), b.getName()) < 0;
	}
	
	template <typename A, typename B>
	bool operator()(const std::pair<A, B> &a, const std::pair<A, B> &b) const {
		return m_cmp(a.first, b.first) < 0;
	}
	
	template <typename ItemT, typename FunT>
	bool operator()(const RunnableItem<ItemT, FunT> &a, const RunnableItem<ItemT, FunT> &b) const {
		return m_cmp(a.item(), b.item()) < 0;
	}
};

class LocaleBasedItemSorting
{
	LocaleBasedSorting m_cmp;
	SortMode m_sort_mode;
	
public:
	LocaleBasedItemSorting(const std::locale &loc, bool ignore_the, SortMode mode)
	: m_cmp(loc, ignore_the), m_sort_mode(mode) { }
	
	bool operator()(const MPD::Item &a, const MPD::Item &b) const;
	
	bool operator()(const NC::Menu<MPD::Item>::Item &a, const NC::Menu<MPD::Item>::Item &b) const {
		return (*this)(a.value(), b.value());
	}
};

#endif // NCMPCPP_UTILITY_COMPARATORS_H
