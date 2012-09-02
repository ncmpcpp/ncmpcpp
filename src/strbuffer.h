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

#include <list>
#include "utility/numeric_conversions.h"
#include "window.h"

namespace NC {//

/// Buffer template class that can store text along with its
/// format attributes. The content can be easily printed to
/// window or taken as raw string at any time.
///
template <typename C> class basic_buffer
{
	friend class Scrollpad;
	
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
	std::basic_string<C> itsString;
	
	/// List used for storing formatting informations
	///
	std::list<FormatPos> itsFormat;
	
	
public:
	/// Constructs an empty buffer
	///
	basic_buffer() { }
	
	/// Constructs a buffer from the existed one
	/// @param b copied buffer
	///
	basic_buffer(const basic_buffer &b);
	
	/// @return raw content of the buffer without formatting informations
	///
	const std::basic_string<C> &Str() const;
	
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
	
	basic_buffer<C> &operator<<(int n)
	{
		itsString += intTo< std::basic_string<C> >::apply(n);
		return *this;
	}
	
	basic_buffer<C> &operator<<(long int n)
	{
		itsString += longIntTo< std::basic_string<C> >::apply(n);
		return *this;
	}
	
	basic_buffer<C> &operator<<(unsigned int n)
	{
		itsString += unsignedIntTo< std::basic_string<C> >::apply(n);
		return *this;
	}
	
	basic_buffer<C> &operator<<(unsigned long int n)
	{
		itsString += unsignedLongIntTo< std::basic_string<C> >::apply(n);
		return *this;
	}
	
	basic_buffer<C> &operator<<(char c)
	{
		itsString += c;
		return *this;
	}
	
	basic_buffer<C> &operator<<(wchar_t wc)
	{
		itsString += wc;
		return *this;
	}
	
	basic_buffer<C> &operator<<(const C *s)
	{
		itsString += s;
		return *this;
	}
	
	basic_buffer<C> &operator<<(const std::basic_string<C> &s)
	{
		itsString += s;
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
	
	/// Friend operator that handles printing
	/// the content of buffer to window object
	friend Window &operator<<(Window &w, const basic_buffer<C> &buf)
	{
		const std::basic_string<C> &s = buf.itsString;
		if (buf.itsFormat.empty())
			w << s;
		else
		{
			std::basic_string<C> tmp;
			auto b = buf.itsFormat.begin(), e = buf.itsFormat.end();
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
				tmp += s[i];
			}
			if (!tmp.empty())
				w << tmp;
		}
		return w;
	}
	
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

template <typename C> basic_buffer<C>::basic_buffer(const basic_buffer &b)
	: itsString(b.itsString), itsFormat(b.itsFormat) { }

template <typename C> const std::basic_string<C> &basic_buffer<C>::Str() const
{
	return itsString;
}

template <typename C> bool basic_buffer<C>::SetFormatting(
	short val_b,
	std::basic_string<C> s,
	short val_e,
	bool case_sensitive,
	bool for_each
	)
{
	if (s.empty())
		return false;
	bool result = false;
	std::basic_string<C> base = itsString;
	if (!case_sensitive)
	{
		lowercase(s);
		lowercase(base);
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

template <typename C> void basic_buffer<C>::RemoveFormatting(
	short val_b,
	std::basic_string<C> pattern,
	short val_e,
	bool case_sensitive,
	bool for_each
	)
{
	if (pattern.empty())
		return;
	std::basic_string<C> base = itsString;
	if (!case_sensitive)
	{
		lowercase(pattern);
		lowercase(base);
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

template <typename C> void basic_buffer<C>::RemoveFormatting()
{
	itsFormat.clear();
}

template <typename C> void basic_buffer<C>::Write(
	Window &w,
	size_t &start_pos,
	size_t width,
	const std::basic_string<C> &separator
	) const
{
	std::basic_string<C> s = itsString;
	size_t len = Window::Length(s);
	
	if (len > width)
	{
		s += separator;
		len = 0;
		
		auto lb = itsFormat.begin();
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

template <typename C> void basic_buffer<C>::Clear()
{
	itsString.clear();
	itsFormat.clear();
}

template <typename C> void basic_buffer<C>::LoadAttribute(Window &w, short value) const
{
	if (value < fmtNone)
		w << Color(value);
	else
		w << Format(value);
}

template <typename C> basic_buffer<C> &basic_buffer<C>::operator<<(Color color)
{
	FormatPos f;
	f.Position = itsString.length();
	f.Value = color;
	itsFormat.push_back(f);
	return *this;
}

template <typename C> basic_buffer<C> &basic_buffer<C>::operator<<(Format f)
{
	return operator<<(Color(f));
}

template <typename C> basic_buffer<C> &basic_buffer<C>::operator<<(const basic_buffer<C> &buf)
{
	size_t length = itsString.length();
	itsString += buf.itsString;
	std::list<FormatPos> tmp = buf.itsFormat;
	if (length)
		for (auto it = tmp.begin(); it != tmp.end(); ++it)
			it->Position += length;
	itsFormat.merge(tmp);
	return *this;
}

}

#endif
