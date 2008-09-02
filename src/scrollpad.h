/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef HAVE_SCROLLPAD_H
#define HAVE_SCROLLPAD_H

#include "window.h"

class Scrollpad: public Window
{
	public:
		Scrollpad(int startx, int starty, int width, int height, string title, Color color, Border border) : Window(startx, starty, width, height, title, color, border), itsBeginning(0), itsRealHeight(1), itsXPos(0) { delwin(itsWindow); itsWindow = newpad(itsHeight,itsWidth); }
		Scrollpad(const Scrollpad &);
		virtual ~Scrollpad() {}
		virtual void Add(string);
		virtual void Display(bool = 0);
		virtual void Refresh(bool = 0);
		virtual void Go(Where);
		virtual void MoveTo(int newx, int newy) { reallocate_win(newx, newy); }
		virtual void Resize(int, int);
		virtual void SetBorder(Border);
		virtual void SetTitle(string);
		virtual void Clear(bool clear_screen = 1);
		virtual Window * Clone() { return new Scrollpad(*this); }
		virtual Window * EmptyClone();
	protected:
		void print_content();
		virtual void recreate_win();
		
		string itsContent;
		string itsRawContent;
		int itsBeginning;
		int itsRealHeight;
		int itsXPos;
};

#endif

