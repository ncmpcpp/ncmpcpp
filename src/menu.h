/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_MENU_H
#define NCMPCPP_MENU_H

#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <set>

#include "strbuffer.h"
#include "window.h"

namespace NC {

/// This template class is generic menu capable of
/// holding any std::vector compatible values.
template <typename ItemT> class Menu : public Window
{
	struct ItemProxy;
	
public:
	struct Item
	{
		typedef ItemT Type;
		
		friend class Menu<ItemT>;
		
		Item()
		: m_is_bold(false), m_is_selected(false), m_is_inactive(false), m_is_separator(false) { }
		Item(ItemT value_, bool is_bold, bool is_inactive)
		: m_value(value_), m_is_bold(is_bold), m_is_selected(false), m_is_inactive(is_inactive), m_is_separator(false) { }
		
		ItemT &value() { return m_value; }
		const ItemT &value() const { return m_value; }
		
		void setBold(bool is_bold) { m_is_bold = is_bold; }
		void setSelected(bool is_selected) { m_is_selected = is_selected; }
		void setInactive(bool is_inactive) { m_is_inactive = is_inactive; }
		void setSeparator(bool is_separator) { m_is_separator = is_separator; }
		
		bool isBold() const { return m_is_bold; }
		bool isSelected() const { return m_is_selected; }
		bool isInactive() const { return m_is_inactive; }
		bool isSeparator() const { return m_is_separator; }
		
	private:
		static Item mkSeparator()
		{
			Item item;
			item.m_is_separator = true;
			return item;
		}
		
		ItemT m_value;
		bool m_is_bold;
		bool m_is_selected;
		bool m_is_inactive;
		bool m_is_separator;
	};
	
	template <typename ValueT, typename BaseIterator> class ItemIterator
		: public std::iterator<std::random_access_iterator_tag, ValueT>
	{
		friend class Menu<ItemT>;
		
		BaseIterator m_it;
		explicit ItemIterator(BaseIterator it) : m_it(it) { }
		
		// base iterator's value_type doesn't change between const and non-const
		// version, so we need to strip const off ValueT too for proper template
		// version to be instantiated.
		static const bool referenceValue = !std::is_same<
			typename std::remove_const<ValueT>::type,
			typename BaseIterator::value_type::element_type
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
		ValueT *operator->() const { return &getObject<ValueT, referenceValue>::apply(m_it); }
		
		ItemIterator &operator++() { ++m_it; return *this; }
		ItemIterator operator++(int) { return ItemIterator(m_it++); }
		
		ItemIterator &operator--() { --m_it; return *this; }
		ItemIterator operator--(int) { return ItemIterator(m_it--); }
		
		ValueT &operator[](ptrdiff_t n) const {
			return getObject<ValueT, referenceValue>::apply(m_it + n);
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
		template <typename Iterator>
		operator ItemIterator<typename std::add_const<ValueT>::type, Iterator>() {
			return ItemIterator<typename std::add_const<ValueT>::type, Iterator>(m_it);
		}
		
		const BaseIterator &base() const { return m_it; }
	};
	
	typedef ItemIterator<
		Item, typename std::vector<ItemProxy>::iterator
	> Iterator;
	typedef ItemIterator<
		const Item, typename std::vector<ItemProxy>::const_iterator
	> ConstIterator;
	
	typedef std::reverse_iterator<Iterator> ReverseIterator;
	typedef std::reverse_iterator<ConstIterator> ConstReverseIterator;
	
	typedef ItemIterator<
		ItemT, typename std::vector<ItemProxy>::iterator
	> ValueIterator;
	typedef ItemIterator<
		typename std::add_const<ItemT>::type, typename std::vector<ItemProxy>::const_iterator
	> ConstValueIterator;
	
	typedef std::reverse_iterator<ValueIterator> ReverseValueIterator;
	typedef std::reverse_iterator<ConstValueIterator> ConstReverseValueIterator;
	
	/// Function helper prototype used to display each option on the screen.
	/// If not set by setItemDisplayer(), menu won't display anything.
	/// @see setItemDisplayer()
	typedef std::function<void(Menu<ItemT> &)> ItemDisplayer;
	
	typedef std::function<bool(const Item &)> FilterFunction;
	
	Menu() { }
	
	Menu(size_t startx, size_t starty, size_t width, size_t height,
			const std::string &title, Color color, Border border);
	
	Menu(const Menu &rhs);
	Menu(Menu &&rhs);
	Menu &operator=(Menu rhs);
	
	/// Sets helper function that is responsible for displaying items
	/// @param ptr function pointer that matches the ItemDisplayer prototype
	void setItemDisplayer(const ItemDisplayer &f) { m_item_displayer = f; }
	
	/// Reserves the size for internal container (this just calls std::vector::reserve())
	/// @param size requested size
	void reserve(size_t size);
	
	/// Resizes the list to given size (adequate to std::vector::resize())
	/// @param size requested size
	void resizeList(size_t size);
	
	/// Adds new option to list
	/// @param item object that has to be added
	/// @param is_bold defines the initial state of bold attribute
	/// @param is_inactive defines the initial state of static attribute
	void addItem(ItemT item, bool is_bold = 0, bool is_inactive = 0);
	
	/// Adds separator to list
	void addSeparator();
	
	/// Inserts new option to list at given position
	/// @param pos initial position of inserted item
	/// @param item object that has to be inserted
	/// @param is_bold defines the initial state of bold attribute
	/// @param is_inactive defines the initial state of static attribute
	void insertItem(size_t pos, const ItemT &Item, bool is_bold = 0, bool is_inactive = 0);
	
	/// Inserts separator to list at given position
	/// @param pos initial position of inserted separator
	void insertSeparator(size_t pos);
	
	/// Deletes item from given position
	/// @param pos given position of item to be deleted
	void deleteItem(size_t pos);
	
	/// Moves the highlighted position to the given line of window
	/// @param y Y position of menu window to be highlighted
	/// @return true if the position is reachable, false otherwise
	bool Goto(size_t y);
	
	/// Highlights given position
	/// @param pos position to be highlighted
	void highlight(size_t pos);
	
	/// @return currently highlighted position
	size_t choice() const;
	
	void filter(ConstIterator first, ConstIterator last, const FilterFunction &f);
	
	void applyCurrentFilter(ConstIterator first, ConstIterator last);
	
	bool search(ConstIterator first, ConstIterator last, const FilterFunction &f);
	
	/// Clears filter results
	void clearFilterResults();
	
	void clearFilter();
	
	/// Clears search results
	void clearSearchResults();
	
	/// Moves current position in the list to the next found one
	/// @param wrap if true, this function will go to the first
	/// found pos after the last one, otherwise it'll do nothing.
	void nextFound(bool wrap);
	
	/// Moves current position in the list to the previous found one
	/// @param wrap if true, this function will go to the last
	/// found pos after the first one, otherwise it'll do nothing.
	void prevFound(bool wrap);
	
	/// @return const reference to currently used filter function
	const FilterFunction &getFilter() { return m_filter; }
	
	/// @return true if list is currently filtered, false otherwise
	bool isFiltered() { return m_options_ptr == &m_filtered_options; }
	
	/// Turns off filtering
	void showAll() { m_options_ptr = &m_options; }
	
	/// Turns on filtering
	void showFiltered() { m_options_ptr = &m_filtered_options; }
	
	/// Refreshes the menu window
	/// @see Window::refresh()
	virtual void refresh() OVERRIDE;
	
	/// Scrolls by given amount of lines
	/// @param where indicated where exactly one wants to go
	/// @see Window::scroll()
	virtual void scroll(Scroll where) OVERRIDE;
	
	/// Cleares all options, used filters etc. It doesn't reset highlighted position though.
	/// @see reset()
	virtual void clear() OVERRIDE;
	
	/// Sets highlighted position to 0
	void reset();
	
	/// Sets prefix, that is put before each selected item to indicate its selection
	/// Note that the passed variable is not deleted along with menu object.
	/// @param b pointer to buffer that contains the prefix
	void setSelectedPrefix(const Buffer &b) { m_selected_prefix = b; }
	
	/// Sets suffix, that is put after each selected item to indicate its selection
	/// Note that the passed variable is not deleted along with menu object.
	/// @param b pointer to buffer that contains the suffix
	void setSelectedSuffix(const Buffer &b) { m_selected_suffix = b; }
	
	/// Sets custom color of highlighted position
	/// @param col custom color
	void setHighlightColor(Color color) { m_highlight_color = color; }
	
	/// @return state of highlighting
	bool isHighlighted() { return m_highlight_enabled; }
	
	/// Turns on/off highlighting
	/// @param state state of hihglighting
	void setHighlighting(bool state) { m_highlight_enabled = state; }
	
	/// Turns on/off cyclic scrolling
	/// @param state state of cyclic scrolling
	void cyclicScrolling(bool state) { m_cyclic_scroll_enabled = state; }
	
	/// Turns on/off centered cursor
	/// @param state state of centered cursor
	void centeredCursor(bool state) { m_autocenter_cursor = state; }
	
	/// Checks if list is empty
	/// @return true if list is empty, false otherwise
	/// @see reallyEmpty()
	bool empty() const { return m_options_ptr->empty(); }
	
	/// Checks if list is really empty since Empty() may not
	/// be accurate if filter is set)
	/// @return true if list is empty, false otherwise
	/// @see Empty()
	bool reallyEmpty() const { return m_options.empty(); }
	
	/// @return size of the list
	size_t size() const { return m_options_ptr->size(); }
	
	/// @return currently drawn item. The result is defined only within
	/// drawing function that is called by refresh()
	/// @see refresh()
	ConstIterator drawn() const { return begin() + m_drawn_position; }
	
	/// @return reference to last item on the list
	/// @throw List::InvalidItem if requested item is separator
	Menu<ItemT>::Item &back() { return *m_options_ptr->back(); }
	
	/// @return const reference to last item on the list
	/// @throw List::InvalidItem if requested item is separator
	const Menu<ItemT>::Item &back() const { return *m_options_ptr->back(); }
	
	/// @return reference to curently highlighted object
	Menu<ItemT>::Item &current() { return *(*m_options_ptr)[m_highlight]; }
	
	/// @return const reference to curently highlighted object
	const Menu<ItemT>::Item &current() const { return *(*m_options_ptr)[m_highlight]; }
	
	/// @param pos requested position
	/// @return reference to item at given position
	/// @throw std::out_of_range if given position is out of range
	Menu<ItemT>::Item &at(size_t pos) { return *m_options_ptr->at(pos); }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	/// @throw std::out_of_range if given position is out of range
	const Menu<ItemT>::Item &at(size_t pos) const { return *m_options_ptr->at(pos); }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	const Menu<ItemT>::Item &operator[](size_t pos) const  { return *(*m_options_ptr)[pos]; }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	Menu<ItemT>::Item &operator[](size_t pos) { return *(*m_options_ptr)[pos]; }
	
	Iterator currentI() { return Iterator(m_options_ptr->begin() + m_highlight); }
	ConstIterator currentI() const { return ConstIterator(m_options_ptr->begin() + m_highlight); }
	ValueIterator currentVI() { return ValueIterator(m_options_ptr->begin() + m_highlight); }
	ConstValueIterator currentVI() const { return ConstValueIterator(m_options_ptr->begin() + m_highlight); }
	
	Iterator begin() { return Iterator(m_options_ptr->begin()); }
	ConstIterator begin() const { return ConstIterator(m_options_ptr->begin()); }
	Iterator end() { return Iterator(m_options_ptr->end()); }
	ConstIterator end() const { return ConstIterator(m_options_ptr->end()); }
	
	ReverseIterator rbegin() { return ReverseIterator(end()); }
	ConstReverseIterator rbegin() const { return ConstReverseIterator(end()); }
	ReverseIterator rend() { return ReverseIterator(begin()); }
	ConstReverseIterator rend() const { return ConstReverseIterator(begin()); }
	
	ValueIterator beginV() { return ValueIterator(m_options_ptr->begin()); }
	ConstValueIterator beginV() const { return ConstValueIterator(m_options_ptr->begin()); }
	ValueIterator endV() { return ValueIterator(m_options_ptr->end()); }
	ConstValueIterator endV() const { return ConstValueIterator(m_options_ptr->end()); }
	
	ReverseValueIterator rbeginV() { return ReverseValueIterator(endV()); }
	ConstReverseIterator rbeginV() const { return ConstReverseValueIterator(endV()); }
	ReverseValueIterator rendV() { return ReverseValueIterator(beginV()); }
	ConstReverseValueIterator rendV() const { return ConstReverseValueIterator(beginV()); }
	
private:
	struct ItemProxy
	{
		typedef Item element_type;
		
		ItemProxy() { }
		ItemProxy(Item item) : m_ptr(std::make_shared<Item>(item)) { }
		
		Item &operator*() const { return *m_ptr; }
		Item *operator->() const { return m_ptr.get(); }
		
		bool operator==(const ItemProxy &rhs) const { return m_ptr == rhs.m_ptr; }
		
	private:
		std::shared_ptr<Item> m_ptr;
	};
	
	bool isHighlightable(size_t pos)
	{
		return !(*m_options_ptr)[pos]->isSeparator()
		    && !(*m_options_ptr)[pos]->isInactive();
	}
	
	ItemDisplayer m_item_displayer;
	
	FilterFunction m_filter;
	FilterFunction m_searcher;
	
	std::vector<ItemProxy> *m_options_ptr;
	std::vector<ItemProxy> m_options;
	std::vector<ItemProxy> m_filtered_options;
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

template <typename ItemT>
Menu<ItemT>::Menu(size_t startx,
	size_t starty,
	size_t width,
	size_t height,
	const std::string &title,
	Color color,
	Border border)
	: Window(startx, starty, width, height, title, color, border),
	m_item_displayer(0),
	m_options_ptr(&m_options),
	m_beginning(0),
	m_highlight(0),
	m_highlight_color(m_base_color),
	m_highlight_enabled(true),
	m_cyclic_scroll_enabled(false),
	m_autocenter_cursor(false)
{
}

template <typename ItemT>
Menu<ItemT>::Menu(const Menu &rhs)
: Window(rhs)
, m_item_displayer(rhs.m_item_displayer)
, m_filter(rhs.m_filter)
, m_searcher(rhs.m_searcher)
, m_found_positions(rhs.m_found_positions)
, m_beginning(rhs.m_beginning)
, m_highlight(rhs.m_highlight)
, m_highlight_color(rhs.m_highlight_color)
, m_highlight_enabled(rhs.m_highlight_enabled)
, m_cyclic_scroll_enabled(rhs.m_cyclic_scroll_enabled)
, m_autocenter_cursor(rhs.m_autocenter_cursor)
, m_drawn_position(rhs.m_drawn_position)
, m_selected_prefix(rhs.m_selected_prefix)
, m_selected_suffix(rhs.m_selected_suffix)
{
	// there is no way to properly fill m_filtered_options
	// (if rhs is filtered), so we just don't do it.
	m_options.reserve(rhs.m_options.size());
	for (auto it = rhs.m_options.begin(); it != rhs.m_options.end(); ++it)
		m_options.push_back(ItemProxy(**it));
	m_options_ptr = &m_options;
}

template <typename ItemT>
Menu<ItemT>::Menu(Menu &&rhs)
: Window(rhs)
, m_item_displayer(rhs.m_item_displayer)
, m_filter(rhs.m_filter)
, m_searcher(rhs.m_searcher)
, m_options(std::move(rhs.m_options))
, m_filtered_options(std::move(rhs.m_filtered_options))
, m_found_positions(std::move(rhs.m_found_positions))
, m_beginning(rhs.m_beginning)
, m_highlight(rhs.m_highlight)
, m_highlight_color(rhs.m_highlight_color)
, m_highlight_enabled(rhs.m_highlight_enabled)
, m_cyclic_scroll_enabled(rhs.m_cyclic_scroll_enabled)
, m_autocenter_cursor(rhs.m_autocenter_cursor)
, m_drawn_position(rhs.m_drawn_position)
, m_selected_prefix(std::move(rhs.m_selected_prefix))
, m_selected_suffix(std::move(rhs.m_selected_suffix))
{
	if (rhs.m_options_ptr == &rhs.m_options)
		m_options_ptr = &m_options;
	else
		m_options_ptr = &m_filtered_options;
}

template <typename ItemT>
Menu<ItemT> &Menu<ItemT>::operator=(Menu rhs)
{
	std::swap(static_cast<Window &>(*this), static_cast<Window &>(rhs));
	std::swap(m_item_displayer, rhs.m_item_displayer);
	std::swap(m_filter, rhs.m_filter);
	std::swap(m_searcher, rhs.m_searcher);
	std::swap(m_options, rhs.m_options);
	std::swap(m_filtered_options, rhs.m_filtered_options);
	std::swap(m_found_positions, rhs.m_found_positions);
	std::swap(m_beginning, rhs.m_beginning);
	std::swap(m_highlight, rhs.m_highlight);
	std::swap(m_highlight_color, rhs.m_highlight_color);
	std::swap(m_highlight_enabled, rhs.m_highlight_enabled);
	std::swap(m_cyclic_scroll_enabled, rhs.m_cyclic_scroll_enabled);
	std::swap(m_autocenter_cursor, rhs.m_autocenter_cursor);
	std::swap(m_drawn_position, rhs.m_drawn_position);
	std::swap(m_selected_prefix, rhs.m_selected_prefix);
	std::swap(m_selected_suffix, rhs.m_selected_suffix);
	if (rhs.m_options_ptr == &rhs.m_options)
		m_options_ptr = &m_options;
	else
		m_options_ptr = &m_filtered_options;
	return *this;
}

template <typename ItemT>
void Menu<ItemT>::reserve(size_t size_)
{
	m_options.reserve(size_);
}

template <typename ItemT>
void Menu<ItemT>::resizeList(size_t new_size)
{
	if (new_size > m_options.size())
	{
		size_t old_size = m_options.size();
		m_options.resize(new_size);
		for (size_t i = old_size; i < new_size; ++i)
			m_options[i] = Item();
	}
	else
		m_options.resize(new_size);
}

template <typename ItemT>
void Menu<ItemT>::addItem(ItemT item, bool is_bold, bool is_inactive)
{
	m_options.push_back(Item(std::forward<ItemT>(item), is_bold, is_inactive));
}

template <typename ItemT>
void Menu<ItemT>::addSeparator()
{
	m_options.push_back(Item::mkSeparator());
}

template <typename ItemT>
void Menu<ItemT>::insertItem(size_t pos, const ItemT &item, bool is_bold, bool is_inactive)
{
	m_options.insert(m_options.begin()+pos, Item(item, is_bold, is_inactive));
}

template <typename ItemT>
void Menu<ItemT>::insertSeparator(size_t pos)
{
	m_options.insert(m_options.begin()+pos, Item::mkSeparator());
}

template <typename ItemT>
void Menu<ItemT>::deleteItem(size_t pos)
{
	assert(m_options_ptr != &m_filtered_options);
	assert(pos < m_options.size());
	m_options.erase(m_options.begin()+pos);
}

template <typename ItemT>
bool Menu<ItemT>::Goto(size_t y)
{
	if (!isHighlightable(m_beginning+y))
		return false;
	m_highlight = m_beginning+y;
	return true;
}

template <typename ItemT>
void Menu<ItemT>::refresh()
{
	if (m_options_ptr->empty())
	{
		Window::clear();
		Window::refresh();
		return;
	}
	
	size_t max_beginning = 0;
	if (m_options_ptr->size() > m_height)
		max_beginning = m_options_ptr->size() - m_height;
	m_beginning = std::min(m_beginning, max_beginning);
	
	// if highlighted position is off the screen, make it visible
	m_highlight = std::min(m_highlight, m_beginning+m_height-1);
	// if highlighted position is invalid, correct it
	m_highlight = std::min(m_highlight, m_options_ptr->size()-1);
	
	if (!isHighlightable(m_highlight))
	{
		scroll(Scroll::Up);
		if (!isHighlightable(m_highlight))
			scroll(Scroll::Down);
	}
	
	size_t line = 0;
	m_drawn_position = m_beginning;
	for (size_t &i = m_drawn_position; i < m_beginning+m_height; ++i, ++line)
	{
		goToXY(0, line);
		if (i >= m_options_ptr->size())
		{
			for (; line < m_height; ++line)
				mvwhline(m_window, line, 0, KEY_SPACE, m_width);
			break;
		}
		if ((*m_options_ptr)[i]->isSeparator())
		{
			mvwhline(m_window, line, 0, 0, m_width);
			continue;
		}
		if ((*m_options_ptr)[i]->isBold())
			*this << Format::Bold;
		if (m_highlight_enabled && i == m_highlight)
		{
			*this << Format::Reverse;
			*this << m_highlight_color;
		}
		mvwhline(m_window, line, 0, KEY_SPACE, m_width);
		if ((*m_options_ptr)[i]->isSelected())
			*this << m_selected_prefix;
		if (m_item_displayer)
			m_item_displayer(*this);
		if ((*m_options_ptr)[i]->isSelected())
			*this << m_selected_suffix;
		if (m_highlight_enabled && i == m_highlight)
		{
			*this << Color::End;
			*this << Format::NoReverse;
		}
		if ((*m_options_ptr)[i]->isBold())
			*this << Format::NoBold;
	}
	Window::refresh();
}

template <typename ItemT>
void Menu<ItemT>::scroll(Scroll where)
{
	if (m_options_ptr->empty())
		return;
	size_t max_highlight = m_options_ptr->size()-1;
	size_t max_beginning = m_options_ptr->size() < m_height ? 0 : m_options_ptr->size()-m_height;
	size_t max_visible_highlight = m_beginning+m_height-1;
	switch (where)
	{
		case Scroll::Up:
		{
			if (m_highlight <= m_beginning && m_highlight > 0)
				--m_beginning;
			if (m_highlight == 0)
			{
				if (m_cyclic_scroll_enabled)
					return scroll(Scroll::End);
				break;
			}
			else
				--m_highlight;
			if (!isHighlightable(m_highlight))
				scroll(m_highlight == 0 && !m_cyclic_scroll_enabled ? Scroll::Down : Scroll::Up);
			break;
		}
		case Scroll::Down:
		{
			if (m_highlight >= max_visible_highlight && m_highlight < max_highlight)
				++m_beginning;
			if (m_highlight == max_highlight)
			{
				if (m_cyclic_scroll_enabled)
					return scroll(Scroll::Home);
				break;
			}
			else
				++m_highlight;
			if (!isHighlightable(m_highlight))
				scroll(m_highlight == max_highlight && !m_cyclic_scroll_enabled ? Scroll::Up : Scroll::Down);
			break;
		}
		case Scroll::PageUp:
		{
			if (m_cyclic_scroll_enabled && m_highlight == 0)
				return scroll(Scroll::End);
			if (m_highlight < m_height)
				m_highlight = 0;
			else
				m_highlight -= m_height;
			if (m_beginning < m_height)
				m_beginning = 0;
			else
				m_beginning -= m_height;
			if (!isHighlightable(m_highlight))
				scroll(m_highlight == 0 && !m_cyclic_scroll_enabled ? Scroll::Down : Scroll::Up);
			break;
		}
		case Scroll::PageDown:
		{
			if (m_cyclic_scroll_enabled && m_highlight == max_highlight)
				return scroll(Scroll::Home);
			m_highlight += m_height;
			m_beginning += m_height;
			m_beginning = std::min(m_beginning, max_beginning);
			m_highlight = std::min(m_highlight, max_highlight);
			if (!isHighlightable(m_highlight))
				scroll(m_highlight == max_highlight && !m_cyclic_scroll_enabled ? Scroll::Up : Scroll::Down);
			break;
		}
		case Scroll::Home:
		{
			m_highlight = 0;
			m_beginning = 0;
			if (!isHighlightable(m_highlight))
				scroll(Scroll::Down);
			break;
		}
		case Scroll::End:
		{
			m_highlight = max_highlight;
			m_beginning = max_beginning;
			if (!isHighlightable(m_highlight))
				scroll(Scroll::Up);
			break;
		}
	}
	if (m_autocenter_cursor)
		highlight(m_highlight);
}

template <typename ItemT>
void Menu<ItemT>::reset()
{
	m_highlight = 0;
	m_beginning = 0;
}

template <typename ItemT>
void Menu<ItemT>::clear()
{
	clearFilterResults();
	m_options.clear();
	m_found_positions.clear();
	m_options_ptr = &m_options;
}

template <typename ItemT>
void Menu<ItemT>::highlight(size_t pos)
{
	assert(pos < m_options_ptr->size());
	m_highlight = pos;
	size_t half_height = m_height/2;
	if (pos < half_height)
		m_beginning = 0;
	else
		m_beginning = pos-half_height;
}

template <typename ItemT>
size_t Menu<ItemT>::choice() const
{
	assert(!empty());
	return m_highlight;
}

template <typename ItemT>
void Menu<ItemT>::filter(ConstIterator first, ConstIterator last, const FilterFunction &f)
{
	assert(m_options_ptr != &m_filtered_options);
	clearFilterResults();
	m_filter = f;
	for (auto it = first; it != last; ++it)
		if (m_filter(*it))
			m_filtered_options.push_back(*it.base());
	if (m_filtered_options == m_options)
		m_filtered_options.clear();
	else
		m_options_ptr = &m_filtered_options;
}

template <typename ItemT>
void Menu<ItemT>::applyCurrentFilter(ConstIterator first, ConstIterator last)
{
	assert(m_filter);
	filter(first, last, m_filter);
}


template <typename ItemT>
void Menu<ItemT>::clearFilterResults()
{
	m_filtered_options.clear();
	m_options_ptr = &m_options;
}

template <typename ItemT>
void Menu<ItemT>::clearFilter()
{
	m_filter = 0;
}

template <typename ItemT>
bool Menu<ItemT>::search(ConstIterator first, ConstIterator last, const FilterFunction &f)
{
	m_found_positions.clear();
	m_searcher = f;
	for (auto it = first; it != last; ++it)
	{
		if (m_searcher(*it))
		{
			size_t pos = it-begin();
			m_found_positions.insert(pos);
		}
	}
	return !m_found_positions.empty();
}

template <typename ItemT>
void Menu<ItemT>::clearSearchResults()
{
	m_found_positions.clear();
}

template <typename ItemT>
void Menu<ItemT>::nextFound(bool wrap)
{
	if (m_found_positions.empty())
		return;
	auto next = m_found_positions.upper_bound(m_highlight);
	if (next != m_found_positions.end())
		highlight(*next);
	else if (wrap)
		highlight(*m_found_positions.begin());
}

template <typename ItemT>
void Menu<ItemT>::prevFound(bool wrap)
{
	if (m_found_positions.empty())
		return;
	auto prev = m_found_positions.lower_bound(m_highlight);
	if (prev != m_found_positions.begin())
		highlight(*--prev);
	else if (wrap)
		highlight(*m_found_positions.rbegin());
}

}

#endif // NCMPCPP_MENU_H
