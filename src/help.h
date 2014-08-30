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

#ifndef NCMPCPP_HELP_H
#define NCMPCPP_HELP_H

#include "actions.h"
#include "interfaces.h"
#include "screen.h"

struct Help: Screen<NC::Scrollpad>, Tabbable
{
	Help();
	
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::Help; }
	
	virtual void update() OVERRIDE { }
	
	virtual void enterPressed() OVERRIDE { }
	virtual void spacePressed() OVERRIDE { }
	
	virtual bool isMergable() OVERRIDE { return true; }
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
};

extern Help *myHelp;

#endif // NCMPCPP_HELP_H

