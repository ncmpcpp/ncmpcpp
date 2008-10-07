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

Scrollpad::Scrollpad(const Scrollpad &s) : Window(s)
{
	itsContent = s.itsContent;
	itsRawContent = s.itsRawContent;
	itsBeginning = s.itsBeginning;
	itsRealHeight = s.itsRealHeight;
	itsXPos = s.itsXPos;
}

void Scrollpad::Add(string str)
{
	if (itsXPos > 0 && (str[0] != ' ' || str[0] != '\n'))
		str = " " + str;
	
	itsRawContent += str;
	
#ifdef UTF8_ENABLED
	wstring s = ToWString(str);
	wstring tmp;
#else
	string &s = str;
	string tmp;
#endif
	
	int x_pos = 0;
	int space_pos = 0;
	int tab_size;
	bool collect = 0;
	
	for (size_t i = 0; i < s.length(); i++)
	{
		tab_size = 8-itsXPos%8;
		
		if (s[i] != '\t')
		{
#			ifdef UTF8_ENABLED
			itsXPos += wcwidth(s[i]);
#			else
			itsXPos++;
#			endif
		}
		else
			itsXPos += tab_size;
		
		if (BBEnabled)
		{
			if (s[i] == '[' && (s[i+1] == '.' || s[i+1] == '/'))
				collect = 1;
			
			if (collect)
			{
				if (s[i] != '[')
				{
					tmp += s[i];
					if (tmp.length() > 10) // the longest bbcode is 10 chars long
						collect = 0;
				}
				else
					tmp = s[i];
			}
			
			if (s[i] == ']')
				collect = 0;
			
			if (!collect && !tmp.empty())
			{
				if (IsValidColor(TO_STRING(tmp)))
					itsXPos -= tmp.length();
				tmp.clear();
			}
		}
		
		if (s[i] == ' ') // if space, remember its position;
		{
			space_pos = i;
			x_pos = itsXPos;
		}
		
		if (!collect && itsXPos >= itsWidth)
		{
			// if line is over, there was at least one space in this line and we are in the middle of the word, restore position to last known space and make it EOL
			if (space_pos > 0 && (s[i] != ' ' || s[i+1] != ' '))
			{
				i = space_pos;
				itsXPos = x_pos;
				s[i] = '\n';
			}
		}
		
		if (!collect && (itsXPos >= itsWidth || s[i] == '\n'))
		{
			itsRealHeight++;
			itsXPos = 0;
			space_pos = 0;
		}
	}
	itsContent += TO_STRING(s);
	Recreate();
}

void Scrollpad::Recreate()
{
	delwin(itsWindow);
	itsWindow = newpad(itsRealHeight, itsWidth);
	SetTimeout(itsWindowTimeout);
	SetColor(itsBaseColor, itsBgColor);
	Write(itsContent.c_str());
}

void Scrollpad::Refresh(bool)
{
	prefresh(itsWindow,itsBeginning,0,itsStartY,itsStartX,itsStartY+itsHeight-1,itsStartX+itsWidth);
}

void Scrollpad::Resize(int width, int height)
{
	if (itsBorder != brNone)
	{
		delwin(itsWinBorder);
		itsWinBorder = newpad(height, width);
		wattron(itsWinBorder,COLOR_PAIR(itsBorder));
		box(itsWinBorder,0,0);
		width -= 2;
		height -= 2;
	}
	if (!itsTitle.empty())
		width -= 2;
	
	if (height > 0 && width > 0)
	{
		itsHeight = height;
		itsWidth = width;
		
		itsBeginning = 0;
		itsRealHeight = 1;
		itsXPos = 0;
		itsContent.clear();
		string tmp = itsRawContent;
		itsRawContent.clear();
		Add(tmp);
		Recreate();
	}
}

void Scrollpad::Go(Where where)
{
	int MaxBeginning = itsContent.size() < itsHeight ? 0 : itsRealHeight-itsHeight;
	
	switch (where)
	{
		case wUp:
		{
			if (itsBeginning > 0) itsBeginning--; // for scrolling
			break;
		}
		case wDown:
		{
			if (itsBeginning < MaxBeginning) itsBeginning++; // scroll
			break;
		}
		case wPageUp:
		{
			itsBeginning -= itsHeight;
			if (itsBeginning < 0) itsBeginning = 0;
			break;
		}
		case wPageDown:
		{
			itsBeginning += itsHeight;
			if (itsBeginning > MaxBeginning) itsBeginning = MaxBeginning;
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

void Scrollpad::Clear(bool clear_screen)
{
	itsBeginning = 0;
	itsRealHeight = 1;
	itsXPos = 0;
	itsContent.clear();
	itsRawContent.clear();
	wclear(itsWindow);
	delwin(itsWindow);
	itsWindow = newpad(itsHeight, itsWidth);
	SetColor(itsColor, itsBgColor);
	SetTimeout(itsWindowTimeout);
	if (clear_screen)
		Window::Clear();
}

Window * Scrollpad::EmptyClone() const
{
	return new Scrollpad(GetStartX(),GetStartY(),GetWidth(),GetHeight(),itsTitle,itsBaseColor,itsBorder);
}

