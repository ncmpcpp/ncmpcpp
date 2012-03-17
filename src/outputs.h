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

#ifndef _OUTPUTS_H
#define _OUTPUTS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef ENABLE_OUTPUTS

#include "menu.h"
#include "mpdpp.h"
#include "screen.h"

class Outputs : public Screen< Menu<MPD::Output> >
{
	public:
		virtual void SwitchTo();
		virtual void Resize();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed() { }
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		virtual bool allowsSelection() { return false; }
		
		virtual List *GetList() { return w; }
		
		virtual bool isMergable() { return true; }
		
		void FetchList();
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
};

extern Outputs *myOutputs;

#endif // ENABLE_OUTPUTS

#endif

