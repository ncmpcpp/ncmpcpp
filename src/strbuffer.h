/***************************************************************************
 *   Copyright (C) 2008-2013 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_STRBUFFER_H
#define NCMPCPP_STRBUFFER_H

#include <boost/lexical_cast.hpp>
#include <set>
#include "window.h"

namespace NC {//

/// Buffer template class that stores text
/// along with its properties (colors/formatting).
template <typename CharT> class BasicBuffer
{
	struct Property
	{
		enum class Type { Color, Format };
		
		Property(size_t position_, NC::Color color_, int id_)
		: m_type(Type::Color), m_position(position_), m_color(color_), m_id(id_) { }
		Property(size_t position_, NC::Format format_, int id_)
		: m_type(Type::Format), m_position(position_), m_format(format_), m_id(id_) { }
		
		size_t position() const { return m_position; }
		size_t id() const { return m_id; }
		
		bool operator<(const Property &rhs) const
		{
			if (m_position != rhs.m_position)
				return m_position < rhs.m_position;
			if (m_type != rhs.m_type)
				return m_type < rhs.m_type;
			switch (m_type)
			{
				case Type::Color:
					if (m_color != rhs.m_color)
						return m_color < rhs.m_color;
					break;
				case Type::Format:
					if (m_format != rhs.m_format)
						return m_format < rhs.m_format;
					break;
			}
			return m_id < rhs.m_id;
		}
		
		friend Window &operator<<(Window &w, const Property &p)
		{
			switch (p.m_type)
			{
				case Type::Color:
					w << p.m_color;
					break;
				case Type::Format:
					w << p.m_format;
					break;
			}
			return w;
		}
		
	private:
		Type m_type;
		size_t m_position;
		Color m_color;
		Format m_format;
		size_t m_id;
	};
	
public:
	typedef std::basic_string<CharT> StringType;
	typedef std::set<Property> Properties;
	
	BasicBuffer() { }
	
	const StringType &str() const { return m_string; }
	const Properties &properties() const { return m_properties; }
	
	template <typename PropertyT>
	void setProperty(size_t position, PropertyT property, size_t id = -1)
	{
		m_properties.insert(Property(position, property, id));
	}
	
	template <typename PropertyT>
	bool removeProperty(size_t position, PropertyT property, size_t id = -1)
	{
		auto it = m_properties.find(Property(position, property, id));
		bool found = it != m_properties.end();
		if (found)
			m_properties.erase(it);
		return found;
	}
	
	void removeProperties(size_t id = -1)
	{
		auto it = m_properties.begin();
		while (it != m_properties.end())
		{
			if (it->id() == id)
				m_properties.erase(it++);
			else
				++it;
		}
	}
	
	void clear()
	{
		m_string.clear();
		m_properties.clear();
	}
	
	BasicBuffer<CharT> &operator<<(int n)
	{
		m_string += boost::lexical_cast<StringType>(n);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(long int n)
	{
		m_string += boost::lexical_cast<StringType>(n);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(unsigned int n)
	{
		m_string += boost::lexical_cast<StringType>(n);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(unsigned long int n)
	{
		m_string += boost::lexical_cast<StringType>(n);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(CharT c)
	{
		m_string += c;
		return *this;
	}

	BasicBuffer<CharT> &operator<<(const CharT *s)
	{
		m_string += s;
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(const StringType &s)
	{
		m_string += s;
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(Color color)
	{
		setProperty(m_string.size(), color);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(Format format)
	{
		setProperty(m_string.size(), format);
		return *this;
	}
	
private:
	StringType m_string;
	Properties m_properties;
};

typedef BasicBuffer<char> Buffer;
typedef BasicBuffer<wchar_t> WBuffer;

template <typename CharT>
Window &operator<<(Window &w, const BasicBuffer<CharT> &buffer)
{
	if (buffer.properties().empty())
		w << buffer.str();
	else
	{
		auto &s = buffer.str();
		auto &ps = buffer.properties();
		auto p = ps.begin();
		for (size_t i = 0; i < s.size(); ++i)
		{
			for (; p != ps.end() && p->position() == i; ++p)
				w << *p;
			w << s[i];
		}
		// load remaining properties
		for (; p != ps.end(); ++p)
			w << *p;
	}
	return w;
}

}

#endif // NCMPCPP_STRBUFFER_H
