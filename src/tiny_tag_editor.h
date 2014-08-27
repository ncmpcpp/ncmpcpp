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

#ifndef NCMPCPP_TINY_TAG_EDITOR_H
#define NCMPCPP_TINY_TAG_EDITOR_H

#include "config.h"

#ifdef HAVE_TAGLIB_H

#include "interfaces.h"
#include "mutable_song.h"
#include "screen.h"

struct TinyTagEditor: Screen<NC::Menu<NC::Buffer>>
{
	TinyTagEditor();
	
	// Screen< NC::Menu<NC::Buffer> > implementation
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::TinyTagEditor; }
	
	virtual void update() OVERRIDE { }
	
	virtual void enterPressed() OVERRIDE;
	virtual void spacePressed() OVERRIDE { }
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;
	
	virtual bool isMergable() OVERRIDE { return true; }
	
	// private members
	void SetEdited(const MPD::Song &);
	
protected:
	virtual bool isLockable() OVERRIDE { return false; }
	
private:
	bool getTags();
	MPD::MutableSong itsEdited;
	BaseScreen *m_previous_screen;
};

extern TinyTagEditor *myTinyTagEditor;

#endif // HAVE_TAGLIB_H

#endif // NCMPCPP_TINY_TAG_EDITOR_H

