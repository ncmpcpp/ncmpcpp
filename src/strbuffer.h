/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include "window.h"

#include <sstream>
#include <list>

namespace NCurses
{
	template <typename C> class basic_buffer
	{
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
		
		std::basic_ostringstream<C> itsString;
		std::list<FormatPos> itsFormat;
		std::basic_string<C> *itsTempString;
		
		public:
			basic_buffer() : itsTempString(0) { }
			basic_buffer(const basic_buffer &b);
			
			std::basic_string<C> Str() const;
			bool SetFormatting(short vb, const std::basic_string<C> &s, short ve, bool for_each = 1);
			void RemoveFormatting(short vb, const std::basic_string<C> &s, short ve, bool for_each = 1);
			void SetTemp(std::basic_string<C> *);
			void Write(Window &w, size_t &pos, size_t width, const std::basic_string<C> &sep);
			void Clear();
			
			template <typename T> basic_buffer<C> &operator<<(const T &t)
			{
				itsString << t;
				return *this;
			}
			
			basic_buffer<C> &operator<<(std::ostream &(*os)(std::ostream &));
			basic_buffer<C> &operator<<(const Color &color);
			basic_buffer<C> &operator<<(const Format &f);
			basic_buffer<C> &operator<<(const basic_buffer<C> &buf);
			
			friend Window &operator<< <>(Window &, const basic_buffer<C> &);
			
		private:
			void LoadAttribute(Window &w, short value) const;
	};
	
	typedef basic_buffer<char> Buffer;
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

template <typename C> bool NCurses::basic_buffer<C>::SetFormatting(short vb, const std::basic_string<C> &s, short ve, bool for_each)
{
	if (s.empty())
		return false;
	bool result = false;
	std::basic_string<C> base = itsString.str();
	FormatPos fp;
	
	for (size_t i = base.find(s); i != std::basic_string<C>::npos; i = base.find(s, i))
	{
		result = true;
		fp.Value = vb;
		fp.Position = i;
		itsFormat.push_back(fp);
		i += s.length();
		fp.Value = ve;
		fp.Position = i;
		itsFormat.push_back(fp);
		if (!for_each)
			break;
	}
	itsFormat.sort();
	return result;
}

template <typename C> void NCurses::basic_buffer<C>::RemoveFormatting(short vb, const std::basic_string<C> &s, short ve, bool for_each)
{
	if (s.empty())
		return;
	std::basic_string<C> base = itsString.str();
	FormatPos fp;
	
	for (size_t i = base.find(s); i != std::basic_string<C>::npos; i = base.find(s, i))
	{
		fp.Value = vb;
		fp.Position = i;
		itsFormat.remove(fp);
		i += s.length();
		fp.Value = ve;
		fp.Position = i;
		itsFormat.remove(fp);
		if (!for_each)
			break;
	}
}

template <typename C> void NCurses::basic_buffer<C>::SetTemp(std::basic_string<C> *tmp)
{
	itsTempString = tmp;
}

template <typename C> void NCurses::basic_buffer<C>::Write(Window &w, size_t &pos, size_t width, const std::basic_string<C> &sep)
{
	std::basic_string<C> s = itsString.str();
	size_t len = Window::Length(s);
	
	if (len > width)
	{
		s += sep;
		len = 0;
		
		typename std::list<typename NCurses::basic_buffer<C>::FormatPos>::const_iterator lb = itsFormat.begin();
		if (itsFormat.back().Position > pos) // if there is no attributes from current position, don't load them
		{
			// load all attributes that are before start position
			for (; lb->Position < pos; ++lb)
				LoadAttribute(w, lb->Value);
		}
		
		for (size_t i = pos; i < s.length() && len < width; ++i)
		{
			while (i == lb->Position && lb != itsFormat.end())
			{
				LoadAttribute(w, lb->Value);
				++lb;
			}
			len += wcwidth(s[i]);
			w << s[i];
		}
		if (++pos >= s.length())
			pos = 0;
		
		if (len < width)
			lb = itsFormat.begin();
		for (size_t i = 0; len < width; ++i)
		{
			while (i == lb->Position && lb != itsFormat.end())
			{
				LoadAttribute(w, lb->Value);
				++lb;
			}
			len += wcwidth(s[i]);
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

template <typename C> NCurses::basic_buffer<C> &NCurses::basic_buffer<C>::operator<<(std::ostream &(*os)(std::ostream&))
{
	itsString << os;
	return *this;
}

template <typename C> NCurses::basic_buffer<C> &NCurses::basic_buffer<C>::operator<<(const Color &color)
{
	FormatPos f;
	f.Position = itsString.str().length();
	f.Value = color;
	itsFormat.push_back(f);
	return *this;
}

template <typename C> NCurses::basic_buffer<C> &NCurses::basic_buffer<C>::operator<<(const Format &f)
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

