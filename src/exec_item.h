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

#ifndef _EXEC_ITEM_H
#define _EXEC_ITEM_H

#include <functional>

template <typename ItemT, typename FunType> struct ExecItem
{
	typedef ItemT Item;
	typedef std::function<FunType> Function;
	
	ExecItem() { }
	ExecItem(const Item &item_, Function f) : m_item(item_), m_exec(f) { }
	
	Function &exec() { return m_exec; }
	const Function &exec() const { return m_exec; }
	
	Item &item() { return m_item; }
	const Item &item() const { return m_item; }
	
private:
	Item m_item;
	Function m_exec;
};

#endif // _EXEC_ITEM_H
