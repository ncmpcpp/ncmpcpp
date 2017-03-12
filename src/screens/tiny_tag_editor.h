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

#ifndef NCMPCPP_TINY_TAG_EDITOR_H
#define NCMPCPP_TINY_TAG_EDITOR_H

#include "config.h"

#ifdef HAVE_TAGLIB_H

#include "interfaces.h"
#include "mutable_song.h"
#include "screens/screen.h"

struct TinyTagEditor: Screen<NC::Menu<NC::Buffer>>, HasActions
{
	TinyTagEditor();
	
	// Screen< NC::Menu<NC::Buffer> > implementation
	virtual void resize() override;
	virtual void switchTo() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::TinyTagEditor; }
	
	virtual void update() override { }
	
	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return true; }

	// HasActions implementation
	virtual bool actionRunnable() override;
	virtual void runAction() override;

	// private members
	void SetEdited(const MPD::Song &);
	
private:
	bool getTags();
	MPD::MutableSong itsEdited;
	BaseScreen *m_previous_screen;
};

extern TinyTagEditor *myTinyTagEditor;

#endif // HAVE_TAGLIB_H

#endif // NCMPCPP_TINY_TAG_EDITOR_H

