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

#include <cassert>
#include "regexes.h"

Regex::Regex() : m_cflags(0), m_compiled(false) { }

Regex::Regex(const std::string &regex_, int cflags)
: m_regex(regex_), m_cflags(cflags), m_compiled(false)
{
	compile();
}

Regex::Regex (const Regex &rhs)
: m_regex(rhs.m_regex), m_cflags(rhs.m_cflags), m_compiled(false)
{
	if (rhs.m_compiled)
		compile();
}

Regex::~Regex()
{
	if (m_compiled)
		regfree(&m_rx);
}

const std::string &Regex::regex() const
{
	return m_regex;
}

const std::string &Regex::error() const
{
	return m_error;
}

bool Regex::compile()
{
	if (m_compiled)
	{
		m_error.clear();
		regfree(&m_rx);
	}
	int comp_res = regcomp(&m_rx, m_regex.c_str(), m_cflags);
	bool result = true;
	if (comp_res != 0)
	{
		char buf[256];
		regerror(comp_res, &m_rx, buf, sizeof(buf));
		m_error = buf;
		result = false;
	}
	m_compiled = result;
	return result;
}

bool Regex::compile(const std::string &regex_, int cflags)
{
	m_regex = regex_;
	m_cflags = cflags;
	return compile();
}

bool Regex::match(const std::string &s) const
{
	assert(m_compiled);
	return regexec(&m_rx, s.c_str(), 0, 0, 0) == 0;
}

Regex &Regex::operator=(const Regex &rhs)
{
	if (this == &rhs)
		return *this;
	m_regex = rhs.m_regex;
	m_cflags = rhs.m_cflags;
	if (rhs.m_compiled)
		compile();
	return *this;
}
