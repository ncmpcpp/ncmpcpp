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

#include "scrollpad.h"

using namespace NCurses;
using std::string;

Scrollpad::Scrollpad(size_t startx,
			size_t starty,
			size_t width,
			size_t height,
			const string &title,
			Color color,
			Border border)
			: Window(startx, starty, width, height, title, color, border),
			itsBeginning(0),
			itsRealHeight(1)
{
}

Scrollpad::Scrollpad(const Scrollpad &s) : Window(s)
{
	itsBuffer << s.itsBuffer;
	itsBeginning = s.itsBeginning;
	itsRealHeight = s.itsRealHeight;
}

void Scrollpad::Flush()
{
	itsRealHeight = 1;
	
	std::basic_string<my_char_t> s = itsBuffer.Str();
	
	size_t x = 0;
	
	int x_pos = 0;
	int space_pos = 0;
	int tab_size = 0;
	
	for (size_t i = 0; i < s.length(); i++)
	{
		tab_size = 8-x%8;
		
		if (s[i] != '\t')
		{
#			ifdef _UTF8
			x += wcwidth(s[i]);
#			else
			x++;
#			endif
		}
		else
			x += tab_size;
		
		if (s[i] == ' ') // if space, remember its position;
		{
			space_pos = i;
			x_pos = x;
		}
		
		if (x >= itsWidth)
		{
			// if line is over, there was at least one space in this line and we are in the middle of the word, restore position to last known space and make it EOL
			if (space_pos > 0 && (s[i] != ' ' || s[i+1] != ' '))
			{
				i = space_pos;
				x = x_pos;
				s[i] = '\n';
			}
		}
		
		if (x >= itsWidth || s[i] == '\n')
		{
			itsRealHeight++;
			x = 0;
			space_pos = 0;
		}
	}
	Recreate();
	itsBuffer.SetTemp(&s);
	static_cast<Window &>(*this) << itsBuffer;
	itsBuffer.SetTemp(0);
}

void Scrollpad::SetFormatting(short vb, const std::basic_string<my_char_t> &s, short ve, bool for_each)
{
	itsBuffer.SetFormatting(vb, s, ve, for_each);
}

void Scrollpad::Recreate()
{
	delwin(itsWindow);
	itsWindow = newpad(itsRealHeight, itsWidth);
	SetTimeout(itsWindowTimeout);
	SetColor(itsBaseColor, itsBgColor);
	keypad(itsWindow, 1);
}

void Scrollpad::Refresh()
{
	prefresh(itsWindow, itsBeginning, 0, itsStartY, itsStartX, itsStartY+itsHeight-1, itsStartX+itsWidth-1);
}

void Scrollpad::Resize(size_t width, size_t height)
{
	AdjustDimensions(width, height);
	itsBeginning = 0;
	itsRealHeight = itsHeight;
	Flush();
}

void Scrollpad::Scroll(Where where)
{
	int MaxBeginning = /*itsContent.size() < itsHeight ? 0 : */itsRealHeight-itsHeight;
	
	switch (where)
	{
		case wUp:
		{
			if (itsBeginning > 0)
				itsBeginning--;
			break;
		}
		case wDown:
		{
			if (itsBeginning < MaxBeginning)
				itsBeginning++;
			break;
		}
		case wPageUp:
		{
			itsBeginning -= itsHeight;
			if (itsBeginning < 0)
				itsBeginning = 0;
			break;
		}
		case wPageDown:
		{
			itsBeginning += itsHeight;
			if (itsBeginning > MaxBeginning)
				itsBeginning = MaxBeginning;
			break;
		}
		case wHome:
		{
			itsBeginning = 0;
			break;
		}
		case wEnd:
		{
			itsBeginning = MaxBeginning;
			break;
		}
	}
}

void Scrollpad::Clear(bool clrscr)
{
	itsBeginning = 0;
	itsRealHeight = itsHeight;
	itsBuffer.Clear();
	wclear(itsWindow);
	delwin(itsWindow);
	itsWindow = newpad(itsHeight, itsWidth);
	SetTimeout(itsWindowTimeout);
	SetColor(itsColor, itsBgColor);
	keypad(itsWindow, 1);
	if (clrscr)
		Refresh();
}

Scrollpad &Scrollpad::operator<<(std::ostream &(*os)(std::ostream&))
{
	itsBuffer << os;
	return *this;
}

#ifdef _UTF8
Scrollpad &Scrollpad::operator<<(const char *s)
{
	wchar_t *ws = ToWString(s);
	itsBuffer << ws;
	delete [] ws;
	return *this;
}

Scrollpad &Scrollpad::operator<<(const std::string &s)
{
	return operator<<(s.c_str());
}
#endif // _UTF8

Scrollpad *Scrollpad::EmptyClone() const
{
	return new Scrollpad(GetStartX(), GetStartY(), GetWidth(), GetHeight(), itsTitle, itsBaseColor, itsBorder);
}

