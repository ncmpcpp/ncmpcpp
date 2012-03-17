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

#ifndef _SEL_ITEMS_ADDER_H
#define _SEL_ITEMS_ADDER_H

#include "ncmpcpp.h"
#include "screen.h"

class SelectedItemsAdder : public Screen< Menu<std::string> >
{
	public:
		SelectedItemsAdder() : itsPSWidth(35), itsPSHeight(11) { }
		
		virtual void SwitchTo();
		virtual void Resize();
		virtual void Refresh();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed() { }
		virtual void MouseButtonPressed(MEVENT);
		
		virtual bool allowsSelection() { return false; }
		
		virtual List *GetList() { return w; }
		
		virtual bool isMergable() { return false; }
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return false; }
		
	private:
		void SetDimensions();
		
		Menu<std::string> *itsPlaylistSelector;
		Menu<std::string> *itsPositionSelector;
		
		size_t itsPSWidth;
		size_t itsPSHeight;
		
		size_t itsWidth;
		size_t itsHeight;
};

extern SelectedItemsAdder *mySelectedItemsAdder;

#endif

