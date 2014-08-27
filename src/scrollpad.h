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

#ifndef NCMPCPP_SCROLLPAD_H
#define NCMPCPP_SCROLLPAD_H

#include <boost/regex.hpp>
#include "window.h"
#include "strbuffer.h"

namespace NC {//

/// Scrollpad is specialized window that holds large portions of text and
/// supports scrolling if the amount of it is bigger than the window area.
struct Scrollpad: public Window
{
	Scrollpad() { }
	
	Scrollpad(size_t startx, size_t starty, size_t width, size_t height,
	          const std::string &title, Color color, Border border);
	
	// override a few Window functions
	virtual void refresh() OVERRIDE;
	virtual void scroll(Scroll where) OVERRIDE;
	virtual void resize(size_t new_width, size_t new_height) OVERRIDE;
	virtual void clear() OVERRIDE;
	
	const std::string &buffer();
	
	void flush();
	void reset();
	
	bool setProperties(Color begin, const std::string &s, Color end, size_t id = -2, boost::regex::flag_type flags = boost::regex::icase);
	bool setProperties(Format begin, const std::string &s, Format end, size_t id = -2, boost::regex::flag_type flags = boost::regex::icase);
	void removeProperties(size_t id = -2);
	
	template <typename ItemT>
	Scrollpad &operator<<(const ItemT &item)
	{
		m_buffer << item;
		return *this;
	}
	
private:
	Buffer m_buffer;
	
	size_t m_beginning;
	size_t m_real_height;
};

}

#endif // NCMPCPP_SCROLLPAD_H

