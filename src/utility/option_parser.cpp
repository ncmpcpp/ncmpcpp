/*
 * Copyright (c) 2014-2021, Andrzej Rybczak <andrzej@rybczak.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <boost/regex.hpp>
#include <iostream>

#include "utility/option_parser.h"

bool yes_no(const std::string &v)
{
	if (v == "yes")
		return true;
	else if (v == "no")
		return false;
	else
		invalid_value(v);
}

std::vector<size_t> parse_ratio(const std::string &v, const std::vector<size_t>::size_type length)
{
	std::vector<size_t> ret = list_of<size_t>(v, verbose_lexical_cast<size_t>, length, "", ":", "");

	size_t total = 0;
	for (auto i : ret)
		total += i;
	if (total == 0)
		invalid_value(v);

	return ret;
}


////////////////////////////////////////////////////////////////////////////////

bool option_parser::run(std::istream &is, bool ignore_errors)
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
				try
				{
					it->second.parse(match[2]);
				}
				catch (std::exception &e)
				{
					std::cerr << "Error while processing option \"" << option
					          << "\": " << e.what() << "\n";
					if (!ignore_errors)
						return false;
				}
			}
			else
			{
				std::cerr << "Unknown option: " << option << "\n";
				if (!ignore_errors)
					return false;
			}
		}
	}
	return true;
}

bool option_parser::initialize_undefined(bool ignore_errors)
{
	for (auto &pp : m_parsers)
	{
		auto &p = pp.second;
		if (!p.used())
		{
			try
			{
				p.parse_default();
			}
			catch (std::exception &e)
			{
				std::cerr << "Error while initializing option \"" << pp.first
				          << "\": " << e.what() << "\n";
				if (ignore_errors)
					return false;
			}
		}
	}
	return true;
}
