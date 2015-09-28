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

#ifndef NCMPCPP_SORT_PLAYLIST_H
#define NCMPCPP_SORT_PLAYLIST_H

#include "runnable_item.h"
#include "interfaces.h"
#include "screen.h"
#include "song.h"

struct SortPlaylistDialog
	: Screen<NC::Menu<RunnableItem<std::pair<std::string, MPD::Song::GetFunction>, void()>>>, HasActions, Tabbable
{
	typedef SortPlaylistDialog Self;
	
	SortPlaylistDialog();
	
	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::SortPlaylistDialog; }
	
	virtual void update() OVERRIDE { }
	
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;
	
	virtual bool isLockable() OVERRIDE { return false; }
	virtual bool isMergable() OVERRIDE { return false; }

	// HasActions implementation
	virtual bool actionRunnable() OVERRIDE;
	virtual void runAction() OVERRIDE;

	// private members
	void moveSortOrderUp();
	void moveSortOrderDown();
	
private:
	void moveSortOrderHint() const;
	void sort() const;
	void cancel() const;
	
	void setDimensions();
	
	size_t m_height;
	size_t m_width;
};

extern SortPlaylistDialog *mySortPlaylistDialog;

#endif // NCMPCPP_SORT_PLAYLIST_H
