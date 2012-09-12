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
#include "utility/wide_string.h"
#include "window.h"

namespace NC {//

/// Buffer template class that can store text along with its
/// format attributes. The content can be easily printed to
/// window or taken as raw string at any time.
template <typename CharT> class BasicBuffer
{
	friend struct Scrollpad;
	
	/// Struct used for storing information about
	/// one color/format flag along with its position
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
	std::basic_string<CharT> m_string;
	
	/// List used for storing formatting informations
	std::list<FormatPos> m_format;
	
public:
	/// Constructs an empty buffer
	BasicBuffer() { }
	
	/// Constructs a buffer from the existed one
	/// @param b copied buffer
	BasicBuffer(const BasicBuffer &b);
	
	/// @return raw content of the buffer without formatting informations
	const std::basic_string<CharT> &str() const;
	
	/// Searches for given string in buffer and sets format/color at the
	/// beginning and end of it using val_b and val_e flags accordingly
	/// @param val_b flag set at the beginning of found occurence of string
	/// @param s string that function seaches for
	/// @param val_e flag set at the end of found occurence of string
	/// @param case_sensitive indicates whether algorithm should care about case sensitivity
	/// @param for_each indicates whether function searches through whole buffer and sets
	/// the format for all occurences of given string or stops after the first one
	/// @return true if at least one occurence of the string was found, false otherwise
	bool setFormatting(short val_b, std::basic_string<CharT> s, short val_e,
						bool case_sensitive, bool for_each = 1);
	
	/// Searches for given string in buffer and removes given
	/// format/color from the beginning and end of its occurence
	/// @param val_b flag to be removed from the beginning of the string
	/// @param s string that function seaches for
	/// @param val_e flag to be removed from the end of the string
	/// @param case_sensitive indicates whether algorithm should care about case sensitivity
	/// @param for_each indicates whether function searches through whole buffer and removes
	/// given format from all occurences of given string or stops after the first one
	void removeFormatting(short val_b, std::basic_string<CharT> pattern, short val_e,
							bool case_sensitive, bool for_each = 1);
	
	/// Removes all formating applied to string in buffer.
	void removeFormatting();
	
	/// Prints to window object given part of the string, loading all needed formatting info
	/// and cleaning up after. The main goal of this function is to provide interface for
	/// colorful scrollers.
	/// @param w window object that we want to print to
	/// @param start_pos reference to start position of the string. note that this variable is
	/// incremented by one after each call or set to 0 if end of string is reached
	/// @param width width of the string to be printed
	/// @param separator additional text to be placed between the end and the beginning of
	/// the string
	void write(Window &w, size_t &start_pos, size_t width,
				const std::basic_string<CharT> &separator) const;
	
	/// Clears the content of the buffer and its formatting informations
	void clear();
	
	BasicBuffer<CharT> &operator<<(int n)
	{
		m_string += intTo< std::basic_string<CharT> >::apply(n);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(long int n)
	{
		m_string += longIntTo< std::basic_string<CharT> >::apply(n);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(unsigned int n)
	{
		m_string += unsignedIntTo< std::basic_string<CharT> >::apply(n);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(unsigned long int n)
	{
		m_string += unsignedLongIntTo< std::basic_string<CharT> >::apply(n);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(char c)
	{
		m_string += c;
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(wchar_t wc)
	{
		m_string += wc;
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(const CharT *s)
	{
		m_string += s;
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(const std::basic_string<CharT> &s)
	{
		m_string += s;
		return *this;
	}
	
	/// Handles colors
	/// @return reference to itself
	BasicBuffer<CharT> &operator<<(Color color);
	
	/// Handles format flags
	/// @return reference to itself
	BasicBuffer<CharT> &operator<<(Format f);
	
	/// Handles copying one buffer to another using operator<<()
	/// @param buf buffer to be copied
	/// @return reference to itself
	BasicBuffer<CharT> &operator<<(const BasicBuffer<CharT> &buf);
	
	/// Friend operator that handles printing
	/// the content of buffer to window object
	friend Window &operator<<(Window &w, const BasicBuffer<CharT> &buf)
	{
		const std::basic_string<CharT> &s = buf.m_string;
		if (buf.m_format.empty())
			w << s;
		else
		{
			std::basic_string<CharT> tmp;
			auto b = buf.m_format.begin(), e = buf.m_format.end();
			for (size_t i = 0; i < s.length() || b != e; ++i)
			{
				while (b != e && i == b->Position)
				{
					if (!tmp.empty())
					{
						w << tmp;
						tmp.clear();
					}
					buf.loadAttribute(w, b->Value);
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
	void loadAttribute(Window &w, short value) const;
};

/// Standard buffer that uses narrow characters
typedef BasicBuffer<char> Buffer;

/// Standard buffer that uses wide characters
typedef BasicBuffer<wchar_t> WBuffer;

template <typename CharT> BasicBuffer<CharT>::BasicBuffer(const BasicBuffer &b)
	: m_string(b.m_string), m_format(b.m_format) { }

template <typename CharT> const std::basic_string<CharT> &BasicBuffer<CharT>::str() const
{
	return m_string;
}

template <typename CharT> bool BasicBuffer<CharT>::setFormatting(
	short val_b,
	std::basic_string<CharT> s,
	short val_e,
	bool case_sensitive,
	bool for_each
	)
{
	if (s.empty())
		return false;
	bool result = false;
	std::basic_string<CharT> base;
	if (case_sensitive)
		base = m_string;
	else
	{
		base = lowercase(m_string);
		s = lowercase(s);
	}
	FormatPos fp;
	for (size_t i = base.find(s); i != std::basic_string<CharT>::npos; i = base.find(s, i))
	{
		result = true;
		fp.Value = val_b;
		fp.Position = i;
		m_format.push_back(fp);
		i += s.length();
		fp.Value = val_e;
		fp.Position = i;
		m_format.push_back(fp);
		if (!for_each)
			break;
	}
	m_format.sort();
	return result;
}

template <typename CharT> void BasicBuffer<CharT>::removeFormatting(
	short val_b,
	std::basic_string<CharT> pattern,
	short val_e,
	bool case_sensitive,
	bool for_each
	)
{
	if (pattern.empty())
		return;
	std::basic_string<CharT> base;
	if (case_sensitive)
		base = m_string;
	else
	{
		base = lowercase(base);
		pattern = lowercase(pattern);
	}
	FormatPos fp;
	for (size_t i = base.find(pattern); i != std::basic_string<CharT>::npos; i = base.find(pattern, i))
	{
		fp.Value = val_b;
		fp.Position = i;
		m_format.remove(fp);
		i += pattern.length();
		fp.Value = val_e;
		fp.Position = i;
		m_format.remove(fp);
		if (!for_each)
			break;
	}
}

template <typename CharT> void BasicBuffer<CharT>::removeFormatting()
{
	m_format.clear();
}

template <typename CharT> void BasicBuffer<CharT>::write(
	Window &w,
	size_t &start_pos,
	size_t width,
	const std::basic_string<CharT> &separator
	) const
{
	std::basic_string<CharT> s = m_string;
	size_t len = wideLength(s);
	
	if (len > width)
	{
		s += separator;
		len = 0;
		
		auto lb = m_format.begin();
		if (m_format.back().Position > start_pos) // if there is no attributes from current position, don't load them
		{
			// load all attributes that are before start position
			for (; lb->Position < start_pos; ++lb)
				loadAttribute(w, lb->Value);
		}
		
		for (size_t i = start_pos; i < s.length() && len < width; ++i)
		{
			while (i == lb->Position && lb != m_format.end())
			{
				loadAttribute(w, lb->Value);
				++lb;
			}
			if ((len += wcwidth(s[i])) > width)
				break;
			w << s[i];
		}
		if (++start_pos >= s.length())
			start_pos = 0;
		
		if (len < width)
			lb = m_format.begin();
		for (size_t i = 0; len < width; ++i)
		{
			while (i == lb->Position && lb != m_format.end())
			{
				loadAttribute(w, lb->Value);
				++lb;
			}
			if ((len += wcwidth(s[i])) > width)
				break;
			w << s[i];
		}
		// load all remained attributes to clean up
		for (; lb != m_format.end(); ++lb)
			loadAttribute(w, lb->Value);
	}
	else
		w << *this;
}

template <typename CharT> void BasicBuffer<CharT>::clear()
{
	m_string.clear();
	m_format.clear();
}

template <typename CharT> void BasicBuffer<CharT>::loadAttribute(Window &w, short value) const
{
	if (value < fmtNone)
		w << Color(value);
	else
		w << Format(value);
}

template <typename CharT> BasicBuffer<CharT> &BasicBuffer<CharT>::operator<<(Color color)
{
	FormatPos f;
	f.Position = m_string.length();
	f.Value = color;
	m_format.push_back(f);
	return *this;
}

template <typename CharT> BasicBuffer<CharT> &BasicBuffer<CharT>::operator<<(Format f)
{
	return operator<<(Color(f));
}

template <typename CharT> BasicBuffer<CharT> &BasicBuffer<CharT>::operator<<(const BasicBuffer<CharT> &buf)
{
	size_t length = m_string.length();
	m_string += buf.m_string;
	std::list<FormatPos> tmp = buf.m_format;
	if (length)
		for (auto it = tmp.begin(); it != tmp.end(); ++it)
			it->Position += length;
	m_format.merge(tmp);
	return *this;
}

}

#endif
