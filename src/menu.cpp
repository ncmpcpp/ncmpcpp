/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

void List::SelectCurrent()
{
	if (Empty())
		return;
	size_t i = Choice();
	Select(i, !isSelected(i));
}

void List::ReverseSelection(size_t beginning)
{
	for (size_t i = beginning; i < Size(); i++)
		Select(i, !isSelected(i) && !isStatic(i));
}

bool List::Deselect()
{
	if (!hasSelected())
		return false;
	for (size_t i = 0; i < Size(); i++)
		Select(i, 0);
	return true;
}

