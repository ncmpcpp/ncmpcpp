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

#ifndef _REGEXES_H
#define _REGEXES_H

#include <regex.h>
#include <string>

struct Regex
{
	Regex();
	Regex(const std::string &regex, int cflags);
	Regex(const Regex &rhs);
	virtual ~Regex();
	
	/// @return regular expression
	const std::string &regex() const;
	
	/// @return compilation error (if there was any)
	const std::string &error() const;
	
	/// @return true if regular expression is compiled, false otherwise
	bool compiled() const;
	
	/// compiles regular expression
	/// @result true if compilation was successful, false otherwise
	bool compile();
	
	/// compiles regular expression
	/// @result true if compilation was successful, false otherwise
	bool compile(const std::string &regex, int cflags);
	
	/// tries to match compiled regex with given string
	/// @return true if string was matched, false otherwise
	bool match(const std::string &s) const;
	
	Regex &operator=(const Regex &rhs);
	
private:
	std::string m_regex;
	std::string m_error;
	regex_t m_rx;
	int m_cflags;
	bool m_compiled;
};

#endif // _REGEXES_H
