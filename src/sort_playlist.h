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

#ifndef _SORT_PLAYLIST
#define _SORT_PLAYLIST

#include "screen.h"
#include "song.h"

struct SortPlaylistDialog : public Screen<NC::Menu<std::pair<std::string, MPD::Song::GetFunction>>>
{
	SortPlaylistDialog();
	
	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	
	virtual void update() OVERRIDE { }
	
	virtual void enterPressed() OVERRIDE;
	virtual void spacePressed() OVERRIDE { }
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;
	
	virtual bool isTabbable() OVERRIDE { return false; }
	virtual bool isMergable() OVERRIDE { return false; }
	
	// private members
	void moveSortOrderUp();
	void moveSortOrderDown();
	
protected:
	virtual bool isLockable() OVERRIDE { return false; }
	
private:
	void setDimensions();
	
	size_t m_sort_options;
	size_t m_height;
	size_t m_width;
	
	const WindowType::Item::Type m_sort_entry;
	const WindowType::Item::Type m_cancel_entry;
};

extern SortPlaylistDialog *mySortPlaylistDialog;

#endif // _SORT_PLAYLIST
