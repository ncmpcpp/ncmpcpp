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

#ifndef _MENU_H
#define _MENU_H

#include <regex.h>
#include <algorithm>
#include <set>

#include "error.h"
#include "window.h"
#include "strbuffer.h"

namespace NCurses
{
	/// List class is an interface for Menu class
	///
	class List
	{
		public:
			/// @see Menu::Select()
			///
			virtual void Select(int pos, bool state) = 0;
			
			/// @see Menu::isSelected()
			///
			virtual bool isSelected(int pos = -1) const = 0;
			
			/// @see Menu::hasSelected()
			///
			virtual bool hasSelected() const = 0;
			
			/// @see Menu::GetSelected()
			///
			virtual void GetSelected(std::vector<size_t> &v) const = 0;
			
			/// Highlights given position
			/// @param pos position to be highlighted
			///
			virtual void Highlight(size_t pos) = 0;
			
			/// @return currently highlighted position
			///
			virtual size_t Choice() const = 0;
			
			/// @see Menu::Empty()
			///
			virtual bool Empty() const = 0;
			
			/// @see Menu::Size()
			///
			virtual size_t Size() const = 0;
			
			/// @see Menu::Search()
			///
			virtual bool Search(const std::string &constraint, size_t beginning = 0, int flags = 0) = 0;
			
			/// @see Menu::GetSearchConstraint()
			///
			virtual const std::string &GetSearchConstraint() = 0;
			
			/// @see Menu::NextFound()
			///
			virtual void NextFound(bool wrap) = 0;
			
			/// @see Menu::PrevFound()
			///
			virtual void PrevFound(bool wrap) = 0;
			
			/// @see Menu::ApplyFilter()
			///
			virtual void ApplyFilter(const std::string &filter, size_t beginning = 0, int flags = 0) = 0;
			
			/// @see Menu::GetFilter()
			///
			virtual const std::string &GetFilter() = 0;
			
			/// @see Menu::isFiltered()
			///
			virtual bool isFiltered() = 0;
	};
	
	/// This template class is generic menu, that has holds
	/// any values that are std::vector compatible.
	///
	template <typename T> class Menu : public Window, public List
	{
		/// Function helper prototype used to display each option on the screen.
		/// If not set by SetItemDisplayer(), menu won't display anything.
		/// @see SetItemDisplayer()
		///
		typedef void (*ItemDisplayer)(const T &, void *, Menu<T> *);
		
		/// Function helper prototype used for converting items to strings.
		/// If not set by SetGetStringFunction(), searching and filtering
		/// won't work (note that Menu<std::string> doesn't need this)
		/// @see SetGetStringFunction()
		///
		typedef std::string (*GetStringFunction)(const T &, void *);
		
		/// Struct that holds each item in the list and its attributes
		///
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
		
		/// Functor that wraps around the functor passed to Sort()
		/// to fit to internal container structure
		///
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
			
			/// Constructs an empty menu with given parameters
			/// @param startx X position of left upper corner of constructed menu
			/// @param starty Y position of left upper corner of constructed menu
			/// @param width width of constructed menu
			/// @param height height of constructed menu
			/// @param title title of constructed menu
			/// @param color base color of constructed menu
			/// @param border border of constructed menu
			///
			Menu(size_t startx, size_t starty, size_t width, size_t height,
			     const std::string &title, Color color, Border border);
			
			/// Copies the menu
			/// @param m copied menu
			///
			Menu(const Menu &m);
			
			/// Destroys the object and frees memory
			///
			virtual ~Menu();
			
			/// Sets helper function that is responsible for displaying items
			/// @param ptr function pointer that matches the ItemDisplayer prototype
			///
			void SetItemDisplayer(ItemDisplayer ptr) { itsItemDisplayer = ptr; }
			
			/// Sets optional user data, that is passed to
			/// ItemDisplayer function each time it's invoked
			/// @param data void pointer to userdata
			///
			void SetItemDisplayerUserData(void *data) { itsItemDisplayerUserdata = data; }
			
			/// Sets helper function that is responsible for converting items to strings
			/// @param f function pointer that matches the GetStringFunction prototype
			///
			void SetGetStringFunction(GetStringFunction f) { itsGetStringFunction = f; }
			
			/// Sets optional user data, that is passed to
			/// GetStringFunction function each time it's invoked
			/// @param data void pointer to user data
			///
			void SetGetStringFunctionUserData(void *data) { itsGetStringFunctionUserData = data; }
			
			/// Reserves the size for internal container (this just calls std::vector::reserve())
			/// @param size requested size
			///
			void Reserve(size_t size);
			
			/// Resizes the list to given size (adequate to std::vector::resize())
			/// @param size requested size
			///
			void ResizeList(size_t size);
			
			/// Adds new option to list
			/// @param item object that has to be added
			/// @param is_bold defines the initial state of bold attribute
			/// @param is_static defines the initial state of static attribute
			///
			void AddOption(const T &item, bool is_bold = 0, bool is_static = 0);
			
			/// Adds separator to list
			///
			void AddSeparator();
			
			/// Inserts new option to list at given position
			/// @param pos initial position of inserted item
			/// @param item object that has to be inserted
			/// @param is_bold defines the initial state of bold attribute
			/// @param is_static defines the initial state of static attribute
			///
			void InsertOption(size_t pos, const T &Item, bool is_bold = 0, bool is_static = 0);
			
			/// Inserts separator to list at given position
			/// @param pos initial position of inserted separator
			///
			void InsertSeparator(size_t pos);
			
			/// Deletes item from given position
			/// @param pos given position of item to be deleted
			///
			void DeleteOption(size_t pos);
			
			/// Converts the option into separator
			/// @param pos position of item to be converted
			///
			void IntoSeparator(size_t pos);
			
			/// Swaps the content of two items
			/// @param one position of first item
			/// @param two position of second item
			///
			void Swap(size_t one, size_t two);
			
			/// Moves requested item from one position to another
			/// @param from the position of item that has to be moved
			/// @param to the position that indicates where the object has to be moved
			///
			void Move(size_t from, size_t to);
			
			/// Moves the highlighted position to the given line of window
			/// @param y Y position of menu window to be highlighted
			/// @return true if the position is reachable, false otherwise
			///
			bool Goto(size_t y);
			
			/// Checks if the given position has bold attribute set.
			/// @param pos position to be checked. -1 = currently highlighted position
			/// @return true if the bold is set, false otherwise
			///
			bool isBold(int pos = -1);
			
			/// Sets bols attribute for given position
			/// @param pos position of item to be bolded/unbolded
			/// @param state state of bold attribute
			///
			void Bold(int pos, bool state);
			
			/// Makes given position static/active.
			/// Static positions cannot be highlighted.
			/// @param pos position in list
			/// @param state state of activity
			///
			void Static(int pos, bool state);
			
			/// Checks whether given position is static or active
			/// @param pos position to be checked, -1 checks currently highlighted position
			/// @return true if position is static, false otherwise
			///
			bool isStatic(int pos = -1) const;
			
			/// Checks whether given position is separator
			/// @param pos position to be checked, -1 checks currently highlighted position
			/// @return true if position is separator, false otherwise
			///
			bool isSeparator(int pos = -1) const;
			
			/// Selects/deselects given position
			/// @param pos position in list
			/// @param state state of selection
			///
			virtual void Select(int pos, bool state);
			
			/// Checks if given position is selected
			/// @param pos position to be checked, -1 checks currently highlighted position
			/// @return true if position is selected, false otherwise
			///
			virtual bool isSelected(int pos = -1) const;
			
			/// Checks whether list contains selected positions
			/// @return true if it contains them, false otherwise
			///
			virtual bool hasSelected() const;
			
			/// Gets positions of items that are selected
			/// @param v vector to be filled with selected positions numbers
			///
			virtual void GetSelected(std::vector<size_t> &v) const;
			
			/// Reverses selection of all items in list
			/// @param beginning beginning of range that has to be reversed
			///
			void ReverseSelection(size_t beginning = 0);
			
			/// Highlights given position
			/// @param pos position to be highlighted
			///
			void Highlight(size_t pos);
			
			/// @return currently highlighted position
			///
			size_t Choice() const;
			
			/// @return real current positions, i.e it doesn't
			/// count positions that are static or separators
			///
			size_t RealChoice() const;
			
			/// Searches the list for a given contraint. It uses GetStringFunction to convert stored items
			/// into strings and then performs pattern matching. Note that this supports regular expressions.
			/// @param constraint a search constraint to be used
			/// @param beginning beginning of range that has to be searched through
			/// @param flags regex flags (REG_EXTENDED, REG_ICASE, REG_NOSUB, REG_NEWLINE)
			/// @return true if at least one item matched the given pattern, false otherwise
			///
			virtual bool Search(const std::string &constraint, size_t beginning = 0, int flags = 0);
			
			/// @return const reference to currently used search constraint
			///
			virtual const std::string &GetSearchConstraint() { return itsSearchConstraint; }
			
			/// Moves current position in the list to the next found one
			/// @param wrap if true, this function will go to the first
			/// found pos after the last one, otherwise it'll do nothing.
			///
			virtual void NextFound(bool wrap);
			
			/// Moves current position in the list to the previous found one
			/// @param wrap if true, this function will go to the last
			/// found pos after the first one, otherwise it'll do nothing.
			///
			virtual void PrevFound(bool wrap);
			
			/// Filters the list, showing only the items that matches the pattern. It uses
			/// GetStringFunction to convert stored items into strings and then performs
			/// pattern matching. Note that this supports regular expressions.
			/// @param filter a pattern to be used in pattern matching
			/// @param beginning beginning of range that has to be filtered
			/// @param flags regex flags (REG_EXTENDED, REG_ICASE, REG_NOSUB, REG_NEWLINE)
			///
			virtual void ApplyFilter(const std::string &filter, size_t beginning = 0, int flags = 0);
			
			/// @return const reference to currently used filter
			///
			virtual const std::string &GetFilter();
			
			/// @return true if list is currently filtered, false otherwise
			///
			virtual bool isFiltered() { return itsOptionsPtr == &itsFilteredOptions; }
			
			/// Turns off filtering
			///
			void ShowAll() { itsOptionsPtr = &itsOptions; }
			
			/// Turns on filtering
			///
			void ShowFiltered() { itsOptionsPtr = &itsFilteredOptions; }
			
			/// Converts given position in list to string using GetStringFunction
			/// if specified and an empty string otherwise
			/// @param pos position to be converted
			/// @return item converted to string
			/// @see SetItemDisplayer()
			///
			std::string GetOption(size_t pos);
			
			/// Refreshes the menu window
			/// @see Window::Refresh()
			///
			virtual void Refresh();
			
			/// Scrolls by given amount of lines
			/// @param where indicated where exactly one wants to go
			/// @see Window::Scroll()
			///
			virtual void Scroll(Where where);
			
			/// Cleares all options, used filters etc. It doesn't reset highlighted position though.
			/// @see Reset()
			///
			virtual void Clear();
			
			/// Sets the highlighted position to 0
			///
			void Reset();
			
			/// Sorts all items using Comparison object with defined operator()
			/// @param beginning beginning of range that has to be sorted
			///
			template <typename Comparison> void Sort(size_t beginning = 0, size_t end = -1)
			{
				if (itsOptions.empty())
					return;
				sort(itsOptions.begin()+beginning, end == size_t(-1) ? itsOptions.end() : itsOptions.begin()+end, InternalSorting<Comparison>());
				if (isFiltered())
					ApplyFilter(itsFilter);
			}
			
			/// Sets prefix, that is put before each selected item to indicate its selection
			/// Note that the passed variable is not deleted along with menu object.
			/// @param b pointer to buffer that contains the prefix
			///
			void SetSelectPrefix(Buffer *b) { itsSelectedPrefix = b; }
			
			/// Sets suffix, that is put after each selected item to indicate its selection
			/// Note that the passed variable is not deleted along with menu object.
			/// @param b pointer to buffer that contains the suffix
			///
			void SetSelectSuffix(Buffer *b) { itsSelectedSuffix = b; }
			
			/// Sets custom color of highlighted position
			/// @param col custom color
			///
			void HighlightColor(Color color) { itsHighlightColor = color; }
			
			/// @return state of highlighting
			///
			bool isHighlighted() { return highlightEnabled; }
			
			/// Turns on/off highlighting
			/// @param state state of hihglighting
			///
			void Highlighting(bool state) { highlightEnabled = state; }
			
			/// Turns on/off cyclic scrolling
			/// @param state state of cyclic scrolling
			///
			void CyclicScrolling(bool state) { useCyclicScrolling = state; }
			
			/// Turns on/off centered cursor
			/// @param state state of centered cursor
			///
			void CenteredCursor(bool state) { useCenteredCursor = state; }
			
			/// Checks if list is empty
			/// @return true if list is empty, false otherwise
			/// @see ReallyEmpty()
			///
			virtual bool Empty() const { return itsOptionsPtr->empty(); }
			
			/// Checks if list is really empty since Empty() may not
			/// be accurate if filter is set)
			/// @return true if list is empty, false otherwise
			/// @see Empty()
			///
			virtual bool ReallyEmpty() const { return itsOptions.empty(); }
			
			/// @return size of the list
			///
			virtual size_t Size() const;
			
			/// @return position of currently drawed item. The result is
			/// defined only within drawing function that is called by Refresh()
			/// @see Refresh()
			///
			size_t CurrentlyDrawedPosition() const { return itsCurrentlyDrawedPosition; }
			
			/// @return reference to last item on the list
			/// @throw List::InvalidItem if requested item is separator
			///
			T &Back();
			
			/// @return const reference to last item on the list
			/// @throw List::InvalidItem if requested item is separator
			///
			const T &Back() const;
			
			/// @return reference to curently highlighted object
			/// @throw List::InvalidItem if requested item is separator
			///
			T &Current();
			
			/// @return const reference to curently highlighted object
			/// @throw List::InvalidItem if requested item is separator
			///
			const T &Current() const;
			
			/// @param pos requested position
			/// @return reference to item at given position
			/// @throw std::out_of_range if given position is out of range
			/// @throw List::InvalidItem if requested item is separator
			///
			T &at(size_t pos);
			
			/// @param pos requested position
			/// @return const reference to item at given position
			/// @throw std::out_of_range if given position is out of range
			/// @throw List::InvalidItem if requested item is separator
			///
			const T &at(size_t pos) const;
			
			/// @param pos requested position
			/// @return const reference to item at given position
			/// @throw List::InvalidItem if requested item is separator
			///
			const T &operator[](size_t pos) const;
			
			/// @param pos requested position
			/// @return const reference to item at given position
			/// @throw List::InvalidItem if requested item is separator
			///
			T &operator[](size_t pos);
		
		protected:
			/// Clears filter, filtered data etc.
			///
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
			
			bool useCenteredCursor;
			
			size_t itsCurrentlyDrawedPosition;
			
			Buffer *itsSelectedPrefix;
			Buffer *itsSelectedSuffix;
	};
	
	/// Specialization of Menu<T>::GetOption for T = std::string, it's obvious
	/// that if strings are stored, we don't need extra function to convert
	/// them to strings by default
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
					useCenteredCursor(0),
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
					useCyclicScrolling(m.useCyclicScrolling),
					useCenteredCursor(m.useCenteredCursor),
					itsSelectedPrefix(m.itsSelectedPrefix),
					itsSelectedSuffix(m.itsSelectedSuffix)
{
	itsOptions.reserve(m.itsOptions.size());
	for (option_const_iterator it = m.itsOptions.begin(); it != m.itsOptions.end(); ++it)
		itsOptions.push_back(new Option(**it));
}

template <typename T> NCurses::Menu<T>::~Menu()
{
	for (option_iterator it = itsOptions.begin(); it != itsOptions.end(); ++it)
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
		for (size_t i = 0; i < size; ++i)
			if (!itsOptions[i])
				itsOptions[i] = new Option();
	}
	else if (size < itsOptions.size())
	{
		for (size_t i = size; i < itsOptions.size(); ++i)
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
		for (size_t i = pos; i < itsFilteredRealPositions.size(); ++i)
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

template <typename T> void NCurses::Menu<T>::Bold(int pos, bool state)
{
	if (!itsOptionsPtr->at(pos))
		return;
	(*itsOptionsPtr)[pos]->isBold = state;
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
		for (size_t i = from; i > to; --i)
			std::swap(itsOptions.at(i), itsOptions.at(i-1));
	}
	else if (diff < 0)
	{
		for (size_t i = from; i < to; ++i)
			std::swap(itsOptions.at(i), itsOptions.at(i+1));
	}
}

template <typename T> bool NCurses::Menu<T>::Goto(size_t y)
{
	if (!itsOptionsPtr->at(itsBeginning+y) || itsOptionsPtr->at(itsBeginning+y)->isStatic)
		return false;
	itsHighlight = itsBeginning+y;
	return true;
}

template <typename T> void NCurses::Menu<T>::Refresh()
{
	if (itsOptionsPtr->empty())
	{
		Window::Refresh();
		return;
	}
	int MaxBeginning = itsOptionsPtr->size() < itsHeight ? 0 : itsOptionsPtr->size()-itsHeight;
	
	if (itsBeginning < itsHighlight-int(itsHeight)+1) // highlighted position is off the screen
		itsBeginning = itsHighlight-itsHeight+1;
	
	if (itsBeginning < 0)
		itsBeginning = 0;
	else if (itsBeginning > MaxBeginning)
		itsBeginning = MaxBeginning;
	
	if (!itsOptionsPtr->empty() && itsHighlight > int(itsOptionsPtr->size())-1)
		itsHighlight = itsOptionsPtr->size()-1;
	
	if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic) // it shouldn't be here
	{
		Scroll(wUp);
		if (!(*itsOptionsPtr)[itsHighlight] || (*itsOptionsPtr)[itsHighlight]->isStatic)
			Scroll(wDown);
	}
	
	size_t line = 0;
	for (size_t &i = (itsCurrentlyDrawedPosition = itsBeginning); i < itsBeginning+itsHeight; ++i)
	{
		GotoXY(0, line);
		if (i >= itsOptionsPtr->size())
		{
			for (; line < itsHeight; ++line)
				mvwhline(itsWindow, line, 0, 32, itsWidth);
			break;
		}
		if (!(*itsOptionsPtr)[i]) // separator
		{
			mvwhline(itsWindow, line++, 0, 0, itsWidth);
			continue;
		}
		if ((*itsOptionsPtr)[i]->isBold)
			*this << fmtBold;
		if (highlightEnabled && int(i) == itsHighlight)
		{
			*this << fmtReverse;
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
			*this << fmtReverseEnd;
		}
		if ((*itsOptionsPtr)[i]->isBold)
			*this << fmtBoldEnd;
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
	if (useCenteredCursor)
		Highlight(itsHighlight);
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

template <typename T> void NCurses::Menu<T>::Clear()
{
	for (option_iterator it = itsOptions.begin(); it != itsOptions.end(); ++it)
		delete *it;
	itsOptions.clear();
	itsFound.clear();
	itsFilter.clear();
	ClearFiltered();
	itsOptionsPtr = &itsOptions;
	Window::Clear();
}

template <typename T> bool NCurses::Menu<T>::isBold(int pos)
{
	pos = pos == -1 ? itsHighlight : pos;
	if (!itsOptionsPtr->at(pos))
		return 0;
	return (*itsOptionsPtr)[pos]->isBold;
}

template <typename T> void NCurses::Menu<T>::Select(int pos, bool state)
{
	if (!itsOptionsPtr->at(pos))
		return;
	(*itsOptionsPtr)[pos]->isSelected = state;
}

template <typename T> void NCurses::Menu<T>::Static(int pos, bool state)
{
	if (!itsOptionsPtr->at(pos))
		return;
	(*itsOptionsPtr)[pos]->isStatic = state;
}

template <typename T> bool NCurses::Menu<T>::isSelected(int pos) const
{
	pos = pos == -1 ? itsHighlight : pos;
	if (!itsOptionsPtr->at(pos))
		return 0;
	return (*itsOptionsPtr)[pos]->isSelected;
}

template <typename T> bool NCurses::Menu<T>::isStatic(int pos) const
{
	pos = pos == -1 ? itsHighlight : pos;
	if (!itsOptionsPtr->at(pos))
		return 1;
	return (*itsOptionsPtr)[pos]->isStatic;
}

template <typename T> bool NCurses::Menu<T>::isSeparator(int pos) const
{
	pos = pos == -1 ? itsHighlight : pos;
	return !itsOptionsPtr->at(pos);
}

template <typename T> bool NCurses::Menu<T>::hasSelected() const
{
	for (option_const_iterator it = itsOptionsPtr->begin(); it != itsOptionsPtr->end(); ++it)
		if (*it && (*it)->isSelected)
			return true;
	return false;
}

template <typename T> void NCurses::Menu<T>::GetSelected(std::vector<size_t> &v) const
{
	for (size_t i = 0; i < itsOptionsPtr->size(); ++i)
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
	for (option_const_iterator it = itsOptionsPtr->begin(); it != itsOptionsPtr->begin()+itsHighlight; ++it)
		if (*it && !(*it)->isStatic)
			result++;
	return result;
}

template <typename T> void NCurses::Menu<T>::ReverseSelection(size_t beginning)
{
	option_iterator it = itsOptionsPtr->begin()+beginning;
	for (size_t i = beginning; i < Size(); ++i, ++it)
		if (*it)
			(*it)->isSelected = !(*it)->isSelected && !(*it)->isStatic;
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
		for (size_t i = beginning; i < itsOptionsPtr->size(); ++i)
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
	for (size_t i = 0; i < beginning; ++i)
	{
		itsFilteredRealPositions.push_back(i);
		itsFilteredOptions.push_back(itsOptions[i]);
	}
	regex_t rx;
	if (regcomp(&rx, itsFilter.c_str(), flags) == 0)
	{
		for (size_t i = beginning; i < itsOptions.size(); ++i)
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
		FatalError("Menu::Back() has requested separator!");
	return itsOptionsPtr->back()->Item;
}

template <typename T> const T &NCurses::Menu<T>::Back() const
{
	if (!itsOptionsPtr->back())
		FatalError("Menu::Back() has requested separator!");
	return itsOptionsPtr->back()->Item;
}

template <typename T> T &NCurses::Menu<T>::Current()
{
	if (!itsOptionsPtr->at(itsHighlight))
		FatalError("Menu::Current() has requested separator!");
	return (*itsOptionsPtr)[itsHighlight]->Item;
}

template <typename T> const T &NCurses::Menu<T>::Current() const
{
	if (!itsOptionsPtr->at(itsHighlight))
		FatalError("Menu::Current() const has requested separator!");
	return (*itsOptionsPtr)[itsHighlight]->Item;
}

template <typename T> T &NCurses::Menu<T>::at(size_t pos)
{
	if (!itsOptionsPtr->at(pos))
		FatalError("Menu::at() has requested separator!");
	return (*itsOptionsPtr)[pos]->Item;
}

template <typename T> const T &NCurses::Menu<T>::at(size_t pos) const
{
	if (!itsOptions->at(pos))
		FatalError("Menu::at() const has requested separator!");
	return (*itsOptionsPtr)[pos]->Item;
}

template <typename T> const T &NCurses::Menu<T>::operator[](size_t pos) const
{
	if (!(*itsOptionsPtr)[pos])
		FatalError("Menu::operator[] const has requested separator!");
	return (*itsOptionsPtr)[pos]->Item;
}

template <typename T> T &NCurses::Menu<T>::operator[](size_t pos)
{
	if (!(*itsOptionsPtr)[pos])
		FatalError("Menu::operator[] has requested separator!");
	return (*itsOptionsPtr)[pos]->Item;
}

#endif

