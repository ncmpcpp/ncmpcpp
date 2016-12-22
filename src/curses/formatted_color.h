/***************************************************************************
 *   Copyright (C) 2008-2016 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_FORMATTED_COLOR_H
#define NCMPCPP_FORMATTED_COLOR_H

#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include "window.h"

namespace NC {

struct FormattedColor
{
	struct End
	{
		End(const FormattedColor &fc)
			: m_fc(fc)
		{ }

		const FormattedColor &base() const { return m_fc; }

	private:
		const FormattedColor &m_fc;
	};

	typedef std::vector<Format> Formats;

	FormattedColor() { }

	FormattedColor(Color color_, Formats formats_);

	const Color &color() const { return m_color; }
	const Formats &formats() const { return m_formats; }

private:
	Color m_color;
	Formats m_formats;
};

std::istream &operator>>(std::istream &is, FormattedColor &fc);

template <typename OutputStreamT>
OutputStreamT &operator<<(OutputStreamT &os, const FormattedColor &fc)
{
	os << fc.color();
	for (auto &fmt : fc.formats())
		os << fmt;
	return os;
}

template <typename OutputStreamT>
OutputStreamT &operator<<(OutputStreamT &os, const FormattedColor::End &rfc)
{
	if (rfc.base().color() != Color::Default)
		os << Color::End;
	for (auto &fmt : boost::adaptors::reverse(rfc.base().formats()))
		os << reverseFormat(fmt);
	return os;
}

}

#endif // NCMPCPP_FORMATTED_COLOR_H
