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

#include "menu.h"
#include "misc.h"

int Menu::count_length(string str)
{
	if (str.empty())
		return 0;
	
	bool collect = false;
	int length = 0;
	
#ifdef UTF8_ENABLED
	wstring str2 = ToWString(str);
	wstring tmp;
#else
	string &str2 = str;
	string tmp;
#endif
	
	for (int i = 0; i < str2.length(); i++, length++)
	{
		if (str2[i] == '[')
			collect = 1;
		
		if (collect)
			tmp += str2[i];
		
		if (str2[i] == ']')
			collect = 0;
		
		if (!collect && !tmp.empty())
		{
			if (is_valid_color(TO_STRING(tmp)))
				length -= tmp.length();
			tmp.clear();
		}
	}
	return length;
}

void Menu::AddOption(const string &str, LOCATION location, HAVE_SEPARATOR separator)
{
	itsLocations.push_back(location);
	itsOptions.push_back(str);
	itsSeparators.push_back(separator);
	itsStaticOptions.push_back(0);
	itsBold.push_back(0);
}

void Menu::AddBoldOption(const string &str, LOCATION location, HAVE_SEPARATOR separator)
{
	itsLocations.push_back(location);
	itsOptions.push_back(str);
	itsSeparators.push_back(separator);
	itsStaticOptions.push_back(0);
	itsBold.push_back(1);
}

void Menu::AddStaticOption(const string &str, LOCATION location, HAVE_SEPARATOR separator)
{
	itsLocations.push_back(location);
	itsOptions.push_back(str);
	itsSeparators.push_back(separator);
	itsStaticOptions.push_back(1);
	itsBold.push_back(0);
	itsStaticsNumber++;
}

void Menu::AddStaticBoldOption(const string &str, LOCATION location, HAVE_SEPARATOR separator)
{
	itsLocations.push_back(location);
	itsOptions.push_back(str);
	itsSeparators.push_back(separator);
	itsStaticOptions.push_back(1);
	itsBold.push_back(1);
	itsStaticsNumber++;
}

void Menu::AddSeparator()
{
	AddStaticOption("", lLeft, 1);
}

void Menu::UpdateOption(int index, string str, LOCATION location, HAVE_SEPARATOR separator)
{
	try
	{
		itsLocations.at(index-1) = location;
		itsOptions.at(index-1) = str;
		itsSeparators.at(index-1) = separator;
	}
	catch (std::out_of_range)
	{
	}
}

void Menu::BoldOption(int index, IS_BOLD bold)
{
	try
	{
		itsBold.at(index-1) = bold;
	}
	catch (std::out_of_range)
	{
	}
}

void Menu::MakeStatic(int index, IS_STATIC stat)
{
	try
	{
		if (stat && !itsStaticOptions.at(index-1))
			itsStaticsNumber++;
		if (!stat && itsStaticOptions.at(index-1))
			itsStaticsNumber--;
		itsStaticOptions.at(index-1) = stat;
	}
	catch (std::out_of_range)
	{
	}
}

void Menu::DeleteOption(int no)
{
	try
	{
		if (itsStaticOptions.at(no-1))
			itsStaticsNumber--;
	}
	catch (std::out_of_range)
	{
		return;
	}
	itsLocations.erase(itsLocations.begin()+no-1);
	itsOptions.erase(itsOptions.begin()+no-1);
	itsStaticOptions.erase(itsStaticOptions.begin()+no-1);
	itsSeparators.erase(itsSeparators.begin()+no-1);
	itsBold.erase(itsBold.begin()+no-1);
	/*if (itsBeginning > 0 && itsBeginning == itsOptions.size()-itsHeight)
		itsBeginning--;
	Go(UP);*/
	Window::Clear();
}

void Menu::Display()
{
	Window::show_border();
	Refresh();
}

void Menu::Refresh()
{
	if (!itsOptions.empty() && is_static())
		if (itsHighlight == 0)
			Go(DOWN);
		else
			Go(UP);
	
	if (MaxChoice() < GetChoice() && !Empty())
		Highlight(MaxChoice());
	
	if (itsHighlight > itsBeginning+itsHeight)
		Highlight(itsBeginning+itsHeight);
	
	int line = 0;
	int last;
	
	if (itsOptions.size() < itsHeight)
		last = itsOptions.size();
	else
		last = itsBeginning+itsHeight;
	
	int check = 0;
	bool next = 1;
	for (int i = last-1; i > itsBeginning && next; i--)
	{
		next = 0;
		try
		{
			itsSeparators.at(i);
		}
		catch (std::out_of_range)
		{
			check++;
			next = 1;
		}
	}
	itsBeginning -= check;
	last -= check;
	
	for (int i = itsBeginning; i < last; i++)
	{
		if (i == itsHighlight && itsHighlightEnabled)
			Reverse(1);
		if (itsBold[i])
			Bold(1);
		
		int ch = itsSeparators[i] ? 0 : 32;
		mvwhline(itsWindow,line, 0, ch, itsWidth);
		
		int strlength = itsLocations[i] != lLeft && BBEnabled ? count_length(itsOptions[i]) : itsOptions[i].length();
		
		if (strlength)
		{
			int x = 0;
			
			if (itsLocations[i] == lCenter)
			{
				for (; x < (itsWidth-strlength-(!ch ? 4 : 0))/2; x++);
				if (!ch)
				{
					AltCharset(1);
					mvwaddstr(itsWindow, line, x, "u ");
					AltCharset(0);
					x += 2;
				}
			}
			if (itsLocations[i] == lRight)
			{
				for (; x < (itsWidth-strlength); x++)
				if (!ch)
				{
					AltCharset(1);
					mvwaddstr(itsWindow, line, x, "u ");
					AltCharset(0);
					x += 2;
				}
			}
			
			WriteXY(x, line, itsOptions[i], 0);
			
			if (!ch && (itsLocations[i] == lCenter || itsLocations[i] == lLeft))
			{
				x += strlength;
				AltCharset(1);
				mvwaddstr(itsWindow, line, x, " t");
				AltCharset(0);
			}
		}
		line++;
		
		if (i == itsHighlight && itsHighlightEnabled)
			Reverse(0);
		if (itsBold[i])
			Bold(0);
	}
	wrefresh(itsWindow);
}

void Menu::Go(WHERE where)
{
	if (Empty()) return;
	int MaxHighlight = itsOptions.size()-1;
	int MaxBeginning;
	if (itsOptions.size() < itsHeight)
		MaxBeginning = 0;
	else MaxBeginning = itsOptions.size()-itsHeight;
	int MaxCurrentHighlight = itsBeginning+itsHeight-1;
	switch (where)
	{
		case UP:
		{
			if (itsHighlight <= itsBeginning && itsHighlight > 0) itsBeginning--; // for scrolling
			if (itsHighlight == 0)
				break;
			else
				itsHighlight--;
			if (is_static())
			{
				if (itsHighlight == 0)
					Go(DOWN);
				else
					Go(UP);
			}
			break;
		}
		case DOWN:
		{
			if (itsHighlight >= MaxCurrentHighlight && itsHighlight < MaxHighlight) itsBeginning++; // scroll
			if (itsHighlight == MaxHighlight)
				break;
			else
				itsHighlight++;
			if (is_static())
			{
				if (itsHighlight == MaxHighlight)
					Go(UP);
				else
					Go(DOWN);
			}
			break;
		}
		case PAGE_UP:
		{
			itsHighlight -= itsHeight;
			itsBeginning -= itsHeight;
			if (itsBeginning < 0)
			{
				itsBeginning = 0;
				if (itsHighlight < 0) itsHighlight = 0;
			}
			if (is_static())
			{
				if (itsHighlight == 0)
					Go(DOWN);
				else
					Go(UP);
			}
			break;
		}
		case PAGE_DOWN:
		{
			itsHighlight += itsHeight;
			itsBeginning += itsHeight;
			if (itsBeginning > MaxBeginning)
			{
				itsBeginning = MaxBeginning;
				if (itsHighlight > MaxHighlight) itsHighlight = MaxHighlight;
			}
			if (is_static())
			{
				if (itsHighlight == MaxHighlight)
					Go(UP);
				else
					Go(DOWN);
			}
			break;
		}
		case HOME:
		{
			itsHighlight = 0;
			itsBeginning = 0;
			if (is_static())
			{
				if (itsHighlight == 0)
					Go(DOWN);
				else
					Go(UP);
			}
			break;
		}
		case END:
		{
			itsHighlight = MaxHighlight;
			itsBeginning = MaxBeginning;
			if (is_static())
			{
				if (itsHighlight == MaxHighlight)
					Go(UP);
				else
					Go(DOWN);
			}
			break;
		}
	}
}

void Menu::Highlight(int which)
{
	if (which <= itsOptions.size())
		itsHighlight = which-1;
	
	if (which > itsHeight)
		itsBeginning = itsHighlight-itsHeight/2;
	else
		itsBeginning = 0;
}

void Menu::Reset()
{
	itsHighlight = 0;
	itsBeginning = 0;
}

void Menu::Clear(bool clear_screen)
{
	itsOptions.clear();
	itsStaticOptions.clear();
	itsSeparators.clear();
	itsLocations.clear();
	itsBold.clear();
	itsStaticsNumber = 0;
	if (clear_screen)
		Window::Clear();
}

int Menu::GetRealChoice() const
{
	int real_choice	= 0;
	vector<IS_STATIC>::const_iterator it = itsStaticOptions.begin();
	for (int i = 0; i < itsHighlight; it++, i++)
		if (!*it) real_choice++;
	
	return real_choice+1;
}

Menu Menu::EmptyClone()
{
	return Menu(GetStartX(),GetStartY(),GetWidth(),GetHeight(),itsTitle,itsBaseColor,itsBorder);
}
