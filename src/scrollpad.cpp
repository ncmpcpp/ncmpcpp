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

#include <cassert>
#include <boost/regex.hpp>
#include <iostream>

#include "scrollpad.h"

namespace {//

template <typename PropT>
bool regexSearch(NC::Buffer &buf, PropT begin, const std::string &ws, PropT end, size_t id, boost::regex::flag_type flags)
{
	try {
		boost::regex rx(ws, flags);
		auto first = boost::sregex_iterator(buf.str().begin(), buf.str().end(), rx);
		auto last = boost::sregex_iterator();
		bool success = first != last;
		for (; first != last; ++first)
		{
			buf.setProperty(first->position(), begin, id);
			buf.setProperty(first->position() + first->length(), end, id);
		}
		return success;
	} catch (boost::bad_expression &e) {
		std::cerr << "regexSearch: bad_expression: " << e.what() << "\n";
		return false;
	}
}

}

namespace NC {//

Scrollpad::Scrollpad(size_t startx,
size_t starty,
size_t width,
size_t height,
const std::string &title,
Color color,
Border border)
: Window(startx, starty, width, height, title, color, border),
m_beginning(0),
m_real_height(height)
{
}

void Scrollpad::refresh()
{
	assert(m_real_height >= m_height);
	size_t max_beginning = m_real_height - m_height;
	m_beginning = std::min(m_beginning, max_beginning);
	prefresh(m_window, m_beginning, 0, m_start_y, m_start_x, m_start_y+m_height-1, m_start_x+m_width-1);
}

void Scrollpad::resize(size_t new_width, size_t new_height)
{
	adjustDimensions(new_width, new_height);
	recreate(new_width, new_height);
	flush();
}

void Scrollpad::scroll(Scroll where)
{
	assert(m_real_height >= m_height);
	size_t max_beginning = m_real_height - m_height;
	switch (where)
	{
		case Scroll::Up:
		{
			if (m_beginning > 0)
				--m_beginning;
			break;
		}
		case Scroll::Down:
		{
			if (m_beginning < max_beginning)
				++m_beginning;
			break;
		}
		case Scroll::PageUp:
		{
			if (m_beginning > m_height)
				m_beginning -= m_height;
			else
				m_beginning = 0;
			break;
		}
		case Scroll::PageDown:
		{
			m_beginning = std::min(m_beginning + m_height, max_beginning);
			break;
		}
		case Scroll::Home:
		{
			m_beginning = 0;
			break;
		}
		case Scroll::End:
		{
			m_beginning = max_beginning;
			break;
		}
	}
}

void Scrollpad::clear()
{
	m_real_height = m_height;
	m_buffer.clear();
	werase(m_window);
	delwin(m_window);
	m_window = newpad(m_height, m_width);
	setTimeout(m_window_timeout);
	setColor(m_color, m_bg_color);
	keypad(m_window, 1);
}

const std::string &Scrollpad::buffer()
{
	return m_buffer.str();
}

void Scrollpad::flush()
{
	auto &w = static_cast<Window &>(*this);
	const auto &s = m_buffer.str();
	const auto &ps = m_buffer.properties();
	auto p = ps.begin();
	size_t i = 0;
	
	auto load_properties = [&]() {
		for (; p != ps.end() && p->position() == i; ++p)
			w << *p;
	};
	auto write_whitespace = [&]() {
		for (; i < s.length() && iswspace(s[i]); ++i)
		{
			load_properties();
			w << s[i];
		}
	};
	auto write_word = [&](bool load_properties_) {
		for (; i < s.length() && !iswspace(s[i]); ++i)
		{
			if (load_properties_)
				load_properties();
			w << s[i];
		}
	};
	auto write_buffer = [&](bool generate_height_only) -> size_t {
		int new_y;
		size_t height = 1;
		size_t old_i;
		auto old_p = p;
		int x, y;
		i = 0;
		p = ps.begin();
		y = getY();
		while (i < s.length())
		{
			// write all whitespaces.
			write_whitespace();
			
			// if we are generating height, check difference
			// between previous Y coord and current one and
			// update height accordingly.
			if (generate_height_only)
			{
				new_y = getY();
				height += new_y - y;
				y = new_y;
			}
			
			if (i == s.length())
				break;
			
			// save current string position state and get current
			// coordinates as we are before the beginning of a word.
			old_i = i;
			old_p = p;
			x = getX();
			y = getY();
			
			// write word to test if it overflows, but do not load properties
			// yet since if it overflows, we do not want to load them twice.
			write_word(false);
			
			// restore previous indexes state
			i = old_i;
			p = old_p;
			
			// get new Y coord to see if word overflew into next line.
			new_y = getY();
			if (new_y != y)
			{
				if (generate_height_only)
				{
					// if it did, let's update height...
					++height;
				}
				else
				{
					// ...or go to old coordinates, erase
					// part of the string from previous line...
					goToXY(x, y);
					wclrtoeol(m_window);
				}
				
				// ...start at the beginning of next line...
				++y;
				goToXY(0, y);
				
				// ...write word again, this time with properties...
				write_word(true);
				
				if (generate_height_only)
				{
					// ... and check for potential
					// difference in Y coordinates again.
					new_y = getY();
					height += new_y - y;
				}
			}
			else
			{
				// word didn't overflow, rewrite it with properties.
				goToXY(x, y);
				write_word(true);
			}
			
			if (generate_height_only)
			{
				// move to the first line, since when we do
				// generation, m_real_height = m_height and we
				// don't have many lines to use.
				goToXY(getX(), 0);
				y = 0;
			}
		}
		// load remaining properties if there are any
		for (; p != ps.end(); ++p)
			w << *p;
		return height;
	};
	m_real_height = std::max(write_buffer(true), m_height);
	if (m_real_height > m_height)
		recreate(m_width, m_real_height);
	else
		werase(m_window);
	write_buffer(false);
}

void Scrollpad::reset()
{
	m_beginning = 0;
}

bool Scrollpad::setProperties(Color begin, const std::string &s, Color end, size_t id, boost::regex::flag_type flags)
{
	return regexSearch(m_buffer, begin, s, end, id, flags);
}

bool Scrollpad::setProperties(Format begin, const std::string &s, Format end, size_t id, boost::regex::flag_type flags)
{
	return regexSearch(m_buffer, begin, s, end, id, flags);
}

void Scrollpad::removeProperties(size_t id)
{
	m_buffer.removeProperties(id);
}

}
