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

#ifndef _SCROLLPAD_H
#define _SCROLLPAD_H

#include "window.h"
#include "strbuffer.h"

namespace NC {//

/// Scrollpad is specialized window that can hold large portion of text and
/// supports scrolling if the amount of it is bigger than the window area.
struct Scrollpad: public Window
{
	/// Constructs an empty scrollpad with given parameters
	/// @param startx X position of left upper corner of constructed window
	/// @param starty Y position of left upper corner of constructed window
	/// @param width width of constructed window
	/// @param height height of constructed window
	/// @param title title of constructed window
	/// @param color base color of constructed window
	/// @param border border of constructed window
	Scrollpad(size_t startx, size_t starty, size_t width, size_t height,
			const std::string &title, Color color, Border border);
	
	/// Prints the text stored in internal buffer to window. Note that
	/// all changes that has been made for text stored in scrollpad won't
	/// be visible until one invokes this function
	void flush();
	
	/// Searches for given string in text and sets format/color at the
	/// beginning and end of it using val_b and val_e flags accordingly
	/// @param val_b flag set at the beginning of found occurence of string
	/// @param s string that function seaches for
	/// @param val_e flag set at the end of found occurence of string
	/// @param case_sensitive indicates whether algorithm should care about case sensitivity
	/// @param for_each indicates whether function searches through whole text and sets
	/// given format for all occurences of given string or stops after first occurence
	/// @return true if at least one occurence of the string was found, false otherwise
	/// @see basic_buffer::setFormatting()
	bool setFormatting(short val_b, const std::wstring &s,
				short val_e, bool case_sensitive, bool for_each = 1);
	
	/// Removes all format flags and colors from stored text
	void forgetFormatting();
	
	/// Removes all format flags and colors that was applied
	/// by the most recent call to setFormatting() function
	/// @see setFormatting()
	/// @see basic_buffer::removeFormatting()
	void removeFormatting();
	
	/// @return text stored in internal buffer
	///
	const std::wstring &content() { return m_buffer.str(); }
	
	/// Refreshes the window
	/// @see Window::Refresh()
	///
	virtual void refresh() OVERRIDE;
	
	/// Scrolls by given amount of lines
	/// @param where indicates where exactly one wants to go
	/// @see Window::Scroll()
	///
	virtual void scroll(Where where) OVERRIDE;
	
	/// Resizes the window
	/// @param new_width new window's width
	/// @param new_height new window's height
	/// @see Window::Resize()
	///
	virtual void resize(size_t new_width, size_t new_height) OVERRIDE;
	
	/// Cleares the content of scrollpad
	/// @see Window::clear()
	///
	virtual void clear() OVERRIDE;
	
	/// Sets starting position to the beginning
	///
	void reset();
	
	/// Template function that redirects all data passed
	/// to the scrollpad window to its internal buffer
	/// @param obj any object that has ostream &operator<<() defined
	/// @return reference to itself
	///
	template <typename T> Scrollpad &operator<<(const T &obj)
	{
		m_buffer << obj;
		return *this;
	}
	Scrollpad &operator<<(const std::string &s);
	
private:
	WBuffer m_buffer;
	
	size_t m_beginning;
	
	bool m_found_for_each;
	bool m_found_case_sensitive;
	short m_found_value_begin;
	short m_found_value_end;
	std::wstring m_found_pattern;
	
	size_t m_real_height;
};

}

#endif

