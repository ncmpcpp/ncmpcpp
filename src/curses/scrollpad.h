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

#ifndef NCMPCPP_SCROLLPAD_H
#define NCMPCPP_SCROLLPAD_H

#include "curses/window.h"
#include "curses/strbuffer.h"

namespace NC {

/// Scrollpad is specialized window that holds large portions of text and
/// supports scrolling if the amount of it is bigger than the window area.
struct Scrollpad: public Window
{
	Scrollpad() { }
	
	Scrollpad(size_t startx, size_t starty, size_t width, size_t height,
	          const std::string &title, Color color, Border border);
	
	// override a few Window functions
	virtual void refresh() override;
	virtual void scroll(Scroll where) override;
	virtual void resize(size_t new_width, size_t new_height) override;
	virtual void clear() override;
	
	const std::string &buffer();
	
	void flush();
	void reset();
	
	bool setProperties(const Color &begin, const std::string &s, const Color &end,
	                   size_t flags, size_t id = -2);
	bool setProperties(const Format &begin, const std::string &s, const Format &end,
	                   size_t flags, size_t id = -2);
	bool setProperties(const FormattedColor &fc, const std::string &s,
	                   size_t flags, size_t id = -2);
	void removeProperties(size_t id = -2);
	
	Scrollpad &operator<<(int n) { return write(n); }
	Scrollpad &operator<<(long int n) { return write(n); }
	Scrollpad &operator<<(unsigned int n) { return write(n); }
	Scrollpad &operator<<(unsigned long int n) { return write(n); }
	Scrollpad &operator<<(char c) { return write(c); }
	Scrollpad &operator<<(const char *s) { return write(s); }
	Scrollpad &operator<<(const std::string &s) { return write(s); }
	Scrollpad &operator<<(Color color) { return write(color); }
	Scrollpad &operator<<(Format format) { return write(format); }

private:
	template <typename ItemT>
	Scrollpad &write(ItemT &&item)
	{
		m_buffer << std::forward<ItemT>(item);
		return *this;
	}

	Buffer m_buffer;
	
	size_t m_beginning;
	size_t m_real_height;
};

}

#endif // NCMPCPP_SCROLLPAD_H

