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

#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/range/detail/any_iterator.hpp>
#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <set>

#include "strbuffer.h"
#include "window.h"

namespace NC {

struct List
{
	struct Properties
	{
		enum Type {
			None       = 0,
			Bold       = (1 << 0),
			Selectable = (1 << 1),
			Selected   = (1 << 2),
			Inactive   = (1 << 3),
			Separator  = (1 << 4)
		};

		Properties(Type properties = Selectable)
		: m_properties(properties)
		{ }

		void setBold(bool is_bold)
		{
			if (is_bold)
				m_properties |= Bold;
			else
				m_properties &= ~Bold;
		}
		void setSelectable(bool is_selectable)
		{
			if (is_selectable)
				m_properties |= Selectable;
			else
				m_properties &= ~(Selectable | Selected);
		}
		void setSelected(bool is_selected)
		{
			if (!isSelectable())
				return;
			if (is_selected)
				m_properties |= Selected;
			else
				m_properties &= ~Selected;
		}
		void setInactive(bool is_inactive)
		{
			if (is_inactive)
				m_properties |= Inactive;
			else
				m_properties &= ~Inactive;
		}
		void setSeparator(bool is_separator)
		{
			if (is_separator)
				m_properties |= Separator;
			else
				m_properties &= ~Separator;
		}

		bool isBold() const { return m_properties & Bold; }
		bool isSelectable() const { return m_properties & Selectable; }
		bool isSelected() const { return m_properties & Selected; }
		bool isInactive() const { return m_properties & Inactive; }
		bool isSeparator() const { return m_properties & Separator; }

	private:
		unsigned m_properties;
	};

	template <typename ValueT>
	using PropertiesIterator = boost::range_detail::any_iterator<
		ValueT,
		boost::random_access_traversal_tag,
		ValueT &,
		std::ptrdiff_t
	>;

	typedef PropertiesIterator<Properties> Iterator;
	typedef PropertiesIterator<const Properties> ConstIterator;

	virtual ~List() { }

	virtual bool empty() const = 0;
	virtual size_t size() const = 0;
	virtual size_t choice() const = 0;
	virtual void highlight(size_t pos) = 0;

	virtual Iterator currentP() = 0;
	virtual ConstIterator currentP() const = 0;
	virtual Iterator beginP() = 0;
	virtual ConstIterator beginP() const = 0;
	virtual Iterator endP() = 0;
	virtual ConstIterator endP() const = 0;
};

inline List::Properties::Type operator|(List::Properties::Type lhs, List::Properties::Type rhs)
{
	return List::Properties::Type(unsigned(lhs) | unsigned(rhs));
}
inline List::Properties::Type &operator|=(List::Properties::Type &lhs, List::Properties::Type rhs)
{
	lhs = lhs | rhs;
	return lhs;
}
inline List::Properties::Type operator&(List::Properties::Type lhs, List::Properties::Type rhs)
{
	return List::Properties::Type(unsigned(lhs) & unsigned(rhs));
}
inline List::Properties::Type &operator&=(List::Properties::Type &lhs, List::Properties::Type rhs)
{
	lhs = lhs & rhs;
	return lhs;
}

// for range-based for loop
inline List::Iterator begin(List &list) { return list.beginP(); }
inline List::ConstIterator begin(const List &list) { return list.beginP(); }
inline List::Iterator end(List &list) { return list.endP(); }
inline List::ConstIterator end(const List &list) { return list.endP(); }

/// This template class is generic menu capable of
/// holding any std::vector compatible values.
template <typename ItemT> struct Menu : Window, List
{
	struct Item : List::Properties
	{
		template <bool Const>
		struct PropertiesExtractor
		{
			typedef PropertiesExtractor type;

			typedef typename std::conditional<Const, const Properties, Properties>::type Properties_;
			typedef typename std::conditional<Const, const Item, Item>::type Item_;

			Properties_ &operator()(Item_ &i) const {
				return static_cast<Properties_ &>(i);
			}
		};

		typedef ItemT Type;
		
		friend struct Menu<ItemT>;
		
		Item() { }
		Item(ItemT value_, Properties::Type properties)
		: Properties(properties)
		, m_value(value_)
		{ }
		
		ItemT &value() { return m_value; }
		const ItemT &value() const { return m_value; }
		
		ItemT &operator*() { return m_value; }
		const ItemT &operator*() const { return m_value; }

	private:
		static Item mkSeparator()
		{
			Item item;
			item.setSelectable(false);
			item.setSeparator(true);
			return item;
		}
		
		ItemT m_value;
	};
	
	typedef typename std::vector<Item>::iterator Iterator;
	typedef typename std::vector<Item>::const_iterator ConstIterator;
	typedef std::reverse_iterator<Iterator> ReverseIterator;
	typedef std::reverse_iterator<ConstIterator> ConstReverseIterator;
	
	typedef boost::indirect_iterator<
		Iterator,
		ItemT,
		boost::random_access_traversal_tag
	> ValueIterator;
	typedef boost::indirect_iterator<
		ConstIterator,
		const ItemT,
		boost::random_access_traversal_tag
	> ConstValueIterator;
	typedef std::reverse_iterator<ValueIterator> ReverseValueIterator;
	typedef std::reverse_iterator<ConstValueIterator> ConstReverseValueIterator;
	
	typedef boost::transform_iterator<
		typename Item::template PropertiesExtractor<false>,
		Iterator
	> PropertiesIterator;
	typedef boost::transform_iterator<
		typename Item::template PropertiesExtractor<true>,
		ConstIterator
	> ConstPropertiesIterator;

	/// Function helper prototype used to display each option on the screen.
	/// If not set by setItemDisplayer(), menu won't display anything.
	/// @see setItemDisplayer()
	typedef std::function<void(Menu<ItemT> &)> ItemDisplayer;
	
	Menu() { }
	
	Menu(size_t startx, size_t starty, size_t width, size_t height,
			const std::string &title, Color color, Border border);
	
	Menu(const Menu &rhs);
	Menu(Menu &&rhs);
	Menu &operator=(Menu rhs);
	
	/// Sets helper function that is responsible for displaying items
	/// @param ptr function pointer that matches the ItemDisplayer prototype
	void setItemDisplayer(const ItemDisplayer &f) { m_item_displayer = f; }
	
	/// Resizes the list to given size (adequate to std::vector::resize())
	/// @param size requested size
	void resizeList(size_t new_size);
	
	/// Adds a new option to list
	void addItem(ItemT item, Properties::Type properties = Properties::Selectable);
	
	/// Adds separator to list
	void addSeparator();
	
	/// Inserts a new option to the list at given position
	void insertItem(size_t pos, ItemT item, Properties::Type properties = Properties::Selectable);
	
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
	
	/// Checks if list is empty
	/// @return true if list is empty, false otherwise
	virtual bool empty() const OVERRIDE { return m_items.empty(); }

	/// @return size of the list
	virtual size_t size() const OVERRIDE { return m_items.size(); }

	/// @return currently highlighted position
	virtual size_t choice() const OVERRIDE;

	/// Highlights given position
	/// @param pos position to be highlighted
	virtual void highlight(size_t position) OVERRIDE;
	
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
	void setHighlightColor(Color color) { m_highlight_color = std::move(color); }
	
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
	
	/// @return currently drawn item. The result is defined only within
	/// drawing function that is called by refresh()
	/// @see refresh()
	ConstIterator drawn() const { return begin() + m_drawn_position; }
	
	/// @param pos requested position
	/// @return reference to item at given position
	/// @throw std::out_of_range if given position is out of range
	Menu<ItemT>::Item &at(size_t pos) { return m_items.at(pos); }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	/// @throw std::out_of_range if given position is out of range
	const Menu<ItemT>::Item &at(size_t pos) const { return m_items.at(pos); }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	const Menu<ItemT>::Item &operator[](size_t pos) const { return m_items[pos]; }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	Menu<ItemT>::Item &operator[](size_t pos) { return m_items[pos]; }
	
	Iterator current() { return Iterator(m_items.begin() + m_highlight); }
	ConstIterator current() const { return ConstIterator(m_items.begin() + m_highlight); }
	ReverseIterator rcurrent() { return ReverseIterator(++current()); }
	ConstReverseIterator rcurrent() const { return ReverseIterator(++current()); }

	ValueIterator currentV() { return ValueIterator(m_items.begin() + m_highlight); }
	ConstValueIterator currentV() const { return ConstValueIterator(m_items.begin() + m_highlight); }
	ReverseValueIterator rcurrentV() { return ReverseValueIterator(++currentV()); }
	ConstReverseValueIterator rcurrentV() const { return ConstReverseValueIterator(++currentV()); }
	
	Iterator begin() { return Iterator(m_items.begin()); }
	ConstIterator begin() const { return ConstIterator(m_items.begin()); }
	Iterator end() { return Iterator(m_items.end()); }
	ConstIterator end() const { return ConstIterator(m_items.end()); }
	
	ReverseIterator rbegin() { return ReverseIterator(end()); }
	ConstReverseIterator rbegin() const { return ConstReverseIterator(end()); }
	ReverseIterator rend() { return ReverseIterator(begin()); }
	ConstReverseIterator rend() const { return ConstReverseIterator(begin()); }
	
	ValueIterator beginV() { return ValueIterator(begin()); }
	ConstValueIterator beginV() const { return ConstValueIterator(begin()); }
	ValueIterator endV() { return ValueIterator(end()); }
	ConstValueIterator endV() const { return ConstValueIterator(end()); }
	
	ReverseValueIterator rbeginV() { return ReverseValueIterator(endV()); }
	ConstReverseIterator rbeginV() const { return ConstReverseValueIterator(endV()); }
	ReverseValueIterator rendV() { return ReverseValueIterator(beginV()); }
	ConstReverseValueIterator rendV() const { return ConstReverseValueIterator(beginV()); }
	
	virtual List::Iterator currentP() OVERRIDE {
		return List::Iterator(PropertiesIterator(m_items.begin() + m_highlight));
	}
	virtual List::ConstIterator currentP() const OVERRIDE {
		return List::ConstIterator(ConstPropertiesIterator(m_items.begin() + m_highlight));
	}
	virtual List::Iterator beginP() OVERRIDE {
		return List::Iterator(PropertiesIterator(m_items.begin()));
	}
	virtual List::ConstIterator beginP() const OVERRIDE {
		return List::ConstIterator(ConstPropertiesIterator(m_items.begin()));
	}
	virtual List::Iterator endP() OVERRIDE {
		return List::Iterator(PropertiesIterator(m_items.end()));
	}
	virtual List::ConstIterator endP() const OVERRIDE {
		return List::ConstIterator(ConstPropertiesIterator(m_items.end()));
	}

private:
	bool isHighlightable(size_t pos)
	{
		return !m_items[pos].isSeparator()
		    && !m_items[pos].isInactive();
	}
	
	ItemDisplayer m_item_displayer;
	
	std::vector<Item> m_items;
	
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

}

#endif // NCMPCPP_MENU_H
