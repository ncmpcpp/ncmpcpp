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
	bool collect = 0;
	
	for (size_t i = 0; i < s.length(); i++)
	{
		if (s[i] != '\t')
			itsXPos++;
		else
			itsXPos += 8;
		
		if (BBEnabled)
		{
			if (s[i] == '[')
				collect = 1;
		
			if (collect)
				tmp += s[i];
		
			if (s[i] == ']' || tmp.length() > 10) // the longest bbcode is 10 chars long
				collect = 0;
		
			if (!collect && !tmp.empty())
			{
				if (is_valid_color(TO_STRING(tmp)))
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
	recreate_win();
}

void Scrollpad::recreate_win()
{
	delwin(itsWindow);
	itsWindow = newpad(itsRealHeight, itsWidth);
	SetColor(itsBaseColor, itsBgColor);
	Write(itsContent.c_str());
}

void Scrollpad::Display(bool stub)
{
	Window::show_border();
	Refresh(stub);
}

void Scrollpad::Refresh(bool stub)
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
		recreate_win();
	}
}

void Scrollpad::Go(WHERE where)
{
	int MaxBeginning;
	
	if (itsContent.size() < itsHeight)
		MaxBeginning = 0;
	else
		MaxBeginning = itsRealHeight-itsHeight;
	
	switch (where)
	{
		case UP:
		{
			if (itsBeginning > 0) itsBeginning--; // for scrolling
			break;
		}
		case DOWN:
		{
			if (itsBeginning < MaxBeginning) itsBeginning++; // scroll
			break;
		}
		case PAGE_UP:
		{
			itsBeginning -= itsHeight;
			if (itsBeginning < 0) itsBeginning = 0;
			break;
		}
		case PAGE_DOWN:
		{
			itsBeginning += itsHeight;
			if (itsBeginning > MaxBeginning) itsBeginning = MaxBeginning;
			break;
		}
		case HOME:
		{
			itsBeginning = 0;
			break;
		}
		case END:
		{
			itsBeginning = MaxBeginning;
			break;
		}
	}
}

void Scrollpad::SetBorder(BORDER border)
{
	if (have_to_recreate(border))
		recreate_win();
	itsBorder = border;
}

void Scrollpad::SetTitle(string newtitle)
{
	if (have_to_recreate(newtitle))
		recreate_win();
	itsTitle = newtitle;
}

void Scrollpad::Clear()
{
	itsBeginning = 0;
	itsRealHeight = 1;
	itsXPos = 0;
	itsContent.clear();
	itsRawContent.clear();
	delwin(itsWindow);
	itsWindow = newpad(itsRealHeight, itsWidth);
	SetColor(itsColor, itsBgColor);
	Window::Clear();
}

Scrollpad Scrollpad::EmptyClone()
{
	return Scrollpad(GetStartX(),GetStartY(),GetWidth(),GetHeight(),itsTitle,itsBaseColor,itsBorder);
}

Scrollpad & Scrollpad::operator=(const Scrollpad &base)
{
	if (this == &base)
		return *this;
	itsWindow = base.itsWindow;
	itsStartX = base.itsStartX;
	itsStartY = base.itsStartY;
	itsWidth = base.itsWidth;
	itsHeight = base.itsHeight;
	itsTitle = base.itsTitle;
	itsBorder = base.itsBorder;
	itsColor = base.itsColor;
	itsContent = base.itsContent;
	itsBeginning = base.itsBeginning;
	itsRealHeight = base.itsRealHeight;
	return *this;
}
