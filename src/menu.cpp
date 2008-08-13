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

Menu::~Menu()
{
	for (vector<Option *>::iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
}

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
	Option *new_option = new Option;
	new_option->content = str;
	new_option->location = location;
	new_option->have_separator = separator;
	new_option->is_static = 0;
	new_option->is_bold = 0;
	itsOptions.push_back(new_option);
	
	if (itsOptions.size() > itsBeginning && itsOptions.size() <= itsBeginning+itsHeight)
		NeedsRedraw.push_back(itsOptions.size()-1);
}

void Menu::AddBoldOption(const string &str, LOCATION location, HAVE_SEPARATOR separator)
{
	Option *new_option = new Option;
	new_option->content = str;
	new_option->location = location;
	new_option->have_separator = separator;
	new_option->is_static = 0;
	new_option->is_bold = 1;
	itsOptions.push_back(new_option);
	
	if (itsOptions.size() > itsBeginning && itsOptions.size() <= itsBeginning+itsHeight)
		NeedsRedraw.push_back(itsOptions.size()-1);
}

void Menu::AddStaticOption(const string &str, LOCATION location, HAVE_SEPARATOR separator)
{
	Option *new_option = new Option;
	new_option->content = str;
	new_option->location = location;
	new_option->have_separator = separator;
	new_option->is_static = 1;
	new_option->is_bold = 0;
	itsOptions.push_back(new_option);
	itsStaticsNumber++;
	
	if (itsOptions.size() > itsBeginning && itsOptions.size() <= itsBeginning+itsHeight)
		NeedsRedraw.push_back(itsOptions.size()-1);
}

void Menu::AddStaticBoldOption(const string &str, LOCATION location, HAVE_SEPARATOR separator)
{
	Option *new_option = new Option;
	new_option->content = str;
	new_option->location = location;
	new_option->have_separator = separator;
	new_option->is_static = 1;
	new_option->is_bold = 1;
	itsOptions.push_back(new_option);
	itsStaticsNumber++;
	
	if (itsOptions.size() > itsBeginning && itsOptions.size() <= itsBeginning+itsHeight)
		NeedsRedraw.push_back(itsOptions.size()-1);
}

void Menu::AddSeparator()
{
	AddStaticOption("", lLeft, 1);
}

void Menu::UpdateOption(int index, string str, LOCATION location, HAVE_SEPARATOR separator)
{
	index--;
	try
	{
		itsOptions.at(index)->location = location;
		itsOptions.at(index)->content = str;
		itsOptions.at(index)->have_separator = separator;
		if (index >= itsBeginning && index < itsBeginning+itsHeight)
			NeedsRedraw.push_back(index);
	}
	catch (std::out_of_range)
	{
	}
}

void Menu::BoldOption(int index, IS_BOLD bold)
{
	index--;
	try
	{
		itsOptions.at(index)->is_bold = bold;
		if (index >= itsBeginning && index < itsBeginning+itsHeight)
			NeedsRedraw.push_back(index);
	}
	catch (std::out_of_range)
	{
	}
}

void Menu::MakeStatic(int index, IS_STATIC stat)
{
	index--;
	try
	{
		if (stat && !itsOptions.at(index)->is_static)
			itsStaticsNumber++;
		if (!stat && itsOptions.at(index)->is_static)
			itsStaticsNumber--;
		itsOptions.at(index)->is_static = stat;
	}
	catch (std::out_of_range)
	{
	}
}

string Menu::GetCurrentOption() const
{
	try
	{
		return itsOptions.at(itsHighlight)->content;
	}
	catch (std::out_of_range)
	{
		return "";
	}
}

string Menu::GetOption(int i) const
{
	try
	{
		return itsOptions.at(i-1)->content;
	}
	catch (std::out_of_range)
	{
		return "";
	}
}

void Menu::DeleteOption(int no)
{
	try
	{
		if (itsOptions.at(no-1)->is_static)
			itsStaticsNumber--;
	}
	catch (std::out_of_range)
	{
		return;
	}
	delete itsOptions[no-1];
	itsOptions.erase(itsOptions.begin()+no-1);
	
	if (itsHighlight > itsOptions.size()-1)
		itsHighlight = itsOptions.size()-1;
	
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	if (itsBeginning > MaxBeginning)
	{
		itsBeginning = MaxBeginning;
		NeedsRedraw.push_back(itsHighlight);
		Refresh();
		redraw_screen();
	}
	else
	{
		vector<Option *>::const_iterator it = itsOptions.begin()+itsHighlight;
		NeedsRedraw.reserve(itsHeight);
		int i = itsHighlight;
		for (; i < itsBeginning+itsHeight && it != itsOptions.end(); i++, it++)
			NeedsRedraw.push_back(i);
		for (; i < itsBeginning+itsHeight; i++)
			mvwhline(itsWindow, i, 0, 32, itsWidth);
	}
	
	/*if (itsBeginning > 0 && itsBeginning == itsOptions.size()-itsHeight)
		itsBeginning--;
	Go(UP);*/
	//Window::Clear();
}

void Menu::redraw_screen()
{
	vector<Option *>::const_iterator it = itsOptions.begin()+itsBeginning;
	NeedsRedraw.reserve(itsHeight);
	for (int i = itsBeginning; i < itsBeginning+itsHeight && it != itsOptions.end(); i++, it++)
		NeedsRedraw.push_back(i);
}

void Menu::Display(bool redraw_whole_window)
{
	Window::show_border();
	Refresh(redraw_whole_window);
}

void Menu::Refresh(bool redraw_whole_window)
{
	if (!itsOptions.empty() && is_static())
		if (itsHighlight == 0)
			Go(DOWN);
		else
			Go(UP);
	
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	if (itsBeginning > MaxBeginning)
		itsBeginning = MaxBeginning;
	
	if (itsHighlight > itsOptions.size()-1)
		Highlight(itsOptions.size());
	
	if (redraw_whole_window)
		redraw_screen();
	
	int line = 0;
	/*int last;
	
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
			itsOptions.at(i);
		}
		catch (std::out_of_range)
		{
			check++;
			next = 1;
		}
	}
	itsBeginning -= check;
	last -= check;*/
	
	for (vector<int>::const_iterator it = NeedsRedraw.begin(); it != NeedsRedraw.end(); it++)
	{
		try
		{
			itsOptions.at(*it);
		}
		catch (std::out_of_range)
		{
			break;
		}
		
		line = *it-itsBeginning;
		
		if (*it == itsHighlight && itsHighlightEnabled)
		{
			Reverse(1);
			SetColor(itsHighlightColor);
		}
		if (itsOptions[*it]->is_bold)
			Bold(1);
		
		int ch = itsOptions[*it]->have_separator ? 0 : 32;
		mvwhline(itsWindow,line, 0, ch, itsWidth);
		
		int strlength = itsOptions[*it]->location != lLeft && BBEnabled ? count_length(itsOptions[*it]->content) : itsOptions[*it]->content.length();
		
		if (strlength)
		{
			int x = 0;
			
			if (itsOptions[*it]->location == lCenter)
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
			if (itsOptions[*it]->location == lRight)
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
			
#			ifdef UTF8_ENABLED
			wstring option = ToWString(itsOptions[*it]->content);
			int bbcodes_length = CountBBCodes(option);
			WriteXY(x, line, option.substr(0, itsWidth+bbcodes_length), 0);
#			else
			int bbcodes_length = CountBBCodes(itsOptions[*it]->content);
			WriteXY(x, line, itsOptions[*it]->content.substr(0, itsWidth+bbcodes_length), 0);
#			endif
			
			if (!ch && (itsOptions[*it]->location == lCenter || itsOptions[*it]->location == lLeft))
			{
				x += strlength;
				AltCharset(1);
				mvwaddstr(itsWindow, line, x, " t");
				AltCharset(0);
			}
		}
		line++;
		
		if (*it == itsHighlight && itsHighlightEnabled)
		{
			Reverse(0);
			SetColor(itsBaseColor);
		}
		if (itsOptions[*it]->is_bold)
			Bold(0);
	}
	NeedsRedraw.clear();
	wrefresh(itsWindow);
}

void Menu::Go(WHERE where)
{
	if (Empty()) return;
	int MaxHighlight = itsOptions.size()-1;
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	int MaxCurrentHighlight = itsBeginning+itsHeight-1;
	idlok(itsWindow, 1);
	scrollok(itsWindow, 1);
	switch (where)
	{
		case UP:
		{
			if (itsHighlight <= itsBeginning && itsHighlight > 0)
			{
				itsBeginning--; // for scrolling
				wscrl(itsWindow, -1);
			}
			if (itsHighlight == 0)
				break;
			else
			{
				NeedsRedraw.push_back(itsHighlight--);
				NeedsRedraw.push_back(itsHighlight);
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
		case DOWN:
		{
			if (itsHighlight >= MaxCurrentHighlight && itsHighlight < MaxHighlight)
			{
				itsBeginning++; // scroll
				wscrl(itsWindow, 1);
			}
			if (itsHighlight == MaxHighlight)
				break;
			else
			{
				NeedsRedraw.push_back(itsHighlight++);
				NeedsRedraw.push_back(itsHighlight);
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
			redraw_screen();
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
			redraw_screen();
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
			redraw_screen();
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
			redraw_screen();
			break;
		}
	}
	idlok(itsWindow, 0);
	scrollok(itsWindow, 0);
}

void Menu::Highlight(int which)
{
	if (which <= itsOptions.size())
	{
		NeedsRedraw.push_back(itsHighlight);
		itsHighlight = which-1;
		NeedsRedraw.push_back(itsHighlight);
	}
	else
		return;
	
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
	for (vector<Option *>::iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
	itsOptions.clear();
	itsStaticsNumber = 0;
	if (clear_screen)
		Window::Clear();
}

int Menu::GetRealChoice() const
{
	int real_choice	= 0;
	vector<Option *>::const_iterator it = itsOptions.begin();
	for (int i = 0; i < itsHighlight; it++, i++)
		if (!(*it)->is_static) real_choice++;
	
	return real_choice+1;
}

bool Menu::IsStatic(int option)
{
	try
	{
		return itsOptions.at(option-1)->is_static;
	}
	catch (std::out_of_range)
	{
		return 1;
	}
}

Menu Menu::EmptyClone()
{
	return Menu(GetStartX(),GetStartY(),GetWidth(),GetHeight(),itsTitle,itsBaseColor,itsBorder);
}
