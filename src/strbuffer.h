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

#ifndef _STRBUFFER_H
#define _STRBUFFER_H

#include "tolower.h"
#include "window.h"

#include <sstream>
#include <list>

namespace NCurses
{
	/// Buffer template class that can store text along with its
	/// format attributes. The content can be easily printed to
	/// window or taken as raw string at any time.
	///
	template <typename C> class basic_buffer
	{
		/// Struct used for storing information about
		/// one color/format flag along with its position
		///
		struct FormatPos
		{
			size_t Position;
			short Value;
			
			bool operator<(const FormatPos &f)
			{
				return Position < f.Position;
			}
			
			bool operator==(const FormatPos &f)
			{
				return Position == f.Position && Value == f.Value;
			}
		};
		
		/// Internal buffer for storing raw text
		///
		std::basic_ostringstream<C> itsString;
		
		/// List used for storing formatting informations
		///
		std::list<FormatPos> itsFormat;
		
		/// Pointer to temporary string
		/// @see SetTemp()
		///
		std::basic_string<C> *itsTempString;
		
		public:
			/// Constructs an empty buffer
			///
			basic_buffer() : itsTempString(0) { }
			
			/// Constructs a buffer from the existed one
			/// @param b copied buffer
			///
			basic_buffer(const basic_buffer &b);
			
			/// @return raw content of the buffer without formatting informations
			///
			std::basic_string<C> Str() const;
			
			/// Searches for given string in buffer and sets format/color at the
			/// beginning and end of it using val_b and val_e flags accordingly
			/// @param val_b flag set at the beginning of found occurence of string
			/// @param s string that function seaches for
			/// @param val_e flag set at the end of found occurence of string
			/// @param case_sensitive indicates whether algorithm should care about case sensitivity
			/// @param for_each indicates whether function searches through whole buffer and sets
			/// the format for all occurences of given string or stops after the first one
			/// @return true if at least one occurence of the string was found, false otherwise
			///
			bool SetFormatting(short val_b, std::basic_string<C> s, short val_e,
					   bool case_sensitive, bool for_each = 1);
			
			/// Searches for given string in buffer and removes given
			/// format/color from the beginning and end of its occurence
			/// @param val_b flag to be removed from the beginning of the string
			/// @param s string that function seaches for
			/// @param val_e flag to be removed from the end of the string
			/// @param case_sensitive indicates whether algorithm should care about case sensitivity
			/// @param for_each indicates whether function searches through whole buffer and removes
			/// given format from all occurences of given string or stops after the first one
			///
			void RemoveFormatting(short val_b, std::basic_string<C> pattern, short val_e,
					      bool case_sensitive, bool for_each = 1);
			
			/// Removes all formating applied to string in buffer.
			///
			void RemoveFormatting();
			
			/// Sets the pointer to string, that will be passed in operator<<() to window
			/// object instead of the internal buffer. This is useful if you took the content
			/// of the buffer, modified it somehow and want to print the modified version instead
			/// of the original one, but with the original formatting informations. Note that after
			/// you're done with the printing etc., this pointer has to be set to null.
			/// @param tmp address of the temporary string
			///
			void SetTemp(std::basic_string<C> *tmp);
			
			/// Prints to window object given part of the string, loading all needed formatting info
			/// and cleaning up after. The main goal of this function is to provide interface for
			/// colorful scrollers.
			/// @param w window object that we want to print to
			/// @param start_pos reference to start position of the string. note that this variable is
			/// incremented by one after each call or set to 0 if end of string is reached
			/// @param width width of the string to be printed
			/// @param separator additional text to be placed between the end and the beginning of
			/// the string
			///
			void Write(Window &w, size_t &start_pos, size_t width,
				   const std::basic_string<C> &separator) const;
			
			/// Clears the content of the buffer and its formatting informations
			///
			void Clear();
			
			/// @param t any object that has defined ostream &operator<<()
			/// @return reference to itself
			///
			template <typename T> basic_buffer<C> &operator<<(const T &t)
			{
				itsString << t;
				return *this;
			}
			
			/// Handles colors
			/// @return reference to itself
			///
			basic_buffer<C> &operator<<(Color color);
			
			/// Handles format flags
			/// @return reference to itself
			///
			basic_buffer<C> &operator<<(Format f);
			
			/// Handles copying one buffer to another using operator<<()
			/// @param buf buffer to be copied
			/// @return reference to itself
			///
			basic_buffer<C> &operator<<(const basic_buffer<C> &buf);
			
			/// Friend operator, that handles printing
			/// the content of buffer to window object
			friend Window &operator<< <>(Window &, const basic_buffer<C> &);
			
		private:
			/// Loads an attribute to given window object
			/// @param w window object we want to load attribute to
			/// @param value value of attribute to be loaded
			///
			void LoadAttribute(Window &w, short value) const;
	};
	
	/// Standard buffer that uses narrow characters
	///
	typedef basic_buffer<char> Buffer;
	
	/// Standard buffer that uses wide characters
	///
	typedef basic_buffer<wchar_t> WBuffer;
}

template <typename C> NCurses::basic_buffer<C>::basic_buffer(const basic_buffer &b) : itsFormat(b.itsFormat),
										itsTempString(b.itsTempString)
{
	itsString << b.itsString.str();
}

template <typename C> std::basic_string<C> NCurses::basic_buffer<C>::Str() const
{
	return itsString.str();
}

template <typename C> bool NCurses::basic_buffer<C>::SetFormatting(	short val_b,
									std::basic_string<C> s,
									short val_e,
									bool case_sensitive,
									bool for_each
								  )
{
	if (s.empty())
		return false;
	bool result = false;
	std::basic_string<C> base = itsString.str();
	if (!case_sensitive)
	{
		ToLower(s);
		ToLower(base);
	}
	FormatPos fp;
	for (size_t i = base.find(s); i != std::basic_string<C>::npos; i = base.find(s, i))
	{
		result = true;
		fp.Value = val_b;
		fp.Position = i;
		itsFormat.push_back(fp);
		i += s.length();
		fp.Value = val_e;
		fp.Position = i;
		itsFormat.push_back(fp);
		if (!for_each)
			break;
	}
	itsFormat.sort();
	return result;
}

template <typename C> void NCurses::basic_buffer<C>::RemoveFormatting(	short val_b,
									std::basic_string<C> pattern,
									short val_e,
									bool case_sensitive,
									bool for_each
								     )
{
	if (pattern.empty())
		return;
	std::basic_string<C> base = itsString.str();
	if (!case_sensitive)
	{
		ToLower(pattern);
		ToLower(base);
	}
	FormatPos fp;
	for (size_t i = base.find(pattern); i != std::basic_string<C>::npos; i = base.find(pattern, i))
	{
		fp.Value = val_b;
		fp.Position = i;
		itsFormat.remove(fp);
		i += pattern.length();
		fp.Value = val_e;
		fp.Position = i;
		itsFormat.remove(fp);
		if (!for_each)
			break;
	}
}

template <typename C> void NCurses::basic_buffer<C>::RemoveFormatting()
{
	itsFormat.clear();
}

template <typename C> void NCurses::basic_buffer<C>::SetTemp(std::basic_string<C> *tmp)
{
	itsTempString = tmp;
}

template <typename C> void NCurses::basic_buffer<C>::Write(	Window &w,
								size_t &start_pos,
								size_t width,
								const std::basic_string<C> &separator
							  ) const
{
	std::basic_string<C> s = itsString.str();
	size_t len = Window::Length(s);
	
	if (len > width)
	{
		s += separator;
		len = 0;
		
		typename std::list<typename NCurses::basic_buffer<C>::FormatPos>::const_iterator lb = itsFormat.begin();
		if (itsFormat.back().Position > start_pos) // if there is no attributes from current position, don't load them
		{
			// load all attributes that are before start position
			for (; lb->Position < start_pos; ++lb)
				LoadAttribute(w, lb->Value);
		}
		
		for (size_t i = start_pos; i < s.length() && len < width; ++i)
		{
			while (i == lb->Position && lb != itsFormat.end())
			{
				LoadAttribute(w, lb->Value);
				++lb;
			}
			if ((len += wcwidth(s[i])) > width)
				break;
			w << s[i];
		}
		if (++start_pos >= s.length())
			start_pos = 0;
		
		if (len < width)
			lb = itsFormat.begin();
		for (size_t i = 0; len < width; ++i)
		{
			while (i == lb->Position && lb != itsFormat.end())
			{
				LoadAttribute(w, lb->Value);
				++lb;
			}
			if ((len += wcwidth(s[i])) > width)
				break;
			w << s[i];
		}
		// load all remained attributes to clean up
		for (; lb != itsFormat.end(); ++lb)
			LoadAttribute(w, lb->Value);
	}
	else
		w << *this;
}

template <typename C> void NCurses::basic_buffer<C>::Clear()
{
	itsString.str(std::basic_string<C>());
	itsFormat.clear();
}

template <typename C> void NCurses::basic_buffer<C>::LoadAttribute(Window &w, short value) const
{
	if (value < NCurses::fmtNone)
		w << NCurses::Color(value);
	else
		w << NCurses::Format(value);
}

template <typename C> NCurses::basic_buffer<C> &NCurses::basic_buffer<C>::operator<<(Color color)
{
	FormatPos f;
	f.Position = itsString.str().length();
	f.Value = color;
	itsFormat.push_back(f);
	return *this;
}

template <typename C> NCurses::basic_buffer<C> &NCurses::basic_buffer<C>::operator<<(Format f)
{
	return operator<<(Color(f));
}

template <typename C> NCurses::basic_buffer<C> &NCurses::basic_buffer<C>::operator<<(const NCurses::basic_buffer<C> &buf)
{
	size_t len = itsString.str().length();
	itsString << buf.itsString.str();
	std::list<FormatPos> tmp = buf.itsFormat;
	if (len)
		for (typename std::list<typename NCurses::basic_buffer<C>::FormatPos>::iterator it = tmp.begin(); it != tmp.end(); ++it)
			it->Position += len;
	itsFormat.merge(tmp);
	return *this;
}

template <typename C> NCurses::Window &operator<<(NCurses::Window &w, const NCurses::basic_buffer<C> &buf)
{
	const std::basic_string<C> &s = buf.itsTempString ? *buf.itsTempString : buf.itsString.str();
	if (buf.itsFormat.empty())
	{
		w << s;
	}
	else
	{
		std::basic_string<C> tmp;
		typename std::list<typename NCurses::basic_buffer<C>::FormatPos>::const_iterator b = buf.itsFormat.begin();
		typename std::list<typename NCurses::basic_buffer<C>::FormatPos>::const_iterator e = buf.itsFormat.end();
		for (size_t i = 0; i < s.length() || b != e; ++i)
		{
			while (b != e && i == b->Position)
			{
				if (!tmp.empty())
				{
					w << tmp;
					tmp.clear();
				}
				buf.LoadAttribute(w, b->Value);
				b++;
			}
			if (i < s.length())
				tmp += s[i];
		}
		if (!tmp.empty())
			w << tmp;
	}
	return w;
}

#endif

