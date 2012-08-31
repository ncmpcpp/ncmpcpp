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

#include "menu.h"

namespace NCurses {

template <> std::string Menu<std::string>::GetItem(size_t pos)
{
	std::string result;
	if (m_options_ptr->at(pos))
	{
		if (m_item_stringifier)
			result = m_item_stringifier((*m_options_ptr)[pos]->value());
		else
			result = (*m_options_ptr)[pos]->value();
	}
	return result;
}

template <> std::string Menu<std::string>::Stringify(const Menu<std::string>::Item &item) const
{
	std::string result;
	if (m_item_stringifier)
		result = m_item_stringifier(item.value());
	else
		result = item.value();
	return result;
}

}
