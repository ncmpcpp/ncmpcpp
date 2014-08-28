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
		
		template <typename OutputStreamT>
		friend OutputStreamT &operator<<(OutputStreamT &os, const Property &p)
		{
			switch (p.m_type)
			{
				case Type::Color:
					os << p.m_color;
					break;
				case Type::Format:
					os << p.m_format;
					break;
			}
			return os;
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
	
	template <typename... Args>
	BasicBuffer(Args... args)
	{
		construct(std::forward<Args>(args)...);
	}

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
	void construct() { }
	template <typename ArgT, typename... Args>
	void construct(ArgT &&arg, Args... args)
	{
		*this << std::forward<ArgT>(arg);
		construct(std::forward<Args>(args)...);
	}

	StringType m_string;
	Properties m_properties;
};

typedef BasicBuffer<char> Buffer;
typedef BasicBuffer<wchar_t> WBuffer;

template <typename OutputStreamT, typename CharT>
OutputStreamT &operator<<(OutputStreamT &os, const BasicBuffer<CharT> &buffer)
{
	if (buffer.properties().empty())
		os << buffer.str();
	else
	{
		auto &s = buffer.str();
		auto &ps = buffer.properties();
		auto p = ps.begin();
		for (size_t i = 0; i < s.size(); ++i)
		{
			for (; p != ps.end() && p->position() == i; ++p)
				os << *p;
			os << s[i];
		}
		// load remaining properties
		for (; p != ps.end(); ++p)
			os << *p;
	}
	return os;
}

}

#endif // NCMPCPP_STRBUFFER_H
