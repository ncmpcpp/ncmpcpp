/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_MENU_IMPL_H
#define NCMPCPP_MENU_IMPL_H

#include "menu.h"

namespace NC {

template <typename ItemT>
Menu<ItemT>::Menu()
{
	m_items = &m_all_items;
}

template <typename ItemT>
Menu<ItemT>::Menu(size_t startx,
                  size_t starty,
                  size_t width,
                  size_t height,
                  const std::string &title,
                  Color color,
                  Border border)
	: Window(startx, starty, width, height, title, color, border)
	, m_item_displayer(nullptr)
	, m_filter_predicate(nullptr)
	, m_beginning(0)
	, m_highlight(0)
	, m_highlight_enabled(true)
	, m_cyclic_scroll_enabled(false)
	, m_autocenter_cursor(false)
{
	auto fc = FormattedColor(m_base_color, {Format::Reverse});
	m_highlight_prefix << fc;
	m_highlight_suffix << FormattedColor::End<>(fc);
	m_items = &m_all_items;
}

template <typename ItemT>
Menu<ItemT>::Menu(const Menu &rhs)
	: Window(rhs)
	, m_item_displayer(rhs.m_item_displayer)
	, m_filter_predicate(rhs.m_filter_predicate)
	, m_beginning(rhs.m_beginning)
	, m_highlight(rhs.m_highlight)
	, m_highlight_enabled(rhs.m_highlight_enabled)
	, m_cyclic_scroll_enabled(rhs.m_cyclic_scroll_enabled)
	, m_autocenter_cursor(rhs.m_autocenter_cursor)
	, m_drawn_position(rhs.m_drawn_position)
	, m_highlight_prefix(rhs.m_highlight_prefix)
	, m_highlight_suffix(rhs.m_highlight_suffix)
	, m_selected_prefix(rhs.m_selected_prefix)
	, m_selected_suffix(rhs.m_selected_suffix)
{
	// TODO: move filtered items
	m_all_items.reserve(rhs.m_all_items.size());
	for (const auto &item : rhs.m_all_items)
		m_all_items.push_back(item.copy());
	m_items = &m_all_items;
}

template <typename ItemT>
Menu<ItemT>::Menu(Menu &&rhs)
	: Window(rhs)
	, m_item_displayer(std::move(rhs.m_item_displayer))
	, m_filter_predicate(std::move(rhs.m_filter_predicate))
	, m_all_items(std::move(rhs.m_all_items))
	, m_filtered_items(std::move(rhs.m_filtered_items))
	, m_beginning(rhs.m_beginning)
	, m_highlight(rhs.m_highlight)
	, m_highlight_enabled(rhs.m_highlight_enabled)
	, m_cyclic_scroll_enabled(rhs.m_cyclic_scroll_enabled)
	, m_autocenter_cursor(rhs.m_autocenter_cursor)
	, m_drawn_position(rhs.m_drawn_position)
	, m_highlight_prefix(std::move(rhs.m_highlight_prefix))
	, m_highlight_suffix(std::move(rhs.m_highlight_suffix))
	, m_selected_prefix(std::move(rhs.m_selected_prefix))
	, m_selected_suffix(std::move(rhs.m_selected_suffix))
{
	if (rhs.m_items == &rhs.m_all_items)
		m_items = &m_all_items;
	else
		m_items = &m_filtered_items;
}

template <typename ItemT>
Menu<ItemT> &Menu<ItemT>::operator=(Menu rhs)
{
	std::swap(static_cast<Window &>(*this), static_cast<Window &>(rhs));
	std::swap(m_item_displayer, rhs.m_item_displayer);
	std::swap(m_filter_predicate, rhs.m_filter_predicate);
	std::swap(m_all_items, rhs.m_all_items);
	std::swap(m_filtered_items, rhs.m_filtered_items);
	std::swap(m_beginning, rhs.m_beginning);
	std::swap(m_highlight, rhs.m_highlight);
	std::swap(m_highlight_enabled, rhs.m_highlight_enabled);
	std::swap(m_cyclic_scroll_enabled, rhs.m_cyclic_scroll_enabled);
	std::swap(m_autocenter_cursor, rhs.m_autocenter_cursor);
	std::swap(m_drawn_position, rhs.m_drawn_position);
	std::swap(m_highlight_prefix, rhs.m_highlight_prefix);
	std::swap(m_highlight_suffix, rhs.m_highlight_suffix);
	std::swap(m_selected_prefix, rhs.m_selected_prefix);
	std::swap(m_selected_suffix, rhs.m_selected_suffix);
	if (rhs.m_items == &rhs.m_all_items)
		m_items = &m_all_items;
	else
		m_items = &m_filtered_items;
	return *this;
}

template <typename ItemT> template <typename ItemDisplayerT>
void Menu<ItemT>::setItemDisplayer(ItemDisplayerT &&displayer)
{
	m_item_displayer = std::forward<ItemDisplayerT>(displayer);
}

template <typename ItemT>
void Menu<ItemT>::resizeList(size_t new_size)
{
	m_all_items.resize(new_size);
}

template <typename ItemT>
void Menu<ItemT>::addItem(ItemT item, Properties::Type properties)
{
	m_all_items.push_back(Item(std::move(item), properties));
}

template <typename ItemT>
void Menu<ItemT>::addSeparator()
{
	m_all_items.push_back(Item::mkSeparator());
}

template <typename ItemT>
void Menu<ItemT>::insertItem(size_t pos, ItemT item, Properties::Type properties)
{
	m_all_items.insert(m_all_items.begin()+pos, Item(std::move(item), properties));
}

template <typename ItemT>
void Menu<ItemT>::insertSeparator(size_t pos)
{
	m_all_items.insert(m_all_items.begin()+pos, Item::mkSeparator());
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
	if (m_items->empty())
	{
		Window::clear();
		Window::refresh();
		return;
	}

	size_t max_beginning = 0;
	if (m_items->size() > m_height)
		max_beginning = m_items->size() - m_height;
	m_beginning = std::min(m_beginning, max_beginning);

	// if highlighted position is off the screen, make it visible
	m_highlight = std::min(m_highlight, m_beginning+m_height-1);
	// if highlighted position is invalid, correct it
	m_highlight = std::min(m_highlight, m_items->size()-1);

	if (!isHighlightable(m_highlight))
	{
		scroll(Scroll::Up);
		if (!isHighlightable(m_highlight))
			scroll(Scroll::Down);
	}

	size_t line = 0;
	const size_t end_ = m_beginning+m_height;
	m_drawn_position = m_beginning;
	for (; m_drawn_position < end_; ++m_drawn_position, ++line)
	{
		goToXY(0, line);
		if (m_drawn_position >= m_items->size())
		{
			for (; line < m_height; ++line)
				mvwhline(m_window, line, 0, NC::Key::Space, m_width);
			break;
		}
		if ((*m_items)[m_drawn_position].isSeparator())
		{
			mvwhline(m_window, line, 0, 0, m_width);
			continue;
		}
		if (m_highlight_enabled && m_drawn_position == m_highlight)
			*this << m_highlight_prefix;
		if ((*m_items)[m_drawn_position].isSelected())
			*this << m_selected_prefix;
		*this << NC::TermManip::ClearToEOL;
		if (m_item_displayer)
			m_item_displayer(*this);
		if ((*m_items)[m_drawn_position].isSelected())
			*this << m_selected_suffix;
		if (m_highlight_enabled && m_drawn_position == m_highlight)
			*this << m_highlight_suffix;
	}
	Window::refresh();
}

template <typename ItemT>
void Menu<ItemT>::scroll(Scroll where)
{
	if (m_items->empty())
		return;
	size_t max_highlight = m_items->size()-1;
	size_t max_beginning = m_items->size() < m_height ? 0 : m_items->size()-m_height;
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
	// Don't clear filter related stuff here.
	m_all_items.clear();
	m_filtered_items.clear();
}

template <typename ItemT>
void Menu<ItemT>::highlight(size_t pos)
{
	assert(pos < m_items->size());
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

template <typename ItemT> template <typename PredicateT>
void Menu<ItemT>::applyFilter(PredicateT &&pred)
{
	m_filter_predicate = std::forward<PredicateT>(pred);
	m_filtered_items.clear();

	for (const auto &item : m_all_items)
		if (m_filter_predicate(item))
			m_filtered_items.push_back(item);

	m_items = &m_filtered_items;
}

template <typename ItemT>
void Menu<ItemT>::reapplyFilter()
{
	applyFilter(m_filter_predicate);
}

template <typename ItemT> template <typename TargetT>
const TargetT *Menu<ItemT>::filterPredicate() const
{
	return m_filter_predicate.template target<TargetT>();
}

template <typename ItemT>
void Menu<ItemT>::clearFilter()
{
	m_filter_predicate = nullptr;
	m_filtered_items.clear();
	m_items = &m_all_items;
}

}

#endif // NCMPCPP_MENU_IMPL_H
