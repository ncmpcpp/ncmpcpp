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

#ifndef _MENU_H
#define _MENU_H

#include "window.h"
#include "strbuffer.h"
#include "misc.h"

namespace NCurses
{
	class List
	{
		public:
			
			class InvalidItem { };
			
			List() { }
			virtual ~List() { }
			
			virtual void Select(int, bool) = 0;
			virtual void Static(int, bool) = 0;
			virtual bool Empty() const = 0;
			virtual bool isSelected(int = -1) const = 0;
			virtual bool isStatic(int = -1) const = 0;
			virtual bool hasSelected() const = 0;
			virtual void GetSelected(std::vector<size_t> &) const = 0;
			virtual void Highlight(size_t) = 0;
			virtual size_t Size() const = 0;
			virtual size_t Choice() const = 0;
			virtual size_t RealChoice() const = 0;
			
			void SelectCurrent();
			void ReverseSelection(size_t = 0);
			bool Deselect();
			
			virtual void ApplyFilter(const std::string &, size_t = 0, bool = 0) = 0;
			virtual const std::string &GetFilter() = 0;
			virtual std::string GetOption(size_t) = 0;
			
			virtual bool isFiltered() = 0;
	};
	
	template <class T> class Menu : public Window, public List
	{
		typedef void (*ItemDisplayer) (const T &, void *, Menu<T> *);
		typedef std::string (*GetStringFunction) (const T &, void *);
		
		struct Option
		{
			Option() : isBold(0), isSelected(0), isStatic(0) { }
			Option(const T &t, bool is_bold, bool is_static) :
			Item(t), isBold(is_bold), isSelected(0), isStatic(is_static) { }
			
			T Item;
			bool isBold;
			bool isSelected;
			bool isStatic;
		};
		
		typedef typename std::vector<Option *>::iterator option_iterator;
		typedef typename std::vector<Option *>::const_iterator option_const_iterator;
		
		public:
			Menu(size_t, size_t, size_t, size_t, const std::string &, Color, Border);
			Menu(const Menu &);
			virtual ~Menu();
			
			void SetItemDisplayer(ItemDisplayer ptr) { itsItemDisplayer = ptr; }
			void SetItemDisplayerUserData(void *data) { itsItemDisplayerUserdata = data; }
			
			void SetGetStringFunction(GetStringFunction f) { itsGetStringFunction = f; }
			void SetGetStringFunctionUserData(void *data) { itsGetStringFunctionUserData = data; }
			
			void Reserve(size_t size);
			void ResizeBuffer(size_t size);
			void AddOption(const T &item, bool is_bold = 0, bool is_static = 0);
			void AddSeparator();
			void InsertOption(size_t pos, const T &Item, bool is_bold = 0, bool is_static = 0);
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
			
			virtual void ApplyFilter(const std::string &filter, size_t beginning = 0, bool case_sensitive = 0);
			virtual const std::string &GetFilter();
			virtual std::string GetOption(size_t pos);
			
			virtual bool isFiltered() { return itsOptionsPtr == &itsFilteredOptions; }
			
			void ShowAll() { itsOptionsPtr = &itsOptions; }
			void ShowFiltered() { itsOptionsPtr = &itsFilteredOptions; }
			
			virtual void Refresh();
			virtual void Scroll(Where);
			virtual void Reset();
			virtual void Clear(bool clrscr = 1);
			
			void SetSelectPrefix(Buffer *b) { itsSelectedPrefix = b; }
			void SetSelectSuffix(Buffer *b) { itsSelectedSuffix = b; }
			
			void HighlightColor(Color col) { itsHighlightColor = col; }
			void Highlighting(bool hl) { highlightEnabled = hl; }
			
			virtual bool Empty() const { return itsOptionsPtr->empty(); }
			
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
		
		protected:
			ItemDisplayer itsItemDisplayer;
			void *itsItemDisplayerUserdata;
			GetStringFunction itsGetStringFunction;
			void *itsGetStringFunctionUserData;
			
			std::string itsFilter;
			
			std::vector<Option *> *itsOptionsPtr;
			std::vector<Option *> itsOptions;
			std::vector<Option *> itsFilteredOptions;
			std::vector<size_t> itsFilteredRealPositions;
			
			int itsBeginning;
			int itsHighlight;
			
			Color itsHighlightColor;
			bool highlightEnabled;
			
			Buffer *itsSelectedPrefix;
			Buffer *itsSelectedSuffix;
	};
	
	template <> std::string Menu<std::string>::GetOption(size_t pos);
}

template <class T> NCurses::Menu<T>::Menu(size_t startx,
				 size_t starty,
				 size_t width,
				 size_t height,
				 const std::string &title,
				 Color color,
				 Border border)
				 : Window(startx, starty, width, height, title, color, border),
				 itsItemDisplayer(0),
				 itsItemDisplayerUserdata(0),
				 itsGetStringFunction(0),
				 itsGetStringFunctionUserData(0),
				 itsOptionsPtr(&itsOptions),
				 itsBeginning(0),
				 itsHighlight(0),
				 itsHighlightColor(itsBaseColor),
				 highlightEnabled(1),
				 itsSelectedPrefix(0),
				 itsSelectedSuffix(0)
{
}

template <class T> NCurses::Menu<T>::Menu(const Menu &m) : Window(m)
{
	itsOptions = m.itsOptions;
	itsItemDisplayer = m.itsItemDisplayer;
	itsItemDisplayerUserdata = m.itsItemDisplayerUserdata;
	itsBeginning = m.itsBeginning;
	itsHighlight = m.itsHighlight;
	itsHighlightColor = m.itsHighlightColor;
	highlightEnabled = m.highlightEnabled;
}

template <class T> NCurses::Menu<T>::~Menu()
{
	for (option_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
}

template <class T> void NCurses::Menu<T>::Reserve(size_t size)
{
	itsOptions.reserve(size);
}

template <class T> void NCurses::Menu<T>::ResizeBuffer(size_t size)
{
	itsOptions.resize(size);
	for (size_t i = 0; i < size; i++)
		if (!itsOptions[i])
			itsOptions[i] = new Option();
}

template <class T> void NCurses::Menu<T>::AddOption(const T &item, bool is_bold, bool is_static)
{
	itsOptions.push_back(new Option(item, is_bold, is_static));
}

template <class T> void NCurses::Menu<T>::AddSeparator()
{
	itsOptions.push_back(0);
}

template <class T> void NCurses::Menu<T>::InsertOption(size_t pos, const T &item, bool is_bold, bool is_static)
{
	itsOptions.insert(itsOptions.begin()+pos, new Option(item, is_bold, is_static));
}

template <class T> void NCurses::Menu<T>::InsertSeparator(size_t pos)
{
	itsOptions.insert(itsOptions.begin()+pos, 0);
}

template <class T> void NCurses::Menu<T>::DeleteOption(size_t pos)
{
	if (itsOptions.empty())
		return;
	if (itsOptionsPtr == &itsFilteredOptions)
	{
		delete itsOptions.at(itsFilteredRealPositions[pos]);
		itsOptions.erase(itsOptions.begin()+itsFilteredRealPositions[pos]);
		itsFilteredOptions.erase(itsFilteredOptions.begin()+pos);
		itsFilteredRealPositions.erase(itsFilteredRealPositions.begin()+pos);
		for (size_t i = pos; i < itsFilteredRealPositions.size(); i++)
			itsFilteredRealPositions[i]--;
	}
	else
	{
		delete itsOptions.at(pos);
		itsOptions.erase(itsOptions.begin()+pos);
	}
	if (itsOptionsPtr->empty())
		Window::Clear();
}

template <class T> void NCurses::Menu<T>::IntoSeparator(size_t pos)
{
	delete itsOptions.at(pos);
	itsOptions[pos] = 0;
}

template <class T> void NCurses::Menu<T>::BoldOption(int index, bool bold)
{
	if (!itsOptions.at(index))
		return;
	itsOptions[index]->isBold = bold;
}

template <class T>
void NCurses::Menu<T>::Swap(size_t one, size_t two)
{
	std::swap(itsOptions.at(one), itsOptions.at(two));
}

template <class T> void NCurses::Menu<T>::Refresh()
{
	if (itsOptionsPtr->empty())
	{
		Window::Refresh();
		return;
	}
	int MaxBeginning = itsOptionsPtr->size() < itsHeight ? 0 : itsOptionsPtr->size()-itsHeight;
	if (itsHighlight > itsBeginning+int(itsHeight)-1)
		itsBeginning = itsHighlight-itsHeight+1;
	if (itsBeginning < 0)
		itsBeginning = 0;
	else if (itsBeginning > MaxBeginning)
		itsBeginning = MaxBeginning;
	if (!itsOptionsPtr->empty() && itsHighlight > int(itsOptionsPtr->size())-1)
		itsHighlight = itsOptionsPtr->size()-1;
	if (!(*itsOptionsPtr)[itsHighlight]) // it shouldn't be on separator.
	{
		Scroll(wUp);
		if (!(*itsOptionsPtr)[itsHighlight]) // if it's still on separator, move in other direction.
			Scroll(wDown);
	}
	size_t line = 0;
	for (size_t i = itsBeginning; i < itsBeginning+itsHeight; i++)
	{
		GotoXY(0, line);
		if (i >= itsOptionsPtr->size())
		{
			for (; line < itsHeight; line++)
				mvwhline(itsWindow, line, 0, 32, itsWidth);
			break;
		}
		if (!(*itsOptionsPtr)[i]) // separator
		{
			mvwhline(itsWindow, line++, 0, 0, itsWidth);
			continue;
		}
		if ((*itsOptionsPtr)[i]->isBold)
			Bold(1);
		if (highlightEnabled && int(i) == itsHighlight)
		{
			Reverse(1);
			*this << itsHighlightColor;
		}
		mvwhline(itsWindow, line, 0, 32, itsWidth);
		if ((*itsOptionsPtr)[i]->isSelected && itsSelectedPrefix)
			*this << *itsSelectedPrefix;
		if (itsItemDisplayer)
			itsItemDisplayer((*itsOptionsPtr)[i]->Item, itsItemDisplayerUserdata, this);
		if ((*itsOptionsPtr)[i]->isSelected && itsSelectedSuffix)
			*this << *itsSelectedSuffix;
		if (highlightEnabled && int(i) == itsHighlight)
		{
			*this << clEnd;
			Reverse(0);
		}
		if ((*itsOptionsPtr)[i]->isBold)
			Bold(0);
		line++;
	}
	Window::Refresh();
}

template <class T> void NCurses::Menu<T>::Scroll(Where where)
{
	if (itsOptionsPtr->empty())
		return;
	int MaxHighlight = itsOptionsPtr->size()-1;
	int MaxBeginning = itsOptionsPtr->size() < itsHeight ? 0 : itsOptionsPtr->size()-itsHeight;
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
			if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
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
			if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
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
			if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
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
			if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == MaxHighlight ? wUp : wDown);
			}
			break;
		}
		case wHome:
		{
			itsHighlight = 0;
			itsBeginning = 0;
			if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == 0 ? wDown : wUp);
			}
			break;
		}
		case wEnd:
		{
			itsHighlight = MaxHighlight;
			itsBeginning = MaxBeginning;
			if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == MaxHighlight ? wUp : wDown);
			}
			break;
		}
	}
}

template <class T> void NCurses::Menu<T>::Reset()
{
	itsHighlight = 0;
	itsBeginning = 0;
}

template <class T> void NCurses::Menu<T>::Clear(bool clrscr)
{
	for (option_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
	itsOptions.clear();
	itsFilteredOptions.clear();
	itsFilteredRealPositions.clear();
	itsFilter.clear();
	itsOptionsPtr = &itsOptions;
	if (clrscr)
		Window::Clear();
}

template <class T> bool NCurses::Menu<T>::isBold(int id)
{
	id = id == -1 ? itsHighlight : id;
	if (!itsOptionsPtr->at(id))
		return 0;
	return (*itsOptionsPtr)[id]->isBold;
}

template <class T> void NCurses::Menu<T>::Select(int id, bool value)
{
	if (!itsOptionsPtr->at(id))
		return;
	(*itsOptionsPtr)[id]->isSelected = value;
}

template <class T> void NCurses::Menu<T>::Static(int id, bool value)
{
	if (!itsOptionsPtr->at(id))
		return;
	(*itsOptionsPtr)[id]->isStatic = value;
}

template <class T> bool NCurses::Menu<T>::isSelected(int id) const
{
	id = id == -1 ? itsHighlight : id;
	if (!itsOptionsPtr->at(id))
		return 0;
	return (*itsOptionsPtr)[id]->isSelected;
}

template <class T> bool NCurses::Menu<T>::isStatic(int id) const
{
	id = id == -1 ? itsHighlight : id;
	if (!itsOptionsPtr->at(id))
		return 1;
	return (*itsOptionsPtr)[id]->isStatic;
}

template <class T> bool NCurses::Menu<T>::hasSelected() const
{
	for (option_const_iterator it = itsOptionsPtr->begin(); it != itsOptionsPtr->end(); it++)
		if (*it && (*it)->isSelected)
			return true;
	return false;
}

template <class T> void NCurses::Menu<T>::GetSelected(std::vector<size_t> &v) const
{
	for (size_t i = 0; i < itsOptionsPtr->size(); i++)
		if ((*itsOptionsPtr)[i]->isSelected)
			v.push_back(i);
}

template <class T> void NCurses::Menu<T>::Highlight(size_t pos)
{
	itsHighlight = pos;
	itsBeginning = pos-itsHeight/2;
}

template <class T> size_t NCurses::Menu<T>::Size() const
{
	return itsOptionsPtr->size();
}

template <class T> size_t NCurses::Menu<T>::Choice() const
{
	return itsHighlight;
}

template <class T> size_t NCurses::Menu<T>::RealChoice() const
{
	size_t result = 0;
	for (option_const_iterator it = itsOptionsPtr->begin(); it != itsOptionsPtr->begin()+itsHighlight; it++)
		if (*it && !(*it)->isStatic)
			result++;
	return result;
}

template <class T> void NCurses::Menu<T>::ApplyFilter(const std::string &filter, size_t beginning, bool case_sensitive)
{
	if (filter == itsFilter)
		return;
	itsFilter = filter;
	if (!case_sensitive)
		ToLower(itsFilter);
	itsFilteredRealPositions.clear();
	itsFilteredOptions.clear();
	itsOptionsPtr = &itsOptions;
	if (itsFilter.empty())
		return;
	for (size_t i = 0; i < beginning; i++)
	{
		itsFilteredRealPositions.push_back(i);
		itsFilteredOptions.push_back(itsOptions[i]);
	}
	std::string option;
	for (size_t i = beginning; i < itsOptions.size(); i++)
	{
		option = GetOption(i);
		if (!case_sensitive)
			ToLower(option);
		if (option.find(itsFilter) != std::string::npos)
		{
			itsFilteredRealPositions.push_back(i);
			itsFilteredOptions.push_back(itsOptions[i]);
		}
	}
	itsOptionsPtr = &itsFilteredOptions;
	if (itsOptionsPtr->empty()) // oops, we didn't find anything
		Window::Clear();
}

template <class T> const std::string &NCurses::Menu<T>::GetFilter()
{
	return itsFilter;
}

template <class T> std::string NCurses::Menu<T>::GetOption(size_t pos)
{
	if (itsOptionsPtr->at(pos) && itsGetStringFunction)
		return itsGetStringFunction((*itsOptionsPtr)[pos]->Item, itsGetStringFunctionUserData);
	else
		return "";
}

template <class T> T &NCurses::Menu<T>::Back()
{
	if (!itsOptionsPtr->back())
		throw InvalidItem();
	return itsOptionsPtr->back()->Item;
}

template <class T> const T &NCurses::Menu<T>::Back() const
{
	if (!itsOptionsPtr->back())
		throw InvalidItem();
	return itsOptionsPtr->back()->Item;
}

template <class T> T &NCurses::Menu<T>::Current()
{
	if (!itsOptionsPtr->at(itsHighlight))
		throw InvalidItem();
	return (*itsOptionsPtr)[itsHighlight]->Item;
}

template <class T> const T &NCurses::Menu<T>::Current() const
{
	if (!itsOptionsPtr->at(itsHighlight))
		throw InvalidItem();
	return (*itsOptionsPtr)[itsHighlight]->Item;
}

template <class T> T &NCurses::Menu<T>::at(size_t i)
{
	if (!itsOptionsPtr->at(i))
		throw InvalidItem();
	return (*itsOptionsPtr)[i]->Item;
}

template <class T> const T &NCurses::Menu<T>::at(size_t i) const
{
	if (!itsOptions->at(i))
		throw InvalidItem();
	return (*itsOptionsPtr)[i]->Item;
}

template <class T> const T &NCurses::Menu<T>::operator[](size_t i) const
{
	if (!(*itsOptionsPtr)[i])
		throw InvalidItem();
	return (*itsOptionsPtr)[i]->Item;
}

template <class T> T &NCurses::Menu<T>::operator[](size_t i)
{
	if (!(*itsOptionsPtr)[i])
		throw InvalidItem();
	return (*itsOptionsPtr)[i]->Item;
}

template <class T> NCurses::Menu<T> *NCurses::Menu<T>::EmptyClone() const
{
	return new NCurses::Menu<T>(GetStartX(), GetStartY(), GetWidth(), GetHeight(), itsTitle, itsBaseColor, itsBorder);
}

#endif

