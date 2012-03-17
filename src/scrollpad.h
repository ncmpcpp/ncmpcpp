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

namespace NCurses
{
	/// Scrollpad is specialized window that can hold large portion of text and
	/// supports scrolling if the amount of it is bigger than the window area.
	///
	class Scrollpad: public Window
	{
		public:
			/// Constructs an empty scrollpad with given parameters
			/// @param startx X position of left upper corner of constructed window
			/// @param starty Y position of left upper corner of constructed window
			/// @param width width of constructed window
			/// @param height height of constructed window
			/// @param title title of constructed window
			/// @param color base color of constructed window
			/// @param border border of constructed window
			///
			Scrollpad(size_t startx, size_t starty, size_t width, size_t height,
				  const std::string &title, Color color, Border border);
			
			/// Copies the scrollpad
			/// @param s copied scrollpad
			///
			Scrollpad(const Scrollpad &s);
			
			/// Prints the text stored in internal buffer to window. Note that
			/// all changes that has been made for text stored in scrollpad won't
			/// be visible until one invokes this function
			///
			void Flush();
			
			/// Searches for given string in text and sets format/color at the
			/// beginning and end of it using val_b and val_e flags accordingly
			/// @param val_b flag set at the beginning of found occurence of string
			/// @param s string that function seaches for
			/// @param val_e flag set at the end of found occurence of string
			/// @param case_sensitive indicates whether algorithm should care about case sensitivity
			/// @param for_each indicates whether function searches through whole text and sets
			/// given format for all occurences of given string or stops after first occurence
			/// @return true if at least one occurence of the string was found, false otherwise
			/// @see basic_buffer::SetFormatting()
			///
			bool SetFormatting(short val_b, const std::basic_string<my_char_t> &s,
					   short val_e, bool case_sensitive, bool for_each = 1);
			
			/// Removes all format flags and colors from stored text
			///
			void ForgetFormatting();
			
			/// Removes all format flags and colors that was applied
			/// by the most recent call to SetFormatting() function
			/// @see SetFormatting()
			/// @see basic_buffer::RemoveFormatting()
			///
			void RemoveFormatting();
			
			/// @return text stored in internal buffer
			///
			std::basic_string<my_char_t> Content() { return itsBuffer.Str(); }
			
			/// Refreshes the window
			/// @see Window::Refresh()
			///
			virtual void Refresh();
			
			/// Scrolls by given amount of lines
			/// @param where indicates where exactly one wants to go
			/// @see Window::Scroll()
			///
			virtual void Scroll(Where where);
			
			/// Resizes the window
			/// @param new_width new window's width
			/// @param new_height new window's height
			/// @see Window::Resize()
			///
			virtual void Resize(size_t new_width, size_t new_height);
			
			/// Cleares the content of scrollpad
			/// @see Window::Clear()
			///
			virtual void Clear();
			
			/// Sets starting position to the beginning
			///
			void Reset();
			
			/// Template function that redirects all data passed
			/// to the scrollpad window to its internal buffer
			/// @param obj any object that has ostream &operator<<() defined
			/// @return reference to itself
			///
			template <typename T> Scrollpad &operator<<(const T &obj)
			{
				itsBuffer << obj;
				return *this;
			}
#			ifdef _UTF8
			Scrollpad &operator<<(const std::string &s);
#			endif // _UTF8
			
		private:
			basic_buffer<my_char_t> itsBuffer;
			
			int itsBeginning;
			
			bool itsFoundForEach;
			bool itsFoundCaseSensitive;
			short itsFoundValueBegin;
			short itsFoundValueEnd;
			std::basic_string<my_char_t> itsFoundPattern;
			
			size_t itsRealHeight;
	};
}

#endif

