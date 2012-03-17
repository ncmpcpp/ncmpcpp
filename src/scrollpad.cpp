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

#include <cassert>

#include "scrollpad.h"

using namespace NCurses;

Scrollpad::Scrollpad(size_t startx,
			size_t starty,
			size_t width,
			size_t height,
			const std::string &title,
			Color color,
			Border border)
			: Window(startx, starty, width, height, title, color, border),
			itsBeginning(0),
			itsFoundValueBegin(-1),
			itsFoundValueEnd(-1),
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
	
	for (size_t i = 0; i < s.length(); ++i)
	{
		x += s[i] != '\t' ? wcwidth(s[i]) : 8-x%8; // tab size
		
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
	itsRealHeight = std::max(itsHeight, itsRealHeight);
	Recreate(itsWidth, itsRealHeight);
	itsBuffer.SetTemp(&s);
	static_cast<Window &>(*this) << itsBuffer;
	itsBuffer.SetTemp(0);
}

bool Scrollpad::SetFormatting(short val_b, const std::basic_string<my_char_t> &s, short val_e, bool case_sensitive, bool for_each)
{
	bool result = itsBuffer.SetFormatting(val_b, s, val_e, case_sensitive, for_each);
	if (result)
	{
		itsFoundForEach = for_each;
		itsFoundCaseSensitive = case_sensitive;
		itsFoundValueBegin = val_b;
		itsFoundValueEnd = val_e;
		itsFoundPattern = s;
	}
	else
		ForgetFormatting();
	return result;
}

void Scrollpad::ForgetFormatting()
{
	itsFoundValueBegin = -1;
	itsFoundValueEnd = -1;
	itsFoundPattern.clear();
}

void Scrollpad::RemoveFormatting()
{
	if (itsFoundValueBegin >= 0 && itsFoundValueEnd >= 0)
		itsBuffer.RemoveFormatting(itsFoundValueBegin, itsFoundPattern, itsFoundValueEnd, itsFoundCaseSensitive, itsFoundForEach);
}

void Scrollpad::Refresh()
{
	int MaxBeginning = itsRealHeight-itsHeight;
	assert(MaxBeginning >= 0);
	if (itsBeginning > MaxBeginning)
		itsBeginning = MaxBeginning;
	prefresh(itsWindow, itsBeginning, 0, itsStartY, itsStartX, itsStartY+itsHeight-1, itsStartX+itsWidth-1);
}

void Scrollpad::Resize(size_t new_width, size_t new_height)
{
	AdjustDimensions(new_width, new_height);
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

void Scrollpad::Clear()
{
	itsRealHeight = itsHeight;
	itsBuffer.Clear();
	wclear(itsWindow);
	delwin(itsWindow);
	itsWindow = newpad(itsHeight, itsWidth);
	SetTimeout(itsWindowTimeout);
	SetColor(itsColor, itsBgColor);
	ForgetFormatting();
	keypad(itsWindow, 1);
}

void Scrollpad::Reset()
{
	itsBeginning = 0;
}

#ifdef _UTF8
Scrollpad &Scrollpad::operator<<(const std::string &s)
{
	itsBuffer << ToWString(s);
	return *this;
}
#endif // _UTF8

