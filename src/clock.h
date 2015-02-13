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

#ifndef NCMPCPP_CLOCK_H
#define NCMPCPP_CLOCK_H

#include "config.h"

#ifdef ENABLE_CLOCK

#include "interfaces.h"
#include "screen.h"
#include "window.h"

struct Clock: Screen<NC::Window>, Tabbable
{
	Clock();
	
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::Clock; }
	
	virtual void update() OVERRIDE;
	virtual void scroll(NC::Scroll) OVERRIDE { }
	
	virtual void enterPressed() OVERRIDE { }
	virtual void spacePressed() OVERRIDE { }
	virtual void mouseButtonPressed(MEVENT) OVERRIDE { }
	
	virtual bool isMergable() OVERRIDE { return true; }
	
protected:
	virtual bool isLockable() OVERRIDE { return false; }
	
private:
	NC::Window m_pane;
	
	static void Prepare();
	static void Set(int, int);
	
	static short disp[11];
	static long older[6], next[6], newer[6], mask;
	
	static size_t Width;
	static const size_t Height;
};

extern Clock *myClock;

#endif // ENABLE_CLOCK

#endif // NCMPCPP_CLOCK_H
