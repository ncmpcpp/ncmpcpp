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

#include <regex.h>
#include <set>

#include "window.h"
#include "strbuffer.h"

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
			
			virtual bool Search(const std::string &, size_t = 0, int = 0) = 0;
			virtual const std::string &GetSearchConstraint() = 0;
			virtual void NextFound(bool) = 0;
			virtual void PrevFound(bool) = 0;
			
			virtual void ApplyFilter(const std::string &, size_t = 0, int = 0) = 0;
			virtual const std::string &GetFilter() = 0;
			virtual std::string GetOption(size_t) = 0;
			
			virtual bool isFiltered() = 0;
	};
	
	template <typename T> class Menu : public Window, public List
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
		
		template <typename Comparison> class InternalSorting
		{
			Comparison cmp;
			
			public:
				bool operator()(Option *a, Option *b)
				{
					return cmp(a->Item, b->Item);
				}
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
			void ResizeList(size_t size);
			void AddOption(const T &item, bool is_bold = 0, bool is_static = 0);
			void AddSeparator();
			void InsertOption(size_t pos, const T &Item, bool is_bold = 0, bool is_static = 0);
			void InsertSeparator(size_t pos);
			void DeleteOption(size_t pos);
			void IntoSeparator(size_t pos);
			void Swap(size_t one, size_t two);
			void Move(size_t from, size_t to);
			
			bool isBold(int id = -1);
			void BoldOption(int index, bool bold);
			
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
			
			virtual bool Search(const std::string &constraint, size_t beginning = 0, int flags = 0);
			virtual const std::string &GetSearchConstraint() { return itsSearchConstraint; }
			virtual void NextFound(bool wrap);
			virtual void PrevFound(bool wrap);
			
			virtual void ApplyFilter(const std::string &filter, size_t beginning = 0, int flags = 0);
			virtual const std::string &GetFilter();
			virtual std::string GetOption(size_t pos);
			
			virtual bool isFiltered() { return itsOptionsPtr == &itsFilteredOptions; }
			
			void ShowAll() { itsOptionsPtr = &itsOptions; }
			void ShowFiltered() { itsOptionsPtr = &itsFilteredOptions; }
			
			virtual void Refresh();
			virtual void Scroll(Where where);
			virtual void Reset();
			virtual void Clear(bool clrscr = 1);
			
			template <typename Comparison> void Sort(size_t beginning = 0)
			{
				if (itsOptions.empty())
					return;
				sort(itsOptions.begin()+beginning, itsOptions.end(), InternalSorting<Comparison>());
				if (isFiltered())
					ApplyFilter(itsFilter);
			}
			
			void SetSelectPrefix(Buffer *b) { itsSelectedPrefix = b; }
			void SetSelectSuffix(Buffer *b) { itsSelectedSuffix = b; }
			
			void HighlightColor(Color col) { itsHighlightColor = col; }
			void Highlighting(bool hl) { highlightEnabled = hl; }
			void CyclicScrolling(bool cs) { useCyclicScrolling = cs; }
			
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
			void ClearFiltered();
			
			ItemDisplayer itsItemDisplayer;
			void *itsItemDisplayerUserdata;
			GetStringFunction itsGetStringFunction;
			void *itsGetStringFunctionUserData;
			
			std::string itsFilter;
			std::string itsSearchConstraint;
			
			std::vector<Option *> *itsOptionsPtr;
			std::vector<Option *> itsOptions;
			std::vector<Option *> itsFilteredOptions;
			std::vector<size_t> itsFilteredRealPositions;
			std::set<size_t> itsFound;
			
			int itsBeginning;
			int itsHighlight;
			
			Color itsHighlightColor;
			bool highlightEnabled;
			bool useCyclicScrolling;
			
			Buffer *itsSelectedPrefix;
			Buffer *itsSelectedSuffix;
	};
	
	template <> std::string Menu<std::string>::GetOption(size_t pos);
}

template <typename T> NCurses::Menu<T>::Menu(size_t startx,
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
					useCyclicScrolling(0),
					itsSelectedPrefix(0),
					itsSelectedSuffix(0)
{
}

template <typename T> NCurses::Menu<T>::Menu(const Menu &m) : Window(m),
					itsItemDisplayer(m.itsItemDisplayer),
					itsItemDisplayerUserdata(m.itsItemDisplayerUserdata),
					itsGetStringFunction(m.itsGetStringFunction),
					itsGetStringFunctionUserData(m.itsGetStringFunctionUserData),
					itsOptionsPtr(m.itsOptionsPtr),
					itsBeginning(m.itsBeginning),
					itsHighlight(m.itsHighlight),
					itsHighlightColor(m.itsHighlightColor),
					highlightEnabled(m.highlightEnabled),
					itsSelectedPrefix(m.itsSelectedPrefix),
					itsSelectedSuffix(m.itsSelectedSuffix)
{
	itsOptions.reserve(m.itsOptions.size());
	for (option_const_iterator it = m.itsOptions.begin(); it != m.itsOptions.end(); it++)
		itsOptions.push_back(new Option(**it));
}

template <typename T> NCurses::Menu<T>::~Menu()
{
	for (option_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
}

template <typename T> void NCurses::Menu<T>::Reserve(size_t size)
{
	itsOptions.reserve(size);
}

template <typename T> void NCurses::Menu<T>::ResizeList(size_t size)
{
	if (size > itsOptions.size())
	{
		itsOptions.resize(size);
		for (size_t i = 0; i < size; i++)
			if (!itsOptions[i])
				itsOptions[i] = new Option();
	}
	else if (size < itsOptions.size())
	{
		for (size_t i = size; i < itsOptions.size(); i++)
			delete itsOptions[i];
		itsOptions.resize(size);
	}
}

template <typename T> void NCurses::Menu<T>::AddOption(const T &item, bool is_bold, bool is_static)
{
	itsOptions.push_back(new Option(item, is_bold, is_static));
}

template <typename T> void NCurses::Menu<T>::AddSeparator()
{
	itsOptions.push_back(static_cast<Option *>(0));
}

template <typename T> void NCurses::Menu<T>::InsertOption(size_t pos, const T &item, bool is_bold, bool is_static)
{
	itsOptions.insert(itsOptions.begin()+pos, new Option(item, is_bold, is_static));
}

template <typename T> void NCurses::Menu<T>::InsertSeparator(size_t pos)
{
	itsOptions.insert(itsOptions.begin()+pos, 0);
}

template <typename T> void NCurses::Menu<T>::DeleteOption(size_t pos)
{
	if (itsOptionsPtr->empty())
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
	itsFound.clear();
	if (itsOptionsPtr->empty())
		Window::Clear();
}

template <typename T> void NCurses::Menu<T>::IntoSeparator(size_t pos)
{
	delete itsOptionsPtr->at(pos);
	(*itsOptionsPtr)[pos] = 0;
}

template <typename T> void NCurses::Menu<T>::BoldOption(int index, bool bold)
{
	if (!itsOptionsPtr->at(index))
		return;
	(*itsOptionsPtr)[index]->isBold = bold;
}

template <typename T> void NCurses::Menu<T>::Swap(size_t one, size_t two)
{
	std::swap(itsOptions.at(one), itsOptions.at(two));
}

template <typename T> void NCurses::Menu<T>::Move(size_t from, size_t to)
{
	int diff = from-to;
	if (diff > 0)
	{
		for (size_t i = from; i > to; i--)
			std::swap(itsOptions.at(i), itsOptions.at(i-1));
	}
	else if (diff < 0)
	{
		for (size_t i = from; i < to; i++)
			std::swap(itsOptions.at(i), itsOptions.at(i+1));
	}
}

template <typename T> void NCurses::Menu<T>::Refresh()
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

template <typename T> void NCurses::Menu<T>::Scroll(Where where)
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
			}
			if (itsHighlight == 0)
			{
				if (useCyclicScrolling)
					return Scroll(wEnd);
				break;
			}
			else
			{
				itsHighlight--;
			}
			if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == 0 && !useCyclicScrolling ? wDown : wUp);
			}
			break;
		}
		case wDown:
		{
			if (itsHighlight >= MaxCurrentHighlight && itsHighlight < MaxHighlight)
			{
				itsBeginning++;
			}
			if (itsHighlight == MaxHighlight)
			{
				if (useCyclicScrolling)
					return Scroll(wHome);
				break;
			}
			else
			{
				itsHighlight++;
			}
			if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == MaxHighlight && !useCyclicScrolling ? wUp : wDown);
			}
			break;
		}
		case wPageUp:
		{
			if (useCyclicScrolling && itsHighlight == 0)
				return Scroll(wEnd);
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
				Scroll(itsHighlight == 0 && !useCyclicScrolling ? wDown : wUp);
			}
			break;
		}
		case wPageDown:
		{
			if (useCyclicScrolling && itsHighlight == MaxHighlight)
				return Scroll(wHome);
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
				Scroll(itsHighlight == MaxHighlight && !useCyclicScrolling ? wUp : wDown);
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

template <typename T> void NCurses::Menu<T>::Reset()
{
	itsHighlight = 0;
	itsBeginning = 0;
}

template <typename T> void NCurses::Menu<T>::ClearFiltered()
{
	itsFilteredOptions.clear();
	itsFilteredRealPositions.clear();
	itsOptionsPtr = &itsOptions;
}

template <typename T> void NCurses::Menu<T>::Clear(bool clrscr)
{
	for (option_iterator it = itsOptions.begin(); it != itsOptions.end(); it++)
		delete *it;
	itsOptions.clear();
	itsFound.clear();
	itsFilter.clear();
	ClearFiltered();
	itsOptionsPtr = &itsOptions;
	if (clrscr)
		Window::Clear();
}

template <typename T> bool NCurses::Menu<T>::isBold(int id)
{
	id = id == -1 ? itsHighlight : id;
	if (!itsOptionsPtr->at(id))
		return 0;
	return (*itsOptionsPtr)[id]->isBold;
}

template <typename T> void NCurses::Menu<T>::Select(int id, bool value)
{
	if (!itsOptionsPtr->at(id))
		return;
	(*itsOptionsPtr)[id]->isSelected = value;
}

template <typename T> void NCurses::Menu<T>::Static(int id, bool value)
{
	if (!itsOptionsPtr->at(id))
		return;
	(*itsOptionsPtr)[id]->isStatic = value;
}

template <typename T> bool NCurses::Menu<T>::isSelected(int id) const
{
	id = id == -1 ? itsHighlight : id;
	if (!itsOptionsPtr->at(id))
		return 0;
	return (*itsOptionsPtr)[id]->isSelected;
}

template <typename T> bool NCurses::Menu<T>::isStatic(int id) const
{
	id = id == -1 ? itsHighlight : id;
	if (!itsOptionsPtr->at(id))
		return 1;
	return (*itsOptionsPtr)[id]->isStatic;
}

template <typename T> bool NCurses::Menu<T>::hasSelected() const
{
	for (option_const_iterator it = itsOptionsPtr->begin(); it != itsOptionsPtr->end(); it++)
		if (*it && (*it)->isSelected)
			return true;
	return false;
}

template <typename T> void NCurses::Menu<T>::GetSelected(std::vector<size_t> &v) const
{
	for (size_t i = 0; i < itsOptionsPtr->size(); i++)
		if ((*itsOptionsPtr)[i] && (*itsOptionsPtr)[i]->isSelected)
			v.push_back(i);
}

template <typename T> void NCurses::Menu<T>::Highlight(size_t pos)
{
	itsHighlight = pos;
	itsBeginning = pos-itsHeight/2;
}

template <typename T> size_t NCurses::Menu<T>::Size() const
{
	return itsOptionsPtr->size();
}

template <typename T> size_t NCurses::Menu<T>::Choice() const
{
	return itsHighlight;
}

template <typename T> size_t NCurses::Menu<T>::RealChoice() const
{
	size_t result = 0;
	for (option_const_iterator it = itsOptionsPtr->begin(); it != itsOptionsPtr->begin()+itsHighlight; it++)
		if (*it && !(*it)->isStatic)
			result++;
	return result;
}

template <typename T> bool NCurses::Menu<T>::Search(const std::string &constraint, size_t beginning, int flags)
{
	itsFound.clear();
	itsSearchConstraint.clear();
	if (constraint.empty())
		return false;
	itsSearchConstraint = constraint;
	regex_t rx;
	if (regcomp(&rx, itsSearchConstraint.c_str(), flags) == 0)
	{
		for (size_t i = beginning; i < itsOptionsPtr->size(); i++)
		{
			if (regexec(&rx, GetOption(i).c_str(), 0, 0, 0) == 0)
				itsFound.insert(i);
		}
	}
	regfree(&rx);
	return !itsFound.empty();
}

template <typename T> void NCurses::Menu<T>::NextFound(bool wrap)
{
	if (itsFound.empty())
		return;
	std::set<size_t>::iterator next = itsFound.upper_bound(itsHighlight);
	if (next != itsFound.end())
		Highlight(*next);
	else if (wrap)
		Highlight(*itsFound.begin());
}

template <typename T> void NCurses::Menu<T>::PrevFound(bool wrap)
{
	if (itsFound.empty())
		return;
	std::set<size_t>::iterator prev = itsFound.lower_bound(itsHighlight);
	if (prev != itsFound.begin())
		Highlight(*--prev);
	else if (wrap)
		Highlight(*itsFound.rbegin());
}

template <typename T> void NCurses::Menu<T>::ApplyFilter(const std::string &filter, size_t beginning, int flags)
{
	itsFound.clear();
	ClearFiltered();
	itsFilter = filter;
	if (itsFilter.empty())
		return;
	for (size_t i = 0; i < beginning; i++)
	{
		itsFilteredRealPositions.push_back(i);
		itsFilteredOptions.push_back(itsOptions[i]);
	}
	regex_t rx;
	if (regcomp(&rx, itsFilter.c_str(), flags) == 0)
	{
		for (size_t i = beginning; i < itsOptions.size(); i++)
		{
			if (regexec(&rx, GetOption(i).c_str(), 0, 0, 0) == 0)
			{
				itsFilteredRealPositions.push_back(i);
				itsFilteredOptions.push_back(itsOptions[i]);
			}
		}
	}
	regfree(&rx);
	itsOptionsPtr = &itsFilteredOptions;
	if (itsOptionsPtr->empty()) // oops, we didn't find anything
		Window::Clear();
}

template <typename T> const std::string &NCurses::Menu<T>::GetFilter()
{
	return itsFilter;
}

template <typename T> std::string NCurses::Menu<T>::GetOption(size_t pos)
{
	if (itsOptionsPtr->at(pos) && itsGetStringFunction)
		return itsGetStringFunction((*itsOptionsPtr)[pos]->Item, itsGetStringFunctionUserData);
	else
		return "";
}

template <typename T> T &NCurses::Menu<T>::Back()
{
	if (!itsOptionsPtr->back())
		throw InvalidItem();
	return itsOptionsPtr->back()->Item;
}

template <typename T> const T &NCurses::Menu<T>::Back() const
{
	if (!itsOptionsPtr->back())
		throw InvalidItem();
	return itsOptionsPtr->back()->Item;
}

template <typename T> T &NCurses::Menu<T>::Current()
{
	if (!itsOptionsPtr->at(itsHighlight))
		throw InvalidItem();
	return (*itsOptionsPtr)[itsHighlight]->Item;
}

template <typename T> const T &NCurses::Menu<T>::Current() const
{
	if (!itsOptionsPtr->at(itsHighlight))
		throw InvalidItem();
	return (*itsOptionsPtr)[itsHighlight]->Item;
}

template <typename T> T &NCurses::Menu<T>::at(size_t i)
{
	if (!itsOptionsPtr->at(i))
		throw InvalidItem();
	return (*itsOptionsPtr)[i]->Item;
}

template <typename T> const T &NCurses::Menu<T>::at(size_t i) const
{
	if (!itsOptions->at(i))
		throw InvalidItem();
	return (*itsOptionsPtr)[i]->Item;
}

template <typename T> const T &NCurses::Menu<T>::operator[](size_t i) const
{
	if (!(*itsOptionsPtr)[i])
		throw InvalidItem();
	return (*itsOptionsPtr)[i]->Item;
}

template <typename T> T &NCurses::Menu<T>::operator[](size_t i)
{
	if (!(*itsOptionsPtr)[i])
		throw InvalidItem();
	return (*itsOptionsPtr)[i]->Item;
}

template <typename T> NCurses::Menu<T> *NCurses::Menu<T>::EmptyClone() const
{
	return new NCurses::Menu<T>(GetStartX(), GetStartY(), GetWidth(), GetHeight(), itsTitle, itsBaseColor, itsBorder);
}

#endif

