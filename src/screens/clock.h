/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#include "curses/window.h"
#include "interfaces.h"
#include "screens/screen.h"

struct Clock: Screen<NC::Window>, Tabbable
{
	Clock();
	
	virtual void resize() override;
	virtual void switchTo() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::Clock; }
	
	virtual void update() override;
	virtual void scroll(NC::Scroll) override { }
	
	virtual void mouseButtonPressed(MEVENT) override { }
	
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return true; }
	
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
