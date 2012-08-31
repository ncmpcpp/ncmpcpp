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

#include <cassert>
#include <regex.h>
#include <functional>
#include <iterator>
#include <set>

#include "error.h"
#include "window.h"
#include "strbuffer.h"

namespace NCurses {

/// List class is an interface for Menu class
///
struct List
{
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
template <typename T> struct Menu : public Window, public List
{
	/// Function helper prototype used to display each option on the screen.
	/// If not set by setItemDisplayer(), menu won't display anything.
	/// @see setItemDisplayer()
	///
	typedef std::function<void(Menu<T> &)> ItemDisplayer;
	
	/// Function helper prototype used for converting items to strings.
	/// If not set by SetItemStringifier(), searching and filtering
	/// won't work (note that Menu<std::string> doesn't need this)
	/// @see SetItemStringifier()
	///
	typedef std::function<std::string(const T &)> ItemStringifier;
	
	/// Struct that holds each item in the list and its attributes
	///
	struct Item
	{
		friend class Menu<T>;
		
		Item()
		: m_is_bold(false), m_is_selected(false), m_is_inactive(false), m_is_separator(false) { }
		Item(const T &value_, bool is_bold, bool is_inactive)
		: m_value(value_), m_is_bold(is_bold), m_is_selected(false), m_is_inactive(is_inactive), m_is_separator(false) { }
		
		T &value() { return m_value; }
		const T &value() const { return m_value; }
		
		void setBold(bool is_bold) { m_is_bold = is_bold; }
		void setSelected(bool is_selected) { m_is_selected = is_selected; }
		void setInactive(bool is_inactive) { m_is_inactive = is_inactive; }
		void setSeparator(bool is_separator) { m_is_separator = is_separator; }
		
		bool isBold() const { return m_is_bold; }
		bool isSelected() const { return m_is_selected; }
		bool isInactive() const { return m_is_inactive; }
		bool isSeparator() const { return m_is_separator; }
		
	private:
		static Item *mkSeparator()
		{
			Item *i = new Item;
			i->m_is_separator = true;
			return i;
		}
		
		T m_value;
		bool m_is_bold;
		bool m_is_selected;
		bool m_is_inactive;
		bool m_is_separator;
	};
	
	template <typename ValueT, typename BaseIterator> class ItemIterator
		: public std::iterator<std::random_access_iterator_tag, ValueT>
	{
		friend class Menu<T>;
		
		BaseIterator m_it;
		explicit ItemIterator(BaseIterator it) : m_it(it) { }
		
		static const bool referenceValue = !std::is_same<
			ValueT, typename std::remove_pointer<
				typename BaseIterator::value_type
			>::type
		>::value;
		template <typename Result, bool referenceValue> struct getObject { };
		template <typename Result> struct getObject<Result, true> {
			static Result &apply(BaseIterator it) { return (*it)->value(); }
		};
		template <typename Result> struct getObject<Result, false> {
			static Result &apply(BaseIterator it) { return **it; }
		};
		
	public:
		ItemIterator() { }
		
		ValueT &operator*() const { return getObject<ValueT, referenceValue>::apply(m_it); }
		typename BaseIterator::value_type operator->() { return *m_it; }
		
		ItemIterator &operator++() { ++m_it; return *this; }
		ItemIterator operator++(int) { return ItemIterator(m_it++); }
		
		ItemIterator &operator--() { --m_it; return *this; }
		ItemIterator operator--(int) { return ItemIterator(m_it--); }
		
		ValueT &operator[](ptrdiff_t n) const {
			return getObject<ValueT, referenceValue>::apply(&m_it[n]);
		}
		
		ItemIterator &operator+=(ptrdiff_t n) { m_it += n; return *this; }
		ItemIterator operator+(ptrdiff_t n) const { return ItemIterator(m_it + n); }
		
		ItemIterator &operator-=(ptrdiff_t n) { m_it -= n; return *this; }
		ItemIterator operator-(ptrdiff_t n) const { return ItemIterator(m_it - n); }
		
		ptrdiff_t operator-(const ItemIterator &rhs) const { return m_it - rhs.m_it; }
		
		template <typename Iterator>
		bool operator==(const Iterator &rhs) const { return m_it == rhs.m_it; }
		template <typename Iterator>
		bool operator!=(const Iterator &rhs) const { return m_it != rhs.m_it; }
		template <typename Iterator>
		bool operator<(const Iterator &rhs) const { return m_it < rhs.m_it; }
		template <typename Iterator>
		bool operator<=(const Iterator &rhs) const { return m_it <= rhs.m_it; }
		template <typename Iterator>
		bool operator>(const Iterator &rhs) const { return m_it > rhs.m_it; }
		template <typename Iterator>
		bool operator>=(const Iterator &rhs) const { return m_it >= rhs.m_it; }
		
		/// non-const to const conversion
		template <typename Iterator> operator ItemIterator<
			typename std::add_const<ValueT>::type, Iterator
		>() { return ItemIterator(m_it); }
		
		const BaseIterator &base() { return m_it; }
	};
	
	typedef ItemIterator<
		Item, typename std::vector<Item *>::iterator
	> Iterator;
	typedef ItemIterator<
		const Item, typename std::vector<Item *>::const_iterator
	> ConstIterator;
	
	typedef std::reverse_iterator<Iterator> ReverseIterator;
	typedef std::reverse_iterator<ConstIterator> ConstReverseIterator;
	
	typedef ItemIterator<
		T, typename std::vector<Item *>::iterator
	> ValueIterator;
	typedef ItemIterator<
		typename std::add_const<T>::type, typename std::vector<Item *>::const_iterator
	> ConstValueIterator;
	
	typedef std::reverse_iterator<ValueIterator> ReverseValueIterator;
	typedef std::reverse_iterator<ConstValueIterator> ConstReverseValueIterator;
	
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
	void AddItem(const T &item, bool is_bold = 0, bool is_static = 0);
	
	/// Adds separator to list
	///
	void AddSeparator();
	
	/// Inserts new option to list at given position
	/// @param pos initial position of inserted item
	/// @param item object that has to be inserted
	/// @param is_bold defines the initial state of bold attribute
	/// @param is_static defines the initial state of static attribute
	///
	void InsertItem(size_t pos, const T &Item, bool is_bold = 0, bool is_static = 0);
	
	/// Inserts separator to list at given position
	/// @param pos initial position of inserted separator
	///
	void InsertSeparator(size_t pos);
	
	/// Deletes item from given position
	/// @param pos given position of item to be deleted
	///
	void DeleteItem(size_t pos);
	
	/// Swaps the content of two items
	/// @param one position of first item
	/// @param two position of second item
	///
	void Swap(size_t one, size_t two);
	
	/// Moves the highlighted position to the given line of window
	/// @param y Y position of menu window to be highlighted
	/// @return true if the position is reachable, false otherwise
	///
	bool Goto(size_t y);
	
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
	std::string GetItem(size_t pos);
	
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
	
	/// Sets prefix, that is put before each selected item to indicate its selection
	/// Note that the passed variable is not deleted along with menu object.
	/// @param b pointer to buffer that contains the prefix
	///
	void SetSelectPrefix(const Buffer &b) { m_selected_prefix = b; }
	
	/// Sets suffix, that is put after each selected item to indicate its selection
	/// Note that the passed variable is not deleted along with menu object.
	/// @param b pointer to buffer that contains the suffix
	///
	void SetSelectSuffix(const Buffer &b) { m_selected_suffix = b; }
	
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
	
	/// @return currently drawn item. The result is defined only within
	/// drawing function that is called by Refresh()
	/// @see Refresh()
	///
	const Item &Drawn() const { return *(*m_options_ptr)[m_drawn_position]; }
	
	/// @return position of currently drawn item. The result is defined
	/// only within drawing function that is called by Refresh()
	/// @see Refresh()
	///
	size_t DrawnPosition() const { return m_drawn_position; }
	
	/// @return reference to last item on the list
	/// @throw List::InvalidItem if requested item is separator
	///
	Menu<T>::Item &Back();
	
	/// @return const reference to last item on the list
	/// @throw List::InvalidItem if requested item is separator
	///
	const Menu<T>::Item &Back() const;
	
	/// @return reference to curently highlighted object
	/// @throw List::InvalidItem if requested item is separator
	///
	Menu<T>::Item &Current();
	
	/// @return const reference to curently highlighted object
	/// @throw List::InvalidItem if requested item is separator
	///
	const Menu<T>::Item &Current() const;
	
	/// @param pos requested position
	/// @return reference to item at given position
	/// @throw std::out_of_range if given position is out of range
	/// @throw List::InvalidItem if requested item is separator
	///
	Menu<T>::Item &at(size_t pos);
	
	/// @param pos requested position
	/// @return const reference to item at given position
	/// @throw std::out_of_range if given position is out of range
	/// @throw List::InvalidItem if requested item is separator
	///
	const Menu<T>::Item &at(size_t pos) const;
	
	/// @param pos requested position
	/// @return const reference to item at given position
	/// @throw List::InvalidItem if requested item is separator
	///
	const Menu<T>::Item &operator[](size_t pos) const;
	
	/// @param pos requested position
	/// @return const reference to item at given position
	/// @throw List::InvalidItem if requested item is separator
	///
	Menu<T>::Item &operator[](size_t pos);
	
	Iterator Begin() { return Iterator(m_options_ptr->begin()); }
	ConstIterator Begin() const { return ConstIterator(m_options_ptr->begin()); }
	Iterator End() { return Iterator(m_options_ptr->end()); }
	ConstIterator End() const { return ConstIterator(m_options_ptr->end()); }
	
	ReverseIterator Rbegin() { return ReverseIterator(End()); }
	ConstReverseIterator Rbegin() const { return ConstReverseIterator(End()); }
	ReverseIterator Rend() { return ReverseIterator(Begin()); }
	ConstReverseIterator Rend() const { return ConstReverseIterator(Begin()); }
	
	ValueIterator BeginV() { return ValueIterator(m_options_ptr->begin()); }
	ConstValueIterator BeginV() const { return ConstValueIterator(m_options_ptr->begin()); }
	ValueIterator EndV() { return ValueIterator(m_options_ptr->end()); }
	ConstValueIterator EndV() const { return ConstValueIterator(m_options_ptr->end()); }
	
	ReverseValueIterator RbeginV() { return ReverseValueIterator(End()); }
	ConstReverseIterator RbeginV() const { return ConstReverseValueIterator(End()); }
	ReverseValueIterator RendV() { return ReverseValueIterator(Begin()); }
	ConstReverseValueIterator RendV() const { return ConstReverseValueIterator(Begin()); }
	
private:
	/// Clears filter, filtered data etc.
	///
	void ClearFiltered();
	
	bool isHighlightable(size_t pos)
	{
		return !(*m_options_ptr)[pos]->isSeparator() && !(*m_options_ptr)[pos]->isInactive();
	}
	
	ItemDisplayer m_item_displayer;
	ItemStringifier m_get_string_helper;
	
	std::string m_filter;
	std::string m_search_constraint;
	
	std::vector<Item *> *m_options_ptr;
	std::vector<Item *> m_options;
	std::vector<Item *> m_filtered_options;
	std::vector<size_t> m_filtered_positions;
	std::set<size_t> m_found_positions;
	
	size_t m_beginning;
	size_t m_highlight;
	
	Color m_highlight_color;
	bool m_highlight_enabled;
	bool m_cyclic_scroll_enabled;
	
	bool m_autocenter_cursor;
	
	size_t m_drawn_position;
	
	Buffer m_selected_prefix;
	Buffer m_selected_suffix;
};

/// Specialization of Menu<T>::GetItem for T = std::string, it's obvious
/// that if strings are stored, we don't need extra function to convert
/// them to strings by default
template <> std::string Menu<std::string>::GetItem(size_t pos);

template <typename T> Menu<T>::Menu(size_t startx,
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
	m_beginning(0),
	m_highlight(0),
	m_highlight_color(itsBaseColor),
	m_highlight_enabled(true),
	m_cyclic_scroll_enabled(false),
	m_autocenter_cursor(false)
{
}

template <typename T> Menu<T>::~Menu()
{
	for (auto it = m_options.begin(); it != m_options.end(); ++it)
		delete *it;
}

template <typename T> void Menu<T>::Reserve(size_t size)
{
	m_options.reserve(size);
}

template <typename T> void Menu<T>::ResizeList(size_t size)
{
	if (size > m_options.size())
	{
		m_options.resize(size);
		for (size_t i = 0; i < size; ++i)
			if (!m_options[i])
				m_options[i] = new Item();
	}
	else if (size < m_options.size())
	{
		for (size_t i = size; i < m_options.size(); ++i)
			delete m_options[i];
		m_options.resize(size);
	}
}

template <typename T> void Menu<T>::AddItem(const T &item, bool is_bold, bool is_inactive)
{
	m_options.push_back(new Item(item, is_bold, is_inactive));
}

template <typename T> void Menu<T>::AddSeparator()
{
	m_options.push_back(Item::mkSeparator());
}

template <typename T> void Menu<T>::InsertItem(size_t pos, const T &item, bool is_bold, bool is_inactive)
{
	m_options.insert(m_options.begin()+pos, new Item(item, is_bold, is_inactive));
}

template <typename T> void Menu<T>::InsertSeparator(size_t pos)
{
	m_options.insert(m_options.begin()+pos, Item::mkSeparator());
}

template <typename T> void Menu<T>::DeleteItem(size_t pos)
{
	assert(m_options_ptr != &m_filtered_options);
	assert(pos < m_options.size());
	delete m_options[pos];
	m_options.erase(m_options.begin()+pos);
}

template <typename T> void Menu<T>::Swap(size_t one, size_t two)
{
	std::swap(m_options.at(one), m_options.at(two));
}

template <typename T> bool Menu<T>::Goto(size_t y)
{
	if (!isHighlightable(m_beginning+y))
		return false;
	m_highlight = m_beginning+y;
	return true;
}

template <typename T> void Menu<T>::Refresh()
{
	if (m_options_ptr->empty())
	{
		Window::Clear();
		return;
	}
	
	size_t max_beginning = m_options_ptr->size() < itsHeight ? 0 : m_options_ptr->size()-itsHeight;
	m_beginning = std::min(m_beginning, max_beginning);
	
	// if highlighted position is off the screen, make it visible
	m_highlight = std::min(m_highlight, m_beginning+itsHeight-1);
	// if highlighted position is invalid, correct it
	m_highlight = std::min(m_highlight, m_options_ptr->size()-1);
	
	if (!isHighlightable(m_highlight))
	{
		Scroll(wUp);
		if (isHighlightable(m_highlight))
			Scroll(wDown);
	}
	
	size_t line = 0;
	m_drawn_position = m_beginning;
	for (size_t &i = m_drawn_position; i < m_beginning+itsHeight; ++i, ++line)
	{
		GotoXY(0, line);
		if (i >= m_options_ptr->size())
		{
			for (; line < itsHeight; ++line)
				mvwhline(itsWindow, line, 0, KEY_SPACE, itsWidth);
			break;
		}
		if ((*m_options_ptr)[i]->isSeparator())
		{
			mvwhline(itsWindow, line, 0, 0, itsWidth);
			continue;
		}
		if ((*m_options_ptr)[i]->isBold())
			*this << fmtBold;
		if (m_highlight_enabled && i == m_highlight)
		{
			*this << fmtReverse;
			*this << m_highlight_color;
		}
		mvwhline(itsWindow, line, 0, KEY_SPACE, itsWidth);
		if ((*m_options_ptr)[i]->isSelected())
			*this << m_selected_prefix;
		if (m_item_displayer)
			m_item_displayer(*this);
		if ((*m_options_ptr)[i]->isSelected())
			*this << m_selected_suffix;
		if (m_highlight_enabled && i == m_highlight)
		{
			*this << clEnd;
			*this << fmtReverseEnd;
		}
		if ((*m_options_ptr)[i]->isBold())
			*this << fmtBoldEnd;
	}
	Window::Refresh();
}

template <typename T> void Menu<T>::Scroll(Where where)
{
	if (m_options_ptr->empty())
		return;
	size_t max_highlight = m_options_ptr->size()-1;
	size_t max_beginning = m_options_ptr->size() < itsHeight ? 0 : m_options_ptr->size()-itsHeight;
	size_t max_visible_highlight = m_beginning+itsHeight-1;
	switch (where)
	{
		case wUp:
		{
			if (m_highlight <= m_beginning && m_highlight > 0)
				--m_beginning;
			if (m_highlight == 0)
			{
				if (m_cyclic_scroll_enabled)
					return Scroll(wEnd);
				break;
			}
			else
				--m_highlight;
			if (!isHighlightable(m_highlight))
				Scroll(m_highlight == 0 && !m_cyclic_scroll_enabled ? wDown : wUp);
			break;
		}
		case wDown:
		{
			if (m_highlight >= max_visible_highlight && m_highlight < max_highlight)
				++m_beginning;
			if (m_highlight == max_highlight)
			{
				if (m_cyclic_scroll_enabled)
					return Scroll(wHome);
				break;
			}
			else
				++m_highlight;
			if (!isHighlightable(m_highlight))
				Scroll(m_highlight == max_highlight && !m_cyclic_scroll_enabled ? wUp : wDown);
			break;
		}
		case wPageUp:
		{
			if (m_cyclic_scroll_enabled && m_highlight == 0)
				return Scroll(wEnd);
			if (m_highlight < itsHeight)
				m_highlight = 0;
			else
				m_highlight -= itsHeight;
			if (m_beginning < itsHeight)
				m_beginning = 0;
			else
				m_beginning -= itsHeight;
			if (!isHighlightable(m_highlight))
				Scroll(m_highlight == 0 && !m_cyclic_scroll_enabled ? wDown : wUp);
			break;
		}
		case wPageDown:
		{
			if (m_cyclic_scroll_enabled && m_highlight == max_highlight)
				return Scroll(wHome);
			m_highlight += itsHeight;
			m_beginning += itsHeight;
			m_beginning = std::min(m_beginning, max_beginning);
			m_highlight = std::min(m_highlight, max_highlight);
			if (!isHighlightable(m_highlight))
				Scroll(m_highlight == max_highlight && !m_cyclic_scroll_enabled ? wUp : wDown);
			break;
		}
		case wHome:
		{
			m_highlight = 0;
			m_beginning = 0;
			if (!isHighlightable(m_highlight))
				Scroll(wDown);
			break;
		}
		case wEnd:
		{
			m_highlight = max_highlight;
			m_beginning = max_beginning;
			if (!isHighlightable(m_highlight))
				Scroll(wUp);
			break;
		}
	}
	if (m_autocenter_cursor)
		Highlight(m_highlight);
}

template <typename T> void Menu<T>::Reset()
{
	m_highlight = 0;
	m_beginning = 0;
}

template <typename T> void Menu<T>::ClearFiltered()
{
	m_filtered_options.clear();
	m_filtered_positions.clear();
	m_options_ptr = &m_options;
}

template <typename T> void Menu<T>::Clear()
{
	for (auto it = m_options.begin(); it != m_options.end(); ++it)
		delete *it;
	m_options.clear();
	m_found_positions.clear();
	m_filter.clear();
	ClearFiltered();
	m_options_ptr = &m_options;
}

template <typename T> bool Menu<T>::hasSelected() const
{
	for (auto it = m_options_ptr->begin(); it != m_options_ptr->end(); ++it)
		if ((*it)->isSelected())
			return true;
	return false;
}

template <typename T> void Menu<T>::GetSelected(std::vector<size_t> &v) const
{
	for (size_t i = 0; i < m_options_ptr->size(); ++i)
		if ((*m_options_ptr)[i]->isSelected())
			v.push_back(i);
}

template <typename T> void Menu<T>::Highlight(size_t pos)
{
	m_highlight = pos;
	size_t half_height = itsHeight/2;
	if (pos < half_height)
		m_beginning = 0;
	else
		m_beginning = pos-half_height;
}

template <typename T> size_t Menu<T>::Size() const
{
	return m_options_ptr->size();
}

template <typename T> size_t Menu<T>::Choice() const
{
	return m_highlight;
}

template <typename T> void Menu<T>::ReverseSelection(size_t beginning)
{
	auto it = m_options_ptr->begin()+beginning;
	for (size_t i = beginning; i < Size(); ++i, ++it)
		(*it)->setSelected(!(*it)->isSelected() && !(*it)->isInactive());
}

template <typename T> bool Menu<T>::Search(const std::string &constraint, size_t beginning, int flags)
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
			if (regexec(&rx, GetItem(i).c_str(), 0, 0, 0) == 0)
				m_found_positions.insert(i);
		}
	}
	regfree(&rx);
	return !m_found_positions.empty();
}

template <typename T> void Menu<T>::NextFound(bool wrap)
{
	if (m_found_positions.empty())
		return;
	std::set<size_t>::iterator next = m_found_positions.upper_bound(m_highlight);
	if (next != m_found_positions.end())
		Highlight(*next);
	else if (wrap)
		Highlight(*m_found_positions.begin());
}

template <typename T> void Menu<T>::PrevFound(bool wrap)
{
	if (m_found_positions.empty())
		return;
	std::set<size_t>::iterator prev = m_found_positions.lower_bound(m_highlight);
	if (prev != m_found_positions.begin())
		Highlight(*--prev);
	else if (wrap)
		Highlight(*m_found_positions.rbegin());
}

template <typename T> void Menu<T>::ApplyFilter(const std::string &filter, size_t beginning, int flags)
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
			if (regexec(&rx, GetItem(i).c_str(), 0, 0, 0) == 0)
			{
				m_filtered_positions.push_back(i);
				m_filtered_options.push_back(m_options[i]);
			}
		}
	}
	regfree(&rx);
	m_options_ptr = &m_filtered_options;
}

template <typename T> const std::string &Menu<T>::GetFilter()
{
	return m_filter;
}

template <typename T> std::string Menu<T>::GetItem(size_t pos)
{
	std::string result;
	if (m_get_string_helper)
		result = m_get_string_helper((*m_options_ptr)[pos]->value());
	return result;
}

template <typename T> typename Menu<T>::Item &Menu<T>::Back()
{
	return *m_options_ptr->back();
}

template <typename T> const typename Menu<T>::Item &Menu<T>::Back() const
{
	return *m_options_ptr->back();
}

template <typename T> typename Menu<T>::Item &Menu<T>::Current()
{
	return *(*m_options_ptr)[m_highlight];
}

template <typename T> const typename Menu<T>::Item &Menu<T>::Current() const
{
	return *(*m_options_ptr)[m_highlight];
}

template <typename T> typename Menu<T>::Item &Menu<T>::at(size_t pos)
{
	return *m_options_ptr->at(pos);
}

template <typename T> const typename Menu<T>::Item &Menu<T>::at(size_t pos) const
{
	return *m_options_ptr->at(pos);
}

template <typename T> const typename Menu<T>::Item &Menu<T>::operator[](size_t pos) const
{
	assert(m_options_ptr->size() > pos);
	return *(*m_options_ptr)[pos];
}

template <typename T> typename Menu<T>::Item &Menu<T>::operator[](size_t pos)
{
	assert(m_options_ptr->size() > pos);
	return *(*m_options_ptr)[pos];
}

}

#endif
