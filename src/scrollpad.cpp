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

#include "scrollpad.h"

Scrollpad::Scrollpad(size_t startx,
			size_t starty,
			size_t width,
			size_t height,
			const string &title,
			Color color,
			Border border)
			: Window(startx, starty, width, height, title, color, border),
			itsBeginning(0),
			itsRealHeight(1),
			itsXPos(0)
{
	delwin(itsWindow);
	itsWindow = newpad(itsHeight, itsWidth);
	SetColor(itsColor);
	keypad(itsWindow, 1);
}

Scrollpad::Scrollpad(const Scrollpad &s) : Window(s)
{
	itsBuffer << s.itsBuffer;
	itsBeginning = s.itsBeginning;
	itsRealHeight = s.itsRealHeight;
	itsXPos = s.itsXPos;
}

void Scrollpad::Flush()
{
	itsRealHeight = 1;
	
	std::basic_string<my_char_t> s = itsBuffer.Str();
	
	int x_pos = 0;
	int space_pos = 0;
	int tab_size = 0;
	
	for (size_t i = 0; i < s.length(); i++)
	{
		tab_size = 8-itsXPos%8;
		
		if (s[i] != '\t')
		{
#			ifdef _UTF8
			itsXPos += wcwidth(s[i]);
#			else
			itsXPos++;
#			endif
		}
		else
			itsXPos += tab_size;
		
		if (s[i] == ' ') // if space, remember its position;
		{
			space_pos = i;
			x_pos = itsXPos;
		}
		
		if (itsXPos >= itsWidth)
		{
			// if line is over, there was at least one space in this line and we are in the middle of the word, restore position to last known space and make it EOL
			if (space_pos > 0 && (s[i] != ' ' || s[i+1] != ' '))
			{
				i = space_pos;
				itsXPos = x_pos;
				s[i] = '\n';
			}
		}
		
		if (itsXPos >= itsWidth || s[i] == '\n')
		{
			itsRealHeight++;
			itsXPos = 0;
			space_pos = 0;
		}
	}
	Recreate();
	itsBuffer.SetTemp(&s);
	reinterpret_cast<Window &>(*this) << itsBuffer;
	itsBuffer.SetTemp(0);
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
	prefresh(itsWindow, itsBeginning, 0, itsStartY, itsStartX, itsStartY+itsHeight-1, itsStartX+itsWidth);
}

void Scrollpad::MoveTo(size_t x, size_t y)
{
	itsStartX = x;
	itsStartY = y;
}

void Scrollpad::Resize(size_t width, size_t height)
{
	/*if (width+itsStartX > size_t(COLS)
	||  height+itsStartY > size_t(LINES))
		throw BadSize();*/
	
	if (itsBorder != brNone)
	{
		delwin(itsWinBorder);
		itsWinBorder = newpad(height, width);
		wattron(itsWinBorder, COLOR_PAIR(itsBorder));
		box(itsWinBorder, 0, 0);
		width -= 2;
		height -= 2;
	}
	if (!itsTitle.empty())
		width -= 2;
	
	itsHeight = height;
	itsWidth = width;
	
	itsBeginning = 0;
	itsRealHeight = itsHeight;
	itsXPos = 0;
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
	itsXPos = 0;
	itsBuffer.Clear();
	wclear(itsWindow);
	delwin(itsWindow);
	itsWindow = newpad(itsHeight, itsWidth);
	SetTimeout(itsWindowTimeout);
	SetColor(itsColor, itsBgColor);
	keypad(itsWindow, 1);
	if (clrscr)
		Window::Clear();
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

