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
#include <functional>
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
	
	/// This template class is generic menu capable of
	/// holding any std::vector compatible values.
	///
	template <typename T> class Menu : public Window, public List
	{
		/// Function helper prototype used to display each option on the screen.
		/// If not set by setItemDisplayer(), menu won't display anything.
		/// @see setItemDisplayer()
		///
		typedef std::function<void(Menu<T> &, const T &)> ItemDisplayer;
		
		/// Function helper prototype used for converting items to strings.
		/// If not set by SetItemStringifier(), searching and filtering
		/// won't work (note that Menu<std::string> doesn't need this)
		/// @see SetItemStringifier()
		///
		typedef std::function<std::string(const T &)> ItemStringifier;
		
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
			void setItemDisplayer(ItemDisplayer ptr) { m_item_displayer = ptr; }
			
			/// Sets helper function that is responsible for converting items to strings
			/// @param f function pointer that matches the ItemStringifier prototype
			///
			void SetItemStringifier(ItemStringifier f) { m_get_string_helper = f; }
			
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
			
			/// Searches the list for a given contraint. It uses ItemStringifier to convert stored items
			/// into strings and then performs pattern matching. Note that this supports regular expressions.
			/// @param constraint a search constraint to be used
			/// @param beginning beginning of range that has to be searched through
			/// @param flags regex flags (REG_EXTENDED, REG_ICASE, REG_NOSUB, REG_NEWLINE)
			/// @return true if at least one item matched the given pattern, false otherwise
			///
			virtual bool Search(const std::string &constraint, size_t beginning = 0, int flags = 0);
			
			/// @return const reference to currently used search constraint
			///
			virtual const std::string &GetSearchConstraint() { return m_search_constraint; }
			
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
			/// ItemStringifier to convert stored items into strings and then performs
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
			virtual bool isFiltered() { return m_options_ptr == &m_filtered_options; }
			
			/// Turns off filtering
			///
			void ShowAll() { m_options_ptr = &m_options; }
			
			/// Turns on filtering
			///
			void ShowFiltered() { m_options_ptr = &m_filtered_options; }
			
			/// Converts given position in list to string using ItemStringifier
			/// if specified and an empty string otherwise
			/// @param pos position to be converted
			/// @return item converted to string
			/// @see setItemDisplayer()
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
				if (m_options.empty())
					return;
				sort(m_options.begin()+beginning, end == size_t(-1) ? m_options.end() : m_options.begin()+end, InternalSorting<Comparison>());
				if (isFiltered())
					ApplyFilter(m_filter);
			}
			
			/// Sets prefix, that is put before each selected item to indicate its selection
			/// Note that the passed variable is not deleted along with menu object.
			/// @param b pointer to buffer that contains the prefix
			///
			void SetSelectPrefix(Buffer *b) { m_selected_prefix = b; }
			
			/// Sets suffix, that is put after each selected item to indicate its selection
			/// Note that the passed variable is not deleted along with menu object.
			/// @param b pointer to buffer that contains the suffix
			///
			void SetSelectSuffix(Buffer *b) { m_selected_suffix = b; }
			
			/// Sets custom color of highlighted position
			/// @param col custom color
			///
			void HighlightColor(Color color) { m_highlight_color = color; }
			
			/// @return state of highlighting
			///
			bool isHighlighted() { return m_highlight_enabled; }
			
			/// Turns on/off highlighting
			/// @param state state of hihglighting
			///
			void Highlighting(bool state) { m_highlight_enabled = state; }
			
			/// Turns on/off cyclic scrolling
			/// @param state state of cyclic scrolling
			///
			void CyclicScrolling(bool state) { m_cyclic_scroll_enabled = state; }
			
			/// Turns on/off centered cursor
			/// @param state state of centered cursor
			///
			void CenteredCursor(bool state) { m_autocenter_cursor = state; }
			
			/// Checks if list is empty
			/// @return true if list is empty, false otherwise
			/// @see ReallyEmpty()
			///
			virtual bool Empty() const { return m_options_ptr->empty(); }
			
			/// Checks if list is really empty since Empty() may not
			/// be accurate if filter is set)
			/// @return true if list is empty, false otherwise
			/// @see Empty()
			///
			virtual bool ReallyEmpty() const { return m_options.empty(); }
			
			/// @return size of the list
			///
			virtual size_t Size() const;
			
			/// @return position of currently drawed item. The result is
			/// defined only within drawing function that is called by Refresh()
			/// @see Refresh()
			///
			size_t CurrentlyDrawedPosition() const { return m_drawn_position; }
			
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
			
			ItemDisplayer m_item_displayer;
			ItemStringifier m_get_string_helper;
			
			std::string m_filter;
			std::string m_search_constraint;
			
			std::vector<Option *> *m_options_ptr;
			std::vector<Option *> m_options;
			std::vector<Option *> m_filtered_options;
			std::vector<size_t> m_filtered_positions;
			std::set<size_t> m_found_positions;
			
			int itsBeginning;
			int itsHighlight;
			
			Color m_highlight_color;
			bool m_highlight_enabled;
			bool m_cyclic_scroll_enabled;
			
			bool m_autocenter_cursor;
			
			size_t m_drawn_position;
			
			Buffer *m_selected_prefix;
			Buffer *m_selected_suffix;
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
					m_item_displayer(0),
					m_get_string_helper(0),
					m_options_ptr(&m_options),
					itsBeginning(0),
					itsHighlight(0),
					m_highlight_color(itsBaseColor),
					m_highlight_enabled(1),
					m_cyclic_scroll_enabled(0),
					m_autocenter_cursor(0),
					m_selected_prefix(0),
					m_selected_suffix(0)
{
}

template <typename T> NCurses::Menu<T>::Menu(const Menu &m) : Window(m),
					m_item_displayer(m.m_item_displayer),
					m_get_string_helper(m.m_get_string_helper),
					m_options_ptr(m.m_options_ptr),
					itsBeginning(m.itsBeginning),
					itsHighlight(m.itsHighlight),
					m_highlight_color(m.m_highlight_color),
					m_highlight_enabled(m.m_highlight_enabled),
					m_cyclic_scroll_enabled(m.m_cyclic_scroll_enabled),
					m_autocenter_cursor(m.m_autocenter_cursor),
					m_selected_prefix(m.m_selected_prefix),
					m_selected_suffix(m.m_selected_suffix)
{
	m_options.reserve(m.m_options.size());
	for (option_const_iterator it = m.m_options.begin(); it != m.m_options.end(); ++it)
		m_options.push_back(new Option(**it));
}

template <typename T> NCurses::Menu<T>::~Menu()
{
	for (option_iterator it = m_options.begin(); it != m_options.end(); ++it)
		delete *it;
}

template <typename T> void NCurses::Menu<T>::Reserve(size_t size)
{
	m_options.reserve(size);
}

template <typename T> void NCurses::Menu<T>::ResizeList(size_t size)
{
	if (size > m_options.size())
	{
		m_options.resize(size);
		for (size_t i = 0; i < size; ++i)
			if (!m_options[i])
				m_options[i] = new Option();
	}
	else if (size < m_options.size())
	{
		for (size_t i = size; i < m_options.size(); ++i)
			delete m_options[i];
		m_options.resize(size);
	}
}

template <typename T> void NCurses::Menu<T>::AddOption(const T &item, bool is_bold, bool is_static)
{
	m_options.push_back(new Option(item, is_bold, is_static));
}

template <typename T> void NCurses::Menu<T>::AddSeparator()
{
	m_options.push_back(static_cast<Option *>(0));
}

template <typename T> void NCurses::Menu<T>::InsertOption(size_t pos, const T &item, bool is_bold, bool is_static)
{
	m_options.insert(m_options.begin()+pos, new Option(item, is_bold, is_static));
}

template <typename T> void NCurses::Menu<T>::InsertSeparator(size_t pos)
{
	m_options.insert(m_options.begin()+pos, 0);
}

template <typename T> void NCurses::Menu<T>::DeleteOption(size_t pos)
{
	if (m_options_ptr->empty())
		return;
	if (m_options_ptr == &m_filtered_options)
	{
		delete m_options.at(m_filtered_positions[pos]);
		m_options.erase(m_options.begin()+m_filtered_positions[pos]);
		m_filtered_options.erase(m_filtered_options.begin()+pos);
		m_filtered_positions.erase(m_filtered_positions.begin()+pos);
		for (size_t i = pos; i < m_filtered_positions.size(); ++i)
			m_filtered_positions[i]--;
	}
	else
	{
		delete m_options.at(pos);
		m_options.erase(m_options.begin()+pos);
	}
	m_found_positions.clear();
	if (m_options_ptr->empty())
		Window::Clear();
}

template <typename T> void NCurses::Menu<T>::IntoSeparator(size_t pos)
{
	delete m_options_ptr->at(pos);
	(*m_options_ptr)[pos] = 0;
}

template <typename T> void NCurses::Menu<T>::Bold(int pos, bool state)
{
	if (!m_options_ptr->at(pos))
		return;
	(*m_options_ptr)[pos]->isBold = state;
}

template <typename T> void NCurses::Menu<T>::Swap(size_t one, size_t two)
{
	std::swap(m_options.at(one), m_options.at(two));
}

template <typename T> void NCurses::Menu<T>::Move(size_t from, size_t to)
{
	int diff = from-to;
	if (diff > 0)
	{
		for (size_t i = from; i > to; --i)
			std::swap(m_options.at(i), m_options.at(i-1));
	}
	else if (diff < 0)
	{
		for (size_t i = from; i < to; ++i)
			std::swap(m_options.at(i), m_options.at(i+1));
	}
}

template <typename T> bool NCurses::Menu<T>::Goto(size_t y)
{
	if (!m_options_ptr->at(itsBeginning+y) || m_options_ptr->at(itsBeginning+y)->isStatic)
		return false;
	itsHighlight = itsBeginning+y;
	return true;
}

template <typename T> void NCurses::Menu<T>::Refresh()
{
	if (m_options_ptr->empty())
	{
		Window::Refresh();
		return;
	}
	int MaxBeginning = m_options_ptr->size() < itsHeight ? 0 : m_options_ptr->size()-itsHeight;
	
	if (itsBeginning < itsHighlight-int(itsHeight)+1) // highlighted position is off the screen
		itsBeginning = itsHighlight-itsHeight+1;
	
	if (itsBeginning < 0)
		itsBeginning = 0;
	else if (itsBeginning > MaxBeginning)
		itsBeginning = MaxBeginning;
	
	if (!m_options_ptr->empty() && itsHighlight > int(m_options_ptr->size())-1)
		itsHighlight = m_options_ptr->size()-1;
	
	if (!(*m_options_ptr)[itsHighlight] || (*m_options_ptr)[itsHighlight]->isStatic) // it shouldn't be here
	{
		Scroll(wUp);
		if (!(*m_options_ptr)[itsHighlight] || (*m_options_ptr)[itsHighlight]->isStatic)
			Scroll(wDown);
	}
	
	size_t line = 0;
	for (size_t &i = (m_drawn_position = itsBeginning); i < itsBeginning+itsHeight; ++i)
	{
		GotoXY(0, line);
		if (i >= m_options_ptr->size())
		{
			for (; line < itsHeight; ++line)
				mvwhline(itsWindow, line, 0, 32, itsWidth);
			break;
		}
		if (!(*m_options_ptr)[i]) // separator
		{
			mvwhline(itsWindow, line++, 0, 0, itsWidth);
			continue;
		}
		if ((*m_options_ptr)[i]->isBold)
			*this << fmtBold;
		if (m_highlight_enabled && int(i) == itsHighlight)
		{
			*this << fmtReverse;
			*this << m_highlight_color;
		}
		mvwhline(itsWindow, line, 0, 32, itsWidth);
		if ((*m_options_ptr)[i]->isSelected && m_selected_prefix)
			*this << *m_selected_prefix;
		if (m_item_displayer)
			m_item_displayer(*this, (*m_options_ptr)[i]->Item);
		if ((*m_options_ptr)[i]->isSelected && m_selected_suffix)
			*this << *m_selected_suffix;
		if (m_highlight_enabled && int(i) == itsHighlight)
		{
			*this << clEnd;
			*this << fmtReverseEnd;
		}
		if ((*m_options_ptr)[i]->isBold)
			*this << fmtBoldEnd;
		line++;
	}
	Window::Refresh();
}

template <typename T> void NCurses::Menu<T>::Scroll(Where where)
{
	if (m_options_ptr->empty())
		return;
	int MaxHighlight = m_options_ptr->size()-1;
	int MaxBeginning = m_options_ptr->size() < itsHeight ? 0 : m_options_ptr->size()-itsHeight;
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
				if (m_cyclic_scroll_enabled)
					return Scroll(wEnd);
				break;
			}
			else
			{
				itsHighlight--;
			}
			if (!(*m_options_ptr)[itsHighlight] || (*m_options_ptr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == 0 && !m_cyclic_scroll_enabled ? wDown : wUp);
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
				if (m_cyclic_scroll_enabled)
					return Scroll(wHome);
				break;
			}
			else
			{
				itsHighlight++;
			}
			if (!(*m_options_ptr)[itsHighlight] || (*m_options_ptr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == MaxHighlight && !m_cyclic_scroll_enabled ? wUp : wDown);
			}
			break;
		}
		case wPageUp:
		{
			if (m_cyclic_scroll_enabled && itsHighlight == 0)
				return Scroll(wEnd);
			itsHighlight -= itsHeight;
			itsBeginning -= itsHeight;
			if (itsBeginning < 0)
			{
				itsBeginning = 0;
				if (itsHighlight < 0)
					itsHighlight = 0;
			}
			if (!(*m_options_ptr)[itsHighlight] || (*m_options_ptr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == 0 && !m_cyclic_scroll_enabled ? wDown : wUp);
			}
			break;
		}
		case wPageDown:
		{
			if (m_cyclic_scroll_enabled && itsHighlight == MaxHighlight)
				return Scroll(wHome);
			itsHighlight += itsHeight;
			itsBeginning += itsHeight;
			if (itsBeginning > MaxBeginning)
			{
				itsBeginning = MaxBeginning;
				if (itsHighlight > MaxHighlight)
					itsHighlight = MaxHighlight;
			}
			if (!(*m_options_ptr)[itsHighlight] || (*m_options_ptr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == MaxHighlight && !m_cyclic_scroll_enabled ? wUp : wDown);
			}
			break;
		}
		case wHome:
		{
			itsHighlight = 0;
			itsBeginning = 0;
			if (!(*m_options_ptr)[itsHighlight] || (*m_options_ptr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == 0 ? wDown : wUp);
			}
			break;
		}
		case wEnd:
		{
			itsHighlight = MaxHighlight;
			itsBeginning = MaxBeginning;
			if (!(*m_options_ptr)[itsHighlight] || (*m_options_ptr)[itsHighlight]->isStatic)
			{
				Scroll(itsHighlight == MaxHighlight ? wUp : wDown);
			}
			break;
		}
	}
	if (m_autocenter_cursor)
		Highlight(itsHighlight);
}

template <typename T> void NCurses::Menu<T>::Reset()
{
	itsHighlight = 0;
	itsBeginning = 0;
}

template <typename T> void NCurses::Menu<T>::ClearFiltered()
{
	m_filtered_options.clear();
	m_filtered_positions.clear();
	m_options_ptr = &m_options;
}

template <typename T> void NCurses::Menu<T>::Clear()
{
	for (option_iterator it = m_options.begin(); it != m_options.end(); ++it)
		delete *it;
	m_options.clear();
	m_found_positions.clear();
	m_filter.clear();
	ClearFiltered();
	m_options_ptr = &m_options;
	Window::Clear();
}

template <typename T> bool NCurses::Menu<T>::isBold(int pos)
{
	pos = pos == -1 ? itsHighlight : pos;
	if (!m_options_ptr->at(pos))
		return 0;
	return (*m_options_ptr)[pos]->isBold;
}

template <typename T> void NCurses::Menu<T>::Select(int pos, bool state)
{
	if (!m_options_ptr->at(pos))
		return;
	(*m_options_ptr)[pos]->isSelected = state;
}

template <typename T> void NCurses::Menu<T>::Static(int pos, bool state)
{
	if (!m_options_ptr->at(pos))
		return;
	(*m_options_ptr)[pos]->isStatic = state;
}

template <typename T> bool NCurses::Menu<T>::isSelected(int pos) const
{
	pos = pos == -1 ? itsHighlight : pos;
	if (!m_options_ptr->at(pos))
		return 0;
	return (*m_options_ptr)[pos]->isSelected;
}

template <typename T> bool NCurses::Menu<T>::isStatic(int pos) const
{
	pos = pos == -1 ? itsHighlight : pos;
	if (!m_options_ptr->at(pos))
		return 1;
	return (*m_options_ptr)[pos]->isStatic;
}

template <typename T> bool NCurses::Menu<T>::isSeparator(int pos) const
{
	pos = pos == -1 ? itsHighlight : pos;
	return !m_options_ptr->at(pos);
}

template <typename T> bool NCurses::Menu<T>::hasSelected() const
{
	for (option_const_iterator it = m_options_ptr->begin(); it != m_options_ptr->end(); ++it)
		if (*it && (*it)->isSelected)
			return true;
	return false;
}

template <typename T> void NCurses::Menu<T>::GetSelected(std::vector<size_t> &v) const
{
	for (size_t i = 0; i < m_options_ptr->size(); ++i)
		if ((*m_options_ptr)[i] && (*m_options_ptr)[i]->isSelected)
			v.push_back(i);
}

template <typename T> void NCurses::Menu<T>::Highlight(size_t pos)
{
	itsHighlight = pos;
	itsBeginning = pos-itsHeight/2;
}

template <typename T> size_t NCurses::Menu<T>::Size() const
{
	return m_options_ptr->size();
}

template <typename T> size_t NCurses::Menu<T>::Choice() const
{
	return itsHighlight;
}

template <typename T> size_t NCurses::Menu<T>::RealChoice() const
{
	size_t result = 0;
	for (option_const_iterator it = m_options_ptr->begin(); it != m_options_ptr->begin()+itsHighlight; ++it)
		if (*it && !(*it)->isStatic)
			result++;
	return result;
}

template <typename T> void NCurses::Menu<T>::ReverseSelection(size_t beginning)
{
	option_iterator it = m_options_ptr->begin()+beginning;
	for (size_t i = beginning; i < Size(); ++i, ++it)
		if (*it)
			(*it)->isSelected = !(*it)->isSelected && !(*it)->isStatic;
}

template <typename T> bool NCurses::Menu<T>::Search(const std::string &constraint, size_t beginning, int flags)
{
	m_found_positions.clear();
	m_search_constraint.clear();
	if (constraint.empty())
		return false;
	m_search_constraint = constraint;
	regex_t rx;
	if (regcomp(&rx, m_search_constraint.c_str(), flags) == 0)
	{
		for (size_t i = beginning; i < m_options_ptr->size(); ++i)
		{
			if (regexec(&rx, GetOption(i).c_str(), 0, 0, 0) == 0)
				m_found_positions.insert(i);
		}
	}
	regfree(&rx);
	return !m_found_positions.empty();
}

template <typename T> void NCurses::Menu<T>::NextFound(bool wrap)
{
	if (m_found_positions.empty())
		return;
	std::set<size_t>::iterator next = m_found_positions.upper_bound(itsHighlight);
	if (next != m_found_positions.end())
		Highlight(*next);
	else if (wrap)
		Highlight(*m_found_positions.begin());
}

template <typename T> void NCurses::Menu<T>::PrevFound(bool wrap)
{
	if (m_found_positions.empty())
		return;
	std::set<size_t>::iterator prev = m_found_positions.lower_bound(itsHighlight);
	if (prev != m_found_positions.begin())
		Highlight(*--prev);
	else if (wrap)
		Highlight(*m_found_positions.rbegin());
}

template <typename T> void NCurses::Menu<T>::ApplyFilter(const std::string &filter, size_t beginning, int flags)
{
	m_found_positions.clear();
	ClearFiltered();
	m_filter = filter;
	if (m_filter.empty())
		return;
	for (size_t i = 0; i < beginning; ++i)
	{
		m_filtered_positions.push_back(i);
		m_filtered_options.push_back(m_options[i]);
	}
	regex_t rx;
	if (regcomp(&rx, m_filter.c_str(), flags) == 0)
	{
		for (size_t i = beginning; i < m_options.size(); ++i)
		{
			if (regexec(&rx, GetOption(i).c_str(), 0, 0, 0) == 0)
			{
				m_filtered_positions.push_back(i);
				m_filtered_options.push_back(m_options[i]);
			}
		}
	}
	regfree(&rx);
	m_options_ptr = &m_filtered_options;
	if (m_options_ptr->empty()) // oops, we didn't find anything
		Window::Clear();
}

template <typename T> const std::string &NCurses::Menu<T>::GetFilter()
{
	return m_filter;
}

template <typename T> std::string NCurses::Menu<T>::GetOption(size_t pos)
{
	if (m_options_ptr->at(pos) && m_get_string_helper)
		return m_get_string_helper((*m_options_ptr)[pos]->Item);
	else
		return "";
}

template <typename T> T &NCurses::Menu<T>::Back()
{
	if (!m_options_ptr->back())
		FatalError("Menu::Back() has requested separator!");
	return m_options_ptr->back()->Item;
}

template <typename T> const T &NCurses::Menu<T>::Back() const
{
	if (!m_options_ptr->back())
		FatalError("Menu::Back() has requested separator!");
	return m_options_ptr->back()->Item;
}

template <typename T> T &NCurses::Menu<T>::Current()
{
	if (!m_options_ptr->at(itsHighlight))
		FatalError("Menu::Current() has requested separator!");
	return (*m_options_ptr)[itsHighlight]->Item;
}

template <typename T> const T &NCurses::Menu<T>::Current() const
{
	if (!m_options_ptr->at(itsHighlight))
		FatalError("Menu::Current() const has requested separator!");
	return (*m_options_ptr)[itsHighlight]->Item;
}

template <typename T> T &NCurses::Menu<T>::at(size_t pos)
{
	if (!m_options_ptr->at(pos))
		FatalError("Menu::at() has requested separator!");
	return (*m_options_ptr)[pos]->Item;
}

template <typename T> const T &NCurses::Menu<T>::at(size_t pos) const
{
	if (!m_options->at(pos))
		FatalError("Menu::at() const has requested separator!");
	return (*m_options_ptr)[pos]->Item;
}

template <typename T> const T &NCurses::Menu<T>::operator[](size_t pos) const
{
	if (!(*m_options_ptr)[pos])
		FatalError("Menu::operator[] const has requested separator!");
	return (*m_options_ptr)[pos]->Item;
}

template <typename T> T &NCurses::Menu<T>::operator[](size_t pos)
{
	if (!(*m_options_ptr)[pos])
		FatalError("Menu::operator[] has requested separator!");
	return (*m_options_ptr)[pos]->Item;
}

#endif

