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

#ifndef _HELP_H
#define _HELP_H

#include "ncmpcpp.h"
#include "screen.h"

class Help : public Screen<Scrollpad>
{
	public:
		virtual void Init();
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::string Title();
		
		virtual bool allowsSelection() { return false; }
		
		virtual List *GetList() { return 0; }
		
	private:
		std::string DisplayKeys(int *, int = 2);
		void GetKeybindings();
};

extern Help *myHelp;

#endif

