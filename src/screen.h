/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#ifndef _SCREEN_H
#define _SCREEN_H

#include "window.h"
#include "menu.h"
#include "mpdpp.h"
#include "helpers.h"
#include "status.h"

class BasicScreen
{
	public:
		BasicScreen() : hasToBeResized(0) { }
		virtual ~BasicScreen() { }
		
		virtual void *Cmp() = 0;
		
		virtual void Init() = 0;
		virtual void SwitchTo() = 0;
		virtual void Resize() = 0;
		
		virtual std::string Title() = 0;
		
		virtual void Update() { }
		virtual void Refresh() = 0;
		virtual void RefreshWindow() = 0;
		virtual void ReadKey(int &) = 0;
		virtual void Scroll(Where, const int * = 0) = 0;
		
		virtual void EnterPressed() = 0;
		virtual void SpacePressed() = 0;
		
		virtual MPD::Song *CurrentSong() { return 0; }
		
		virtual bool allowsSelection() = 0;
		virtual void ReverseSelection() { }
		virtual void GetSelectedSongs(MPD::SongList &) { }
		
		virtual void ApplyFilter(const std::string &) { }
		
		virtual List *GetList() = 0;
		
		bool hasToBeResized;
};

template <typename WindowType> class Screen : public BasicScreen
{
	public:
		Screen() : w(0) { }
		virtual ~Screen() { }
		
		virtual void *Cmp();
		
		WindowType *Main();
		
		virtual void Refresh();
		virtual void RefreshWindow();
		virtual void ReadKey(int &input);
		virtual void Scroll(Where where, const int *);
		
	protected:
		WindowType *w;
};

template <typename WindowType> void *Screen<WindowType>::Cmp()
{
	return w;
}

template <typename WindowType> WindowType *Screen<WindowType>::Main()
{
	return w;
}

template <typename WindowType> void Screen<WindowType>::Refresh()
{
	w->Display();
}

template <typename WindowType> void Screen<WindowType>::RefreshWindow()
{
	w->Display();
}

template <typename WindowType> void Screen<WindowType>::ReadKey(int &input)
{
	w->ReadKey(input);
}

template <typename WindowType> void Screen<WindowType>::Scroll(Where where, const int *key)
{
	if (!Config.fancy_scrolling && key)
	{
		int in = key[0];
		w->SetTimeout(ncmpcpp_window_timeout/10);
		while (Keypressed(in, key))
		{
			TraceMpdStatus();
			w->Scroll(where);
			w->Refresh();
			w->ReadKey(in);
		}
		w->SetTimeout(ncmpcpp_window_timeout);
	}
	else
		w->Scroll(where);
}

#endif

