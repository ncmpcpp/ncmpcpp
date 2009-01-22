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

struct FormatPos
{
	size_t Position;
	short Value;
	
	bool operator<(const FormatPos &f)
	{
		return Position < f.Position;
	}
};

template <class C> class basic_buffer
{
	std::basic_ostringstream<C> itsString;
	std::list<FormatPos> itsFormat;
	std::basic_string<C> *itsTempString;
	
	public:
		basic_buffer() : itsTempString(0) { }
		
		std::basic_string<C> Str() const;
		void SetFormatting(short vb, const std::basic_string<C> &s, short ve, bool for_each = 1);
		void SetTemp(std::basic_string<C> *);
		void Clear();
		
		template <class T> basic_buffer<C> &operator<<(const T &t)
		{
			itsString << t;
			return *this;
		}
		
		basic_buffer<C> &operator<<(std::ostream &(*os)(std::ostream &));
		basic_buffer<C> &operator<<(const Color &color);
		basic_buffer<C> &operator<<(const Format &f);
		basic_buffer<C> &operator<<(const basic_buffer<C> &buf);
		
		friend Window &operator<< <>(Window &, const basic_buffer<C> &);
};

typedef basic_buffer<char> Buffer;
typedef basic_buffer<wchar_t> WBuffer;

template <class C> std::basic_string<C> basic_buffer<C>::Str() const
{
	return itsString.str();
}

template <class C> void basic_buffer<C>::SetFormatting(short vb, const std::basic_string<C> &s, short ve, bool for_each)
{
	std::basic_string<C> base = itsString.str();
	FormatPos fp;
	
	for (size_t i = base.find(s); i != std::basic_string<C>::npos; i = base.find(s))
	{
		base[i] = 0;
		fp.Value = vb;
		fp.Position = i;
		itsFormat.push_back(fp);
		fp.Value = ve;
		fp.Position = i+s.length();
		itsFormat.push_back(fp);
		if (!for_each)
			break;
	}
}

template <class C> void basic_buffer<C>::SetTemp(std::basic_string<C> *tmp)
{
	itsTempString = tmp;
}

template <class C> void basic_buffer<C>::Clear()
{
	itsString.str(std::basic_string<C>());
	itsFormat.clear();
}

template <class C> basic_buffer<C> &basic_buffer<C>::operator<<(std::ostream &(*os)(std::ostream&))
{
	itsString << os;
	return *this;
}

template <class C> basic_buffer<C> &basic_buffer<C>::operator<<(const Color &color)
{
	FormatPos f;
	f.Position = itsString.str().length();
	f.Value = color;
	itsFormat.push_back(f);
	return *this;
}

template <class C> basic_buffer<C> &basic_buffer<C>::operator<<(const Format &f)
{
	return operator<<(Color(f));
}

template <class C> basic_buffer<C> &basic_buffer<C>::operator<<(const basic_buffer<C> &buf)
{
	size_t len = itsString.str().length();
	itsString << buf.itsString.str();
	std::list<FormatPos> tmp = buf.itsFormat;
	if (len)
		for (std::list<FormatPos>::iterator it = tmp.begin(); it != tmp.end(); it++)
			it->Position += len;
	itsFormat.merge(tmp);
	return *this;
}

template <class C> Window &operator<<(Window &w, const basic_buffer<C> &buf)
{
	const std::basic_string<C> &s = buf.itsTempString ? *buf.itsTempString : buf.itsString.str();
	if (buf.itsFormat.empty())
	{
		w << s;
	}
	else
	{
		std::basic_string<C> tmp;
		std::list<FormatPos>::const_iterator b = buf.itsFormat.begin(), e = buf.itsFormat.end();
		for (size_t i = 0; i < s.length() || b != e; i++)
		{
			while (b != e && i == b->Position)
			{
				if (!tmp.empty())
				{
					w << tmp;
					tmp.clear();
				}
				if (b->Value < 100)
					w << Color(b->Value);
				else
					w << Format(b->Value);
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

