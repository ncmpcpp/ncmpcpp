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

#include <boost/regex.hpp>
#include <iostream>

#include "utility/option_parser.h"

bool option_parser::run(std::istream &is)
{
	// quoted value. leftmost and rightmost quotation marks are the delimiters.
	boost::regex quoted("(\\w+)\\h*=\\h*\"(.*)\"[^\"]*");
	// unquoted value. whitespaces get trimmed.
	boost::regex unquoted("(\\w+)\\h*=\\h*(.*?)\\h*");
	boost::smatch match;
	std::string line;
	while (std::getline(is, line))
	{
		if (boost::regex_match(line, match, quoted)
		||  boost::regex_match(line, match, unquoted))
		{
			std::string option = match[1];
			auto it = m_parsers.find(option);
			if (it != m_parsers.end())
			{
				try {
					it->second.parse(match[2]);
				} catch (std::exception &e) {
					std::cerr << "Error while processing option \"" << option << "\": " << e.what() << "\n";
					return false;
				}
			}
			else
			{
				std::cerr << "Unknown option: " << option << "\n";
				return false;
			}
		}
	}
	for (auto &p : m_parsers)
	{
		if (!p.second.defined())
		{
			try {
				p.second.run_default();
			} catch (std::exception &e) {
				std::cerr << "Error while finalizing option \"" << p.first << "\": " << e.what() << "\n";
				return false;
			}
		}
	}
	return true;
}

option_parser::worker yes_no(bool &arg, bool value)
{
	return option_parser::worker([&arg](std::string &&v) {
		if (v == "yes")
			arg = true;
		else if (v == "no")
			arg = false;
		else
			throw std::runtime_error("invalid argument: " + v);
	}, defaults_to(arg, value));
}
