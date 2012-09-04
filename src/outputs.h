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

#include "config.h"

#ifdef ENABLE_OUTPUTS

#include "menu.h"
#include "mpdpp.h"
#include "screen.h"

class Outputs : public Screen< NC::Menu<MPD::Output> >
{
	public:
		
		// Screen< NC::Menu<MPD::Output> > implementation
		virtual void SwitchTo() OVERRIDE;
		virtual void Resize() OVERRIDE;
		
		virtual std::basic_string<my_char_t> Title() OVERRIDE;
		
		virtual void Update() OVERRIDE { }
		
		virtual void EnterPressed() OVERRIDE;
		virtual void SpacePressed() OVERRIDE { }
		virtual void MouseButtonPressed(MEVENT me) OVERRIDE;
		
		virtual bool isTabbable() OVERRIDE { return true; }
		virtual bool isMergable() OVERRIDE { return true; }
		
		// private members
		void FetchList();
		
	protected:
		virtual void Init() OVERRIDE;
		virtual bool isLockable() OVERRIDE { return true; }
};

extern Outputs *myOutputs;

#endif // ENABLE_OUTPUTS

#endif

