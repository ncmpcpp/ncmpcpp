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

#include <stdexcept>

#include "format.h"
#include "utility/type_conversions.h"

namespace {

template <typename CharT> using string = std::basic_string<CharT>;
template <typename CharT> using iterator = typename std::basic_string<CharT>::const_iterator;
template <typename CharT> using expressions = std::vector<Format::Expression<CharT>>;

template <typename CharT>
std::string invalidCharacter(CharT c)
{
	return "invalid character '"
	     + convertString<char, CharT>::apply(boost::lexical_cast<string<CharT>>(c))
	     + "'";
}

template <typename CharT>
void throwError(const string<CharT> &s, iterator<CharT> current, std::string msg)
{
	throw std::runtime_error(
		std::move(msg) + " at position " + boost::lexical_cast<std::string>(current - s.begin())
	);
}

template <typename CharT>
void rangeCheck(const string<CharT> &s, iterator<CharT> current, iterator<CharT> end)
{
	if (current >= end)
		throwError(s, current, "unexpected end");
}

template <typename CharT>
expressions<CharT> parseBracket(const string<CharT> &s,
                                iterator<CharT> it, iterator<CharT> end)
{
	string<CharT> token;
	expressions<CharT> tmp, result;
	auto push_token = [&] {
		result.push_back(std::move(token));
	};
	for (; it != end; ++it)
	{
		if (*it == '{')
		{
			push_token();
			bool done;
			Format::Any<CharT> any;
			do
			{
				auto jt = it;
				done = true;
				// get to the corresponding closing bracket
				unsigned brackets = 1;
				while (brackets > 0)
				{
					if (++jt == end)
						break;
					if (*jt == '{')
						++brackets;
					else if (*jt == '}')
						--brackets;
				}
				// check if we're still in range
				rangeCheck(s, jt, end);
				// skip the opening bracket
				++it;
				// recursively parse the bracket
				tmp = parseBracket(s, it, jt);
				// if the inner bracket contains only one expression,
				// put it as is. otherwise require all of them.
				if (tmp.size() == 1)
					any.base().push_back(std::move(tmp[0]));
				else
					any.base().push_back(Format::All<CharT>(std::move(tmp)));
				it = jt;
				// check for the alternative
				++jt;
				if (jt != end && *jt == '|')
				{
					++jt;
					rangeCheck(s, jt, end);
					if (*jt != '{')
						throwError(s, jt, invalidCharacter(*jt) + ", expected '{'");
					it = jt;
					done = false;
				}
			}
			while (!done);
			assert(!any.base().empty());
			// if there was only one bracket, append empty branch
			// so that it always evaluates to true. otherwise put
			// it as is.
			if (any.base().size() == 1)
				any.base().push_back(string<CharT>());
			result.push_back(std::move(any));
		}
		else if (*it == '%')
		{
			++it;
			rangeCheck(s, it, end);
			// %% is escaped %
			if (*it == '%')
			{
				token += '%';
				continue;
			}
			push_token();
			// check for tag delimiter
			unsigned delimiter = 0;
			if (isdigit(*it))
			{
				std::string sdelimiter;
				do
					sdelimiter += *it++;
				while (it != end && isdigit(*it));
				rangeCheck(s, it, end);
				delimiter = boost::lexical_cast<unsigned>(sdelimiter);
			}
			auto f = charToGetFunction(*it);
			if (f == nullptr)
				throwError(s, it, invalidCharacter(*it));
			result.push_back(Format::SongTag(f, delimiter));
		}
		else if (*it == '$')
		{
			++it;
			rangeCheck(s, it, end);
			// $$ is escaped $
			if (*it == '$')
			{
				token += '$';
				continue;
			}
			push_token();
			if (isdigit(*it))
			{
				auto color = charToColor(*it);
				result.push_back(color);
			}
			else if (*it == 'R')
				result.push_back(Format::AlignRight());
			else if (*it == 'b')
				result.push_back(NC::Format::Bold);
			else if (*it == 'u')
				result.push_back(NC::Format::Underline);
			else if (*it == 'a')
				result.push_back(NC::Format::AltCharset);
			else if (*it == 'r')
				result.push_back(NC::Format::Reverse);
			else if (*it == '/')
			{
				++it;
				rangeCheck(s, it, end);
				if (*it == 'b')
					result.push_back(NC::Format::NoBold);
				else if (*it == 'u')
					result.push_back(NC::Format::NoUnderline);
				else if (*it == 'a')
					result.push_back(NC::Format::NoAltCharset);
				else if (*it == 'r')
					result.push_back(NC::Format::NoReverse);
				else
					throwError(s, it, invalidCharacter(*it));
			}
			else
				throwError(s, it, invalidCharacter(*it));
		}
		else
			token += *it;
	}
	push_token();
	return result;
}

}

namespace Format {

AST<char> parse(const std::string &s)
{
	return AST<char>(parseBracket(s, s.begin(), s.end()));
}

AST<wchar_t> wparse(const std::wstring &s)
{
	return AST<wchar_t>(parseBracket(s, s.begin(), s.end()));
}

}
