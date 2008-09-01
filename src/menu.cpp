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

Menu::Menu(const Menu &m) : Window(m)
{
	for (vector<Option *>::const_iterator it = m.itsOptions.begin(); it != m.itsOptions.end(); it++)
	{
		Option *new_option = new Option(**it);
		itsOptions.push_back(new_option);
	}
	NeedsRedraw = m.NeedsRedraw;
	itsSelectedPrefix = m.itsSelectedPrefix;
	itsSelectedSuffix = m.itsSelectedSuffix;
	itsStaticsNumber = m.itsStaticsNumber;
	itsBeginning = m.itsBeginning;
	itsHighlight = m.itsHighlight;
	itsHighlightColor = m.itsHighlightColor;
	itsHighlightEnabled = m.itsHighlightEnabled;
}

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
	no--;
	try
	{
		if (itsOptions.at(no)->is_static)
			itsStaticsNumber--;
	}
	catch (std::out_of_range)
	{
		return;
	}
	delete itsOptions[no];
	itsOptions.erase(itsOptions.begin()+no);
	
	if (itsHighlight > itsOptions.size()-1)
		itsHighlight = itsOptions.size()-1;
	
	idlok(itsWindow, 1);
	scrollok(itsWindow, 1);
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	if (itsBeginning > MaxBeginning)
	{
		itsBeginning = MaxBeginning;
		wmove(itsWindow, no-itsBeginning, 0);
		wdeleteln(itsWindow);
		wscrl(itsWindow, -1);
		NeedsRedraw.push_back(itsBeginning);
	}
	else
	{
		wmove(itsWindow, no-itsBeginning, 0);
		wdeleteln(itsWindow);
	}
	NeedsRedraw.push_back(itsHighlight);
	NeedsRedraw.push_back(itsBeginning+itsHeight-1);
	scrollok(itsWindow, 0);
	idlok(itsWindow, 0);
}

void Menu::Swap(int one, int two)
{
	try
	{
		std::swap<Option *>(itsOptions.at(one), itsOptions.at(two));
		NeedsRedraw.push_back(one);
		NeedsRedraw.push_back(two);
	}
	catch (std::out_of_range)
	{
	}
}

void Menu::redraw_screen()
{
	NeedsRedraw.clear();
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
		itsHighlight == 0 ? Go(DOWN) : Go(UP);
	
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	if (itsBeginning > MaxBeginning)
		itsBeginning = MaxBeginning;
	
	if (itsHighlight >= itsOptions.size()-1)
		Highlight(itsOptions.size());
	
	if (redraw_whole_window)
	{
		Window::Clear();
		redraw_screen();
	}
	
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
		
		int line = *it-itsBeginning;
		
		if (line < 0 || line+1 > itsHeight) // do not draw if line should be invisible anyway
			continue;
		
		COLOR old_basecolor = itsBaseColor;
		
		if (*it == itsHighlight && itsHighlightEnabled)
		{
			Reverse(1);
			SetBaseColor(itsHighlightColor);
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
			if (itsOptions[*it]->selected)
				WriteXY(x, line, itsWidth, ToWString(itsSelectedPrefix + itsOptions[*it]->content + itsSelectedSuffix), 0);
			else
				WriteXY(x, line, itsWidth, ToWString(itsOptions[*it]->content), 0);
#			else
			if (itsOptions[*it]->selected)
				WriteXY(x, line, itsWidth, itsSelectedPrefix + itsOptions[*it]->content + itsSelectedSuffix, 0);
			else
				WriteXY(x, line, itsWidth, itsOptions[*it]->content, 0);
#			endif
			
			if (!ch && (itsOptions[*it]->location == lCenter || itsOptions[*it]->location == lLeft))
			{
				x += strlength;
				AltCharset(1);
				mvwaddstr(itsWindow, line, x, " t");
				AltCharset(0);
			}
		}
		
		SetBaseColor(old_basecolor);
		SetColor(old_basecolor);
		while (!itsColors.empty()) // clear color stack and disable bold and reverse as
			itsColors.pop();   // some items are too long to close all tags properly
		Reverse(0);
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
	which--;
	
	int old_highlight = itsHighlight;
	int old_beginning = itsBeginning;
	
	if (which < itsOptions.size())
		itsHighlight = which;
	else
		return;
	
	if (which >= itsHeight/2 && itsOptions.size() > itsHeight)
	{
		itsBeginning = itsHighlight-itsHeight/2;
		if (itsBeginning > itsOptions.size()-itsHeight)
			itsBeginning = itsOptions.size()-itsHeight;
	}
	else
		itsBeginning = 0;
	
	int howmuch = itsBeginning-old_beginning;
	
	if (Abs(howmuch) > itsHeight)
		redraw_screen();
	else
	{
		idlok(itsWindow, 1);
		scrollok(itsWindow, 1);
		if (old_highlight >= itsBeginning && old_highlight < itsBeginning+itsHeight)
			NeedsRedraw.push_back(old_highlight);
		wscrl(itsWindow, howmuch);
		if (howmuch < 0)
			for (int i = 0; i < Abs(howmuch); i++)
				NeedsRedraw.push_back(itsBeginning+i);
		else
			for (int i = 0; i < Abs(howmuch); i++)
				NeedsRedraw.push_back(itsBeginning+itsHeight-1-i);
		NeedsRedraw.push_back(itsHighlight);
		scrollok(itsWindow, 0);
		idlok(itsWindow, 0);
	}
}

void Menu::Reset()
{
	NeedsRedraw.clear();
	itsHighlight = 0;
	itsBeginning = 0;
}

void Menu::Clear(bool clear_screen)
{
	for (vector<Option *>::iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
	itsOptions.clear();
	NeedsRedraw.clear();
	itsStaticsNumber = 0;
	if (clear_screen)
		Window::Clear();
}

void Menu::Select(int option, bool selected)
{
	option--;
	try
	{
		if (itsOptions.at(option)->selected != selected)
			NeedsRedraw.push_back(option);
		itsOptions.at(option)->selected = selected;
	}
	catch (std::out_of_range)
	{
	}
}

bool Menu::Selected(int option)
{
	option--;
	try
	{
		return itsOptions.at(option)->selected;
	}
	catch (std::out_of_range)
	{
		return false;
	}
}

bool Menu::IsAnySelected()
{
	bool result = 0;
	for (vector<Option *>::const_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
	{
		if ((*it)->selected)
		{
			result = 1;
			break;
		}
	}
	return result;
}

void Menu::GetSelectedList(vector<int> &v)
{
	int i = 1;
	for (vector<Option *>::const_iterator it = itsOptions.begin(); it != itsOptions.end(); it++, i++)
		if ((*it)->selected)
			v.push_back(i);
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

Window * Menu::EmptyClone()
{
	return new Menu(GetStartX(),GetStartY(),GetWidth(),GetHeight(),itsTitle,itsBaseColor,itsBorder);
}
