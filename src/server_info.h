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

#ifndef _SERVER_INFO
#define _SERVER_INFO

#include "screen.h"

struct ServerInfo : public Screen<NC::Scrollpad>
{
	ServerInfo();
	
	// Screen<NC::Scrollpad> implementation
	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	
	virtual void update() OVERRIDE;
	
	virtual void enterPressed() OVERRIDE { }
	virtual void spacePressed() OVERRIDE { }
	
	virtual bool isMergable() OVERRIDE { return false; }
	virtual bool isTabbable() OVERRIDE { return false; }
	
protected:
	virtual bool isLockable() OVERRIDE { return false; }
	
private:
	void SetDimensions();
	
	MPD::StringList itsURLHandlers;
	MPD::StringList itsTagTypes;
	
	size_t itsWidth;
	size_t itsHeight;
};

extern ServerInfo *myServerInfo;

#endif

