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
#include "strbuffer.h"

//include <stdexcept>

//enum Location { lLeft, lCenter, lRight };

class List
{
	public:
		
		class InvalidItem { };
		
		List() { }
		virtual ~List() { };
		virtual void Select(int, bool) = 0;
		virtual void Static(int, bool) = 0;
		virtual bool isSelected(int = -1) const = 0;
		virtual bool isStatic(int = -1) const = 0;
		virtual bool hasSelected() const = 0;
		virtual void GetSelected(std::vector<size_t> &) const = 0;
		virtual void Highlight(size_t) = 0;
		virtual size_t Size() const = 0;
		virtual size_t Choice() const = 0;
		virtual size_t RealChoice() const = 0;
};

template <class T> class Menu : public Window, public List
{
	typedef void (*ItemDisplayer) (const T &, void *, Menu<T> *);
	
	struct Option
	{
		Option() : Item(0), isBold(0), isSelected(0), isStatic(0), haveSeparator(0) { }
		T *Item;
		bool isBold;
		bool isSelected;
		bool isStatic;
		bool haveSeparator;
	};
	
	typedef typename std::vector<Option>::iterator option_iterator;
	typedef typename std::vector<Option>::const_iterator option_const_iterator;
	
	public:
		Menu(size_t, size_t, size_t, size_t, const std::string &, Color, Border);
		Menu(const Menu &);
		virtual ~Menu();
		
		void SetItemDisplayer(ItemDisplayer ptr) { itsItemDisplayer = ptr; }
		void SetItemDisplayerUserData(void *data) { itsItemDisplayerUserdata = data; }
		
		void Reserve(size_t size);
		void ResizeBuffer(size_t size);
		void AddOption(const T &item = T(), bool is_bold = 0, bool is_static = 0, bool have_separator = 0);
		void AddSeparator();
		void InsertOption(size_t pos, const T &Item = T(), bool is_bold = 0, bool is_static = 0, bool have_separator = 0);
		void InsertSeparator(size_t pos);
		void DeleteOption(size_t pos);
		void IntoSeparator(size_t pos);
		void Swap(size_t, size_t);
		
		bool isBold(int id = -1);
		void BoldOption(int, bool);
		
		virtual void Select(int id, bool value);
		virtual void Static(int id, bool value);
		virtual bool isSelected(int id = -1) const;
		virtual bool isStatic(int id = -1) const;
		virtual bool hasSelected() const;
		virtual void GetSelected(std::vector<size_t> &v) const;
		virtual void Highlight(size_t);
		virtual size_t Size() const;
		virtual size_t Choice() const;
		virtual size_t RealChoice() const;
		
		virtual void Refresh();
		virtual void Scroll(Where);
		virtual void Reset();
		virtual void Clear(bool clrscr = 1);
		
		void SetSelectPrefix(Buffer *b) { itsSelectedPrefix = b; }
		void SetSelectSuffix(Buffer *b) { itsSelectedSuffix = b; }
		
		void HighlightColor(Color col) { itsHighlightColor = col; }
		void Highlighting(bool hl) { highlightEnabled = hl; }
		
		bool Empty() const { return itsOptions.empty(); }
		
		T &Back();
		const T &Back() const;
		T &Current();
		const T &Current() const;
		T &at(size_t i);
		const T &at(size_t i) const;
		const T &operator[](size_t i) const;
		T &operator[](size_t i);
		
		virtual Menu<T> *Clone() const { return new Menu<T>(*this); }
		virtual Menu<T> *EmptyClone() const;
		
		//virtual string GetOption(int i = -1) const;
		
	protected:
		ItemDisplayer itsItemDisplayer;
		void *itsItemDisplayerUserdata;
		
		std::vector<Option> itsOptions;
		
		int itsBeginning;
		int itsHighlight;
		
		Color itsHighlightColor;
		bool highlightEnabled;
		
		Buffer *itsSelectedPrefix;
		Buffer *itsSelectedSuffix;
};

template <class T> Menu<T>::Menu(size_t startx,
				 size_t starty,
				 size_t width,
				 size_t height,
				 const std::string &title,
				 Color color,
				 Border border)
				 : Window(startx, starty, width, height, title, color, border),
				 itsItemDisplayer(0),
				 itsItemDisplayerUserdata(0),
				 itsBeginning(0),
				 itsHighlight(0),
				 itsHighlightColor(itsBaseColor),
				 highlightEnabled(1),
				 itsSelectedPrefix(0),
				 itsSelectedSuffix(0)
{
}

template <class T> Menu<T>::Menu(const Menu &m) : Window(m)
{
	itsOptions = m.itsOptions;
	itsItemDisplayer = m.itsItemDisplayer;
	itsItemDisplayerUserdata = m.itsItemDisplayerUserdata;
	itsBeginning = m.itsBeginning;
	itsHighlight = m.itsHighlight;
	itsHighlightColor = m.itsHighlightColor;
	highlightEnabled = m.highlightEnabled;
}

template <class T> Menu<T>::~Menu()
{
	for (option_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete it->Item;
}

template <class T> void Menu<T>::Reserve(size_t size)
{
	itsOptions.reserve(size);
}

template <class T> void Menu<T>::ResizeBuffer(size_t size)
{
	itsOptions.resize(size);
	for (size_t i = 0; i < size; i++)
		if (!itsOptions[i].Item)
			itsOptions[i].Item = new T();
}

template <class T> void Menu<T>::AddOption(const T &item, bool is_bold, bool is_static, bool have_separator)
{
	Option o;
	o.Item = new T(item);
	o.isBold = is_bold;
	o.isStatic = is_static;
	o.haveSeparator = have_separator;
	itsOptions.push_back(o);
}

//template <> void Menu<Buffer>::AddOption(const Buffer &buf, bool is_bold, bool is_static, bool have_separator);

template <class T> void Menu<T>::AddSeparator()
{
	Option o;
	o.isStatic = true;
	o.haveSeparator = true;
	itsOptions.push_back(o);
}

template <class T> void Menu<T>::InsertOption(size_t pos, const T &item, bool is_bold, bool is_static, bool have_separator)
{
	Option o;
	o.Item = new T(item);
	o.isBold = is_bold;
	o.isStatic = is_static;
	o.haveSeparator = have_separator;
	itsOptions.insert(itsOptions.begin()+pos, o);
}

template <class T> void Menu<T>::InsertSeparator(size_t pos)
{
	Option o;
	o.isStatic = true;
	o.haveSeparator = true;
	itsOptions.insert(itsOptions.begin()+pos, o);
}

template <class T> void Menu<T>::DeleteOption(size_t pos)
{
	if (itsOptions.empty())
		return;
	delete itsOptions.at(pos).Item;
	itsOptions.erase(itsOptions.begin()+pos);
}

template <class T> void Menu<T>::IntoSeparator(size_t pos)
{
	if (itsOptions.at(pos).Item)
		delete itsOptions[pos].Item;
	itsOptions[pos].Item = 0;
	itsOptions[pos].isBold = false;
	itsOptions[pos].isSelected = false;
	itsOptions[pos].isStatic = true;
	itsOptions[pos].haveSeparator = true;
}

template <class T> void Menu<T>::BoldOption(int index, bool bold)
{
	itsOptions.at(index).isBold = bold;
}

template <class T>
void Menu<T>::Swap(size_t one, size_t two)
{
	std::swap(itsOptions.at(one), itsOptions.at(two));
}

template <class T> void Menu<T>::Refresh()
{
	if (itsOptions.empty())
	{
		wrefresh(itsWindow);
		return;
	}
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	if (itsHighlight > itsBeginning+int(itsHeight)-1)
		itsBeginning = itsHighlight-itsHeight+1;
	if (itsBeginning < 0)
		itsBeginning = 0;
	else if (itsBeginning > MaxBeginning)
		itsBeginning = MaxBeginning;
	if (!itsOptions.empty() && itsHighlight > int(itsOptions.size())-1)
		itsHighlight = itsOptions.size()-1;
	size_t line = 0;
	for (size_t i = itsBeginning; i < itsBeginning+itsHeight; i++)
	{
		GotoXY(0, line);
		if (i >= itsOptions.size())
		{
			for (; line < itsHeight; line++)
				mvwhline(itsWindow, line, 0, 32, itsWidth);
			break;
		}
		if (itsOptions[i].isBold)
			Bold(1);
		if (highlightEnabled && int(i) == itsHighlight)
		{
			Reverse(1);
			*this << itsHighlightColor;
		}
		mvwhline(itsWindow, line, 0, !itsOptions[i].haveSeparator*32, itsWidth);
		if (itsOptions[i].isSelected && itsSelectedPrefix)
			*this << *itsSelectedPrefix;
		if (itsItemDisplayer && itsOptions[i].Item)
			itsItemDisplayer(*itsOptions[i].Item, itsItemDisplayerUserdata, this);
		if (itsOptions[i].isSelected && itsSelectedSuffix)
			*this << *itsSelectedSuffix;
		if (highlightEnabled && int(i) == itsHighlight)
		{
			*this << clEnd;
			Reverse(0);
		}
		if (itsOptions[i].isBold)
			Bold(0);
		line++;
	}
	wrefresh(itsWindow);
}

template <class T> void Menu<T>::Scroll(Where where)
{
	int MaxHighlight = itsOptions.size()-1;
	int MaxBeginning = itsOptions.size() < itsHeight ? 0 : itsOptions.size()-itsHeight;
	int MaxCurrentHighlight = itsBeginning+itsHeight-1;
	switch (where)
	{
		case wUp:
		{
			if (itsHighlight <= itsBeginning && itsHighlight > 0)
			{
				itsBeginning--;
				//Window::Scroll(wUp);
			}
			if (itsHighlight == 0)
			{
				break;
			}
			else
			{
				itsHighlight--;
			}
			if (itsOptions[itsHighlight].isStatic)
			{
				Scroll(itsHighlight == 0 ? wDown : wUp);
			}
			break;
		}
		case wDown:
		{
			if (itsHighlight >= MaxCurrentHighlight && itsHighlight < MaxHighlight)
			{
				itsBeginning++;
				//Window::Scroll(wDown);
			}
			if (itsHighlight == MaxHighlight)
			{
				break;
			}
			else
			{
				itsHighlight++;
			}
			if (itsOptions[itsHighlight].isStatic)
			{
				Scroll(itsHighlight == MaxHighlight ? wUp : wDown);
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
				if (itsHighlight < 0)
					itsHighlight = 0;
			}
			if (itsOptions[itsHighlight].isStatic)
			{
				Scroll(itsHighlight == 0 ? wDown: wUp);
			}
			break;
		}
		case wPageDown:
		{
			itsHighlight += itsHeight;
			itsBeginning += itsHeight;
			if (itsBeginning > MaxBeginning)
			{
				itsBeginning = MaxBeginning;
				if (itsHighlight > MaxHighlight)
					itsHighlight = MaxHighlight;
			}
			if (itsOptions[itsHighlight].isStatic)
			{
				Scroll(itsHighlight == MaxHighlight ? wUp : wDown);
			}
			break;
		}
		case wHome:
		{
			itsHighlight = 0;
			itsBeginning = 0;
			if (itsOptions[itsHighlight].isStatic)
			{
				Scroll(itsHighlight == 0 ? wDown : wUp);
			}
			break;
		}
		case wEnd:
		{
			itsHighlight = MaxHighlight;
			itsBeginning = MaxBeginning;
			if (itsOptions[itsHighlight].isStatic)
			{
				Scroll(itsHighlight == MaxHighlight ? wUp : wDown);
			}
			break;
		}
	}
}

template <class T> void Menu<T>::Reset()
{
	itsHighlight = 0;
	itsBeginning = 0;
}

template <class T> void Menu<T>::Clear(bool clrscr)
{
	for (option_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete it->Item;
	itsOptions.clear();
	if (clrscr)
		Window::Clear();
}

template <class T> bool Menu<T>::isBold(int id)
{
	return itsOptions.at(id == -1 ? itsHighlight : id).isBold;
}

template <class T> void Menu<T>::Select(int id, bool value)
{
	itsOptions.at(id).isSelected = value;
}

template <class T> void Menu<T>::Static(int id, bool value)
{
	itsOptions.at(id).isStatic = value;
}

template <class T> bool Menu<T>::isSelected(int id) const
{
	return itsOptions.at(id == -1 ? itsHighlight : id).isSelected;
}

template <class T> bool Menu<T>::isStatic(int id) const
{
	return itsOptions.at(id == -1 ? itsHighlight : id).isStatic;
}

template <class T> bool Menu<T>::hasSelected() const
{
	for (option_const_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		if (it->isSelected)
			return true;
	return false;
}

template <class T> void Menu<T>::GetSelected(std::vector<size_t> &v) const
{
	for (size_t i = 0; i < itsOptions.size(); i++)
		if (itsOptions[i].isSelected)
			v.push_back(i);
}

template <class T> void Menu<T>::Highlight(size_t pos)
{
	itsHighlight = pos;
	itsBeginning = pos-itsHeight/2;
}

template <class T> size_t Menu<T>::Size() const
{
	return itsOptions.size();
}

template <class T> size_t Menu<T>::Choice() const
{
	return itsHighlight;
}

template <class T> size_t Menu<T>::RealChoice() const
{
	size_t result = 0;
	for (option_const_iterator it = itsOptions.begin(); it != itsOptions.begin()+itsHighlight; it++)
		if (!it->isStatic)
			result++;
	return result;
}

template <class T> T &Menu<T>::Back()
{
	if (itsOptions.back().Item)
		return *itsOptions.back().Item;
	else
		throw InvalidItem();
}

template <class T> const T &Menu<T>::Back() const
{
	if (itsOptions.back().Item)
		return *itsOptions.back().Item;
	else
		throw InvalidItem();
}

template <class T> T &Menu<T>::Current()
{
	if (itsOptions.at(itsHighlight).Item)
		return *itsOptions.at(itsHighlight).Item;
	else
		throw InvalidItem();
}

template <class T> const T &Menu<T>::Current() const
{
	if (itsOptions.at(itsHighlight).Item)
		return *itsOptions.at(itsHighlight).Item;
	else
		throw InvalidItem();
}

template <class T> T &Menu<T>::at(size_t i)
{
	if (itsOptions.at(i).Item)
		return *itsOptions.at(i).Item;
	else
		throw InvalidItem();
}

template <class T> const T &Menu<T>::at(size_t i) const
{
	if (itsOptions.at(i).Item)
		return *itsOptions.at(i).Item;
	else
		throw InvalidItem();
}

template <class T> const T &Menu<T>::operator[](size_t i) const
{
	if (itsOptions[i].Item)
		return *itsOptions[i].Item;
	else
		throw InvalidItem();
}

template <class T> T &Menu<T>::operator[](size_t i)
{
	if (itsOptions[i].Item)
		return *itsOptions[i].Item;
	else
		throw InvalidItem();
}

template <class T> Menu<T> *Menu<T>::EmptyClone() const
{
	return new Menu<T>(GetStartX(), GetStartY(), GetWidth(), GetHeight(), itsTitle, itsBaseColor, itsBorder);
}

#endif

