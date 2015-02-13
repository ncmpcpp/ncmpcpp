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

#ifndef NCMPCPP_EXEC_ITEM_H
#define NCMPCPP_EXEC_ITEM_H

#include <functional>

template <typename ItemT, typename FunctionT>
struct RunnableItem
{
	typedef ItemT Item;
	typedef std::function<FunctionT> Function;
	
	RunnableItem() { }
	template <typename Arg1, typename Arg2>
	RunnableItem(Arg1 &&opt, Arg2 &&f)
	: m_item(std::forward<Arg1>(opt)), m_f(std::forward<Arg2>(f)) { }
	
	template <typename... Args>
	typename Function::result_type run(Args&&... args) const
	{
		return m_f(std::forward<Args>(args)...);
	}
	
	Item &item() { return m_item; }
	const Item &item() const { return m_item; }
	
private:
	Item m_item;
	Function m_f;
};

#endif // NCMPCPP_EXEC_ITEM_H
