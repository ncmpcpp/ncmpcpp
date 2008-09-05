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

#ifndef HAVE_MENU_H
#define HAVE_MENU_H

#include "window.h"
#include "misc.h"

#include <stdexcept>

enum Location { lLeft, lCenter, lRight };

template <class T>
struct Option
{
	Option() : is_static(0), is_bold(0), selected(0), have_separator(0) { }
	T item;
	bool is_static;
	bool is_bold;
	bool selected;
	bool have_separator;
	Location location;
};

template <class T>
class Menu : public Window
{
	typedef typename vector<Option<T> *>::iterator T_iterator;
	typedef typename vector<Option<T> *>::const_iterator T_const_iterator;
	typedef string (*ItemDisplayer) (const T &, void *);
	
	public:
		Menu(int startx, int starty, int width, int height, string title, Color color, Border border) : itsItemDisplayer(0), itsItemDisplayerUserdata(0), Window(startx, starty, width, height, title, color, border), itsSelectedPrefix("[.r]"), itsSelectedSuffix("[/r]"), itsStaticsNumber(0), itsBeginning(0), itsHighlight(0), itsHighlightColor(itsBaseColor), itsHighlightEnabled(1) { SetColor(color); }
		Menu(const Menu &);
		virtual ~Menu();
		
		void SetItemDisplayer(ItemDisplayer ptr) { itsItemDisplayer = ptr; }
		void SetItemDisplayerUserData(void *data) { itsItemDisplayerUserdata = data; }
		
		void AddOption(const T &, Location = lLeft, bool separator = 0);
		void AddBoldOption(const T &item, Location location = lLeft, bool separator = 0);
		void AddStaticOption(const T &item, Location location = lLeft, bool separator = 0);
		void AddStaticBoldOption(const T &item, Location location = lLeft, bool separator = 0);
		void AddSeparator();
		void UpdateOption(int, const T &, Location = lLeft, bool separator = 0);
		void BoldOption(int, bool);
		void MakeStatic(int, bool);
		void DeleteOption(int);
		void Swap(int, int);
		virtual string GetOption(int i = -1) const;
		
		virtual void Refresh(bool redraw_whole_window = 0);
		virtual void Go(Where);
		virtual void Highlight(int);
		virtual void Reset();
		virtual void Clear(bool clear_screen = 1);
		
		virtual void Select(int, bool);
		virtual bool Selected(int) const;
		virtual bool IsAnySelected() const;
		virtual void GetSelectedList(vector<int> &) const;
		void SetSelectPrefix(string str) { itsSelectedPrefix = str; }
		void SetSelectSuffix(string str) { itsSelectedSuffix = str; }
		
		void HighlightColor(Color col) { itsHighlightColor = col; NeedsRedraw.push_back(itsHighlight); }
		void Highlighting(bool hl) { itsHighlightEnabled = hl; NeedsRedraw.push_back(itsHighlight); Refresh(); }
		
		int GetRealChoice() const;
		virtual int GetChoice() const { return itsHighlight; }
		virtual int Size() const { return itsOptions.size(); }
		
		bool Empty() const { return itsOptions.empty(); }
		virtual bool IsStatic(int) const;
		virtual Window * Clone() const { return new Menu(*this); }
		virtual Window * EmptyClone() const;
		
		T & at(int i) { return itsOptions.at(i)->item; }
		const T & at(int i) const { return itsOptions.at(i)->item; }
		const T & operator[](int i) const { return itsOptions[i]->item; }
		T & operator[](int i) { return itsOptions[i]->item; }
	protected:
		string DisplayOption(const T &t) const;
		ItemDisplayer itsItemDisplayer;
		void *itsItemDisplayerUserdata;
		
		vector<Option<T> *> itsOptions;
		vector<int> NeedsRedraw;
		
		string itsSelectedPrefix;
		string itsSelectedSuffix;
		
		int itsStaticsNumber;
		int count_length(string);
		
		void redraw_screen();
		bool is_static() { return itsOptions[itsHighlight]->is_static; }
		
		int itsBeginning;
		int itsHighlight;
		
		Color itsHighlightColor;
		
		bool itsHighlightEnabled;
};

template <class T>
Menu<T>::Menu(const Menu &m) : Window(m)
{
	for (T_const_iterator it = m.itsOptions.begin(); it != m.itsOptions.end(); it++)
	{
		Option<T> *new_option = new Option<T>(**it);
		itsOptions.push_back(new_option);
	}
	itsItemDisplayer = m.itsItemDisplayer;
	itsItemDisplayerUserdata = m.itsItemDisplayerUserdata;
	NeedsRedraw = m.NeedsRedraw;
	itsSelectedPrefix = m.itsSelectedPrefix;
	itsSelectedSuffix = m.itsSelectedSuffix;
	itsStaticsNumber = m.itsStaticsNumber;
	itsBeginning = m.itsBeginning;
	itsHighlight = m.itsHighlight;
	itsHighlightColor = m.itsHighlightColor;
	itsHighlightEnabled = m.itsHighlightEnabled;
}

template <class T>
Menu<T>::~Menu()
{
	for (T_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
}

template <class T>
int Menu<T>::count_length(string str)
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
			if (IsValidColor(TO_STRING(tmp)))
				length -= tmp.length();
			tmp.clear();
		}
	}
	return length;
}

template <class T>
void Menu<T>::AddOption(const T &item, Location location, bool separator)
{
	Option<T> *new_option = new Option<T>;
	new_option->item = item;
	new_option->location = location;
	new_option->have_separator = separator;
	itsOptions.push_back(new_option);
	NeedsRedraw.push_back(itsOptions.size()-1);
}

template <class T>
void Menu<T>::AddBoldOption(const T &item, Location location, bool separator)
{
	Option<T> *new_option = new Option<T>;
	new_option->item = item;
	new_option->location = location;
	new_option->have_separator = separator;
	new_option->is_bold = 1;
	itsOptions.push_back(new_option);
	NeedsRedraw.push_back(itsOptions.size()-1);
}

template <class T>
void Menu<T>::AddStaticOption(const T &item, Location location, bool separator)
{
	Option<T> *new_option = new Option<T>;
	new_option->item = item;
	new_option->location = location;
	new_option->have_separator = separator;
	new_option->is_static = 1;
	itsOptions.push_back(new_option);
	itsStaticsNumber++;
	NeedsRedraw.push_back(itsOptions.size()-1);
}

template <class T>
void Menu<T>::AddStaticBoldOption(const T &item, Location location, bool separator)
{
	Option<T> *new_option = new Option<T>;
	new_option->item = item;
	new_option->location = location;
	new_option->have_separator = separator;
	new_option->is_static = 1;
	new_option->is_bold = 1;
	itsOptions.push_back(new_option);
	itsStaticsNumber++;
	NeedsRedraw.push_back(itsOptions.size()-1);
}

template <class T>
void Menu<T>::AddSeparator()
{
	AddStaticOption("", lLeft, 1);
}

template <class T>
void Menu<T>::UpdateOption(int index, const T &item, Location location, bool separator)
{
	try
	{
		itsOptions.at(index)->location = location;
		itsOptions.at(index)->item = item;
		itsOptions.at(index)->have_separator = separator;
		NeedsRedraw.push_back(index);
	}
	catch (std::out_of_range)
	{
	}
}

template <class T>
void Menu<T>::BoldOption(int index, bool bold)
{
	try
	{
		itsOptions.at(index)->is_bold = bold;
		NeedsRedraw.push_back(index);
	}
	catch (std::out_of_range)
	{
	}
}

template <class T>
void Menu<T>::MakeStatic(int index, bool stat)
{
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

template <class T>
string Menu<T>::GetOption(int i) const
{
	try
	{
		return DisplayOption(itsOptions.at(i == -1 ? itsHighlight : i)->item);
	}
	catch (std::out_of_range)
	{
		return "";
	}
}

template <class T>
void Menu<T>::DeleteOption(int no)
{
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

template <class T>
void Menu<T>::Swap(int one, int two)
{
	try
	{
		std::swap<Option<T> *>(itsOptions.at(one), itsOptions.at(two));
		NeedsRedraw.push_back(one);
		NeedsRedraw.push_back(two);
	}
	catch (std::out_of_range)
	{
	}
}

template <class T>
void Menu<T>::redraw_screen()
{
	NeedsRedraw.clear();
	T_const_iterator it = itsOptions.begin()+itsBeginning;
	NeedsRedraw.reserve(itsHeight);
	for (int i = itsBeginning; i < itsBeginning+itsHeight && it != itsOptions.end(); i++, it++)
		NeedsRedraw.push_back(i);
}

template <class T>
void Menu<T>::Refresh(bool redraw_whole_window)
{
	if (!itsOptions.empty() && is_static())
		itsHighlight == 0 ? Go(wDown) : Go(wUp);
	
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	if (itsBeginning > MaxBeginning)
		itsBeginning = MaxBeginning;
	
	if (itsHighlight >= itsOptions.size()-1)
		Highlight(itsOptions.size()-1);
	
	while (itsHighlight-itsBeginning > itsHeight-1)
		itsBeginning++;
	
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
		
		Color old_basecolor = itsBaseColor;
		
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
		
		string option = DisplayOption(itsOptions[*it]->item);
		
		int strlength = itsOptions[*it]->location != lLeft && BBEnabled ? count_length(option) : option.length();
		
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
				WriteXY(x, line, itsWidth, ToWString(itsSelectedPrefix + option + itsSelectedSuffix), 0);
			else
				WriteXY(x, line, itsWidth, ToWString(option), 0);
#			else
			if (itsOptions[*it]->selected)
				WriteXY(x, line, itsWidth, itsSelectedPrefix + option + itsSelectedSuffix, 0);
			else
				WriteXY(x, line, itsWidth, option, 0);
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
		AltCharset(0);
		Reverse(0);
		Bold(0);
	}
	NeedsRedraw.clear();
	wrefresh(itsWindow);
}

template <class T>
void Menu<T>::Go(Where where)
{
	if (Empty()) return;
	int MaxHighlight = itsOptions.size()-1;
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	int MaxCurrentHighlight = itsBeginning+itsHeight-1;
	idlok(itsWindow, 1);
	scrollok(itsWindow, 1);
	switch (where)
	{
		case wUp:
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
					Go(wDown);
				else
					Go(wUp);
			}
			break;
		}
		case wDown:
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
					Go(wUp);
				else
					Go(wDown);
			}
			break;
		}
		case wPageUp:
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
					Go(wDown);
				else
					Go(wUp);
			}
			redraw_screen();
			break;
		}
		case wPageDown:
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
					Go(wUp);
				else
					Go(wDown);
			}
			redraw_screen();
			break;
		}
		case wHome:
		{
			itsHighlight = 0;
			itsBeginning = 0;
			if (is_static())
			{
				if (itsHighlight == 0)
					Go(wDown);
				else
					Go(wUp);
			}
			redraw_screen();
			break;
		}
		case wEnd:
		{
			itsHighlight = MaxHighlight;
			itsBeginning = MaxBeginning;
			if (is_static())
			{
				if (itsHighlight == MaxHighlight)
					Go(wUp);
				else
					Go(wDown);
			}
			redraw_screen();
			break;
		}
	}
	idlok(itsWindow, 0);
	scrollok(itsWindow, 0);
}

template <class T>
void Menu<T>::Highlight(int which)
{
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

template <class T>
void Menu<T>::Reset()
{
	NeedsRedraw.clear();
	itsHighlight = 0;
	itsBeginning = 0;
}

template <class T>
void Menu<T>::Clear(bool clear_screen)
{
	for (T_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
	itsOptions.clear();
	NeedsRedraw.clear();
	itsStaticsNumber = 0;
	if (clear_screen)
		Window::Clear();
}

template <class T>
void Menu<T>::Select(int option, bool selected)
{
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

template <class T>
bool Menu<T>::Selected(int option) const
{
	try
	{
		return itsOptions.at(option)->selected;
	}
	catch (std::out_of_range)
	{
		return false;
	}
}

template <class T>
bool Menu<T>::IsAnySelected() const
{
	bool result = 0;
	for (T_const_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
	{
		if ((*it)->selected)
		{
			result = 1;
			break;
		}
	}
	return result;
}

template <class T>
void Menu<T>::GetSelectedList(vector<int> &v) const
{
	int i = 0;
	for (T_const_iterator it = itsOptions.begin(); it != itsOptions.end(); it++, i++)
		if ((*it)->selected)
			v.push_back(i);
}

template <class T>
int Menu<T>::GetRealChoice() const
{
	int real_choice	= 0;
	T_const_iterator it = itsOptions.begin();
	for (int i = 0; i < itsHighlight; it++, i++)
		if (!(*it)->is_static) real_choice++;
	
	return real_choice;
}

template <class T>
bool Menu<T>::IsStatic(int option) const
{
	try
	{
		return itsOptions.at(option)->is_static;
	}
	catch (std::out_of_range)
	{
		return 1;
	}
}

template <class T>
Window * Menu<T>::EmptyClone() const
{
	return new Menu(GetStartX(),GetStartY(),GetWidth(),GetHeight(),itsTitle,itsBaseColor,itsBorder);
}

template <class T>
string Menu<T>::DisplayOption(const T &t) const
{
	return itsItemDisplayer ? itsItemDisplayer(t, itsItemDisplayerUserdata) : "";
}

template <>
string Menu<string>::DisplayOption(const string &str) const;

#endif

