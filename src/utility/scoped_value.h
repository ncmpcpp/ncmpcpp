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

#ifndef NCMPCPP_UTILITY_SCOPED_VALUE_H
#define NCMPCPP_UTILITY_SCOPED_VALUE_H

template <typename ValueT>
struct ScopedValue
{
	ScopedValue(ValueT &ref, ValueT &&new_value)
		: m_ref(ref)
	{
		m_value = ref;
		m_ref = std::forward<ValueT>(new_value);
	}

	~ScopedValue()
	{
		m_ref = std::move(m_value);
	}

private:
	ValueT &m_ref;
	ValueT m_value;
};

#endif // NCMPCPP_UTILITY_SCOPED_VALUE_H
