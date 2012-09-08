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

#include <cassert>

#include "scrollpad.h"
#include "utility/wide_string.h"

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
			m_found_value_begin(-1),
			m_found_value_end(-1),
			m_real_height(height)
{
}

void Scrollpad::flush()
{
	m_real_height = 1;
	
	std::wstring s = m_buffer.str();
	
	size_t x = 0;
	int x_pos = 0;
	int space_pos = 0;
	
	for (size_t i = 0; i < s.length(); ++i)
	{
		x += s[i] != '\t' ? wcwidth(s[i]) : 8-x%8; // tab size
		
		if (s[i] == ' ') // if space, remember its position;
		{
			space_pos = i;
			x_pos = x;
		}
		
		if (x >= m_width)
		{
			// if line is over, there was at least one space in this line and we are in the middle of the word, restore position to last known space and make it EOL
			if (space_pos > 0 && (s[i] != ' ' || s[i+1] != ' '))
			{
				i = space_pos;
				x = x_pos;
				s[i] = '\n';
			}
		}
		
		if (x >= m_width || s[i] == '\n')
		{
			m_real_height++;
			x = 0;
			space_pos = 0;
		}
	}
	m_real_height = std::max(m_height, m_real_height);
	recreate(m_width, m_real_height);
	// print our modified string
	std::swap(s, m_buffer.m_string);
	static_cast<Window &>(*this) << m_buffer;
	// restore original one
	std::swap(s, m_buffer.m_string);
}

bool Scrollpad::setFormatting(short val_b, const std::wstring &s, short val_e, bool case_sensitive, bool for_each)
{
	bool result = m_buffer.setFormatting(val_b, s, val_e, case_sensitive, for_each);
	if (result)
	{
		m_found_for_each = for_each;
		m_found_case_sensitive = case_sensitive;
		m_found_value_begin = val_b;
		m_found_value_end = val_e;
		m_found_pattern = s;
	}
	else
		forgetFormatting();
	return result;
}

void Scrollpad::forgetFormatting()
{
	m_found_value_begin = -1;
	m_found_value_end = -1;
	m_found_pattern.clear();
}

void Scrollpad::removeFormatting()
{
	if (m_found_value_begin >= 0 && m_found_value_end >= 0)
		m_buffer.removeFormatting(m_found_value_begin, m_found_pattern, m_found_value_end, m_found_case_sensitive, m_found_for_each);
}

void Scrollpad::refresh()
{
	assert(m_real_height >= m_real_height);
	size_t max_beginning = m_real_height - m_height;
	m_beginning = std::min(m_beginning, max_beginning);
	prefresh(m_window, m_beginning, 0, m_start_y, m_start_x, m_start_y+m_height-1, m_start_x+m_width-1);
}

void Scrollpad::resize(size_t new_width, size_t new_height)
{
	adjustDimensions(new_width, new_height);
	flush();
}

void Scrollpad::scroll(Where where)
{
	assert(m_real_height >= m_height);
	size_t max_beginning = m_real_height - m_height;
	switch (where)
	{
		case wUp:
		{
			if (m_beginning > 0)
				--m_beginning;
			break;
		}
		case wDown:
		{
			if (m_beginning < max_beginning)
				++m_beginning;
			break;
		}
		case wPageUp:
		{
			if (m_beginning > m_height)
				m_beginning -= m_height;
			else
				m_beginning = 0;
			break;
		}
		case wPageDown:
		{
			m_beginning = std::min(m_beginning + m_height, max_beginning);
			break;
		}
		case wHome:
		{
			m_beginning = 0;
			break;
		}
		case wEnd:
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
	wclear(m_window);
	delwin(m_window);
	m_window = newpad(m_height, m_width);
	setTimeout(m_window_timeout);
	setColor(m_color, m_bg_color);
	forgetFormatting();
	keypad(m_window, 1);
}

void Scrollpad::reset()
{
	m_beginning = 0;
}

Scrollpad &Scrollpad::operator<<(const std::string &s)
{
	m_buffer << ToWString(s);
	return *this;
}

}
