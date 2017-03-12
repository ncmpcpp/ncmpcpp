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

#ifndef NCMPCPP_HELP_H
#define NCMPCPP_HELP_H

#include "actions.h"
#include "interfaces.h"
#include "screens/screen.h"

struct Help: Screen<NC::Scrollpad>, Tabbable
{
	Help();
	
	virtual void resize() override;
	virtual void switchTo() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::Help; }
	
	virtual void update() override { }
	
	virtual bool isLockable() override { return true; }
	virtual bool isMergable() override { return true; }
};

extern Help *myHelp;

#endif // NCMPCPP_HELP_H

