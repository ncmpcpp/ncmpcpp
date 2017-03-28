/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#include <istream>
#include "formatted_color.h"

namespace {

void verifyFormats(const NC::FormattedColor::Formats &formats)
{
	for (auto &fmt : formats)
	{
		if (fmt == NC::Format::NoBold
		    || fmt == NC::Format::NoUnderline
		    || fmt == NC::Format::NoReverse
		    || fmt == NC::Format::NoAltCharset)
			throw std::logic_error("FormattedColor can't hold disabling formats");
	}
}

}

NC::FormattedColor::FormattedColor(Color color_, Formats formats_)
{
	if (color_ == NC::Color::End)
		throw std::logic_error("FormattedColor can't hold Color::End");
	m_color = std::move(color_);
	verifyFormats(formats_);
	m_formats = std::move(formats_);
}

std::istream &NC::operator>>(std::istream &is, NC::FormattedColor &fc)
{
	NC::Color c;
	is >> c;
	if (!is.eof() && is.peek() == ':')
	{
		is.get();
		NC::FormattedColor::Formats formats;
		while (!is.eof() && isalpha(is.peek()))
		{
			char flag = is.get();
			switch (flag)
			{
			case 'b':
				formats.push_back(NC::Format::Bold);
				break;
			case 'u':
				formats.push_back(NC::Format::Underline);
				break;
			case 'r':
				formats.push_back(NC::Format::Reverse);
				break;
			case 'a':
				formats.push_back(NC::Format::AltCharset);
				break;
			default:
				is.setstate(std::ios::failbit);
				break;
			}
		}
		fc = NC::FormattedColor(c, std::move(formats));
	}
	else
		fc = NC::FormattedColor(c, {});
	return is;
}
