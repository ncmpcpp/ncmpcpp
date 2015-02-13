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

#ifndef NCMPCPP_UTILITY_CONVERSION_H
#define NCMPCPP_UTILITY_CONVERSION_H

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_unsigned.hpp>

#include "config.h"
#include "gcc.h"

struct ConversionError
{
	ConversionError(std::string source) : m_source_value(source) { }
	
	const std::string &value() { return m_source_value; }
	
private:
	std::string m_target_type;
	std::string m_source_value;
};

struct OutOfBounds : std::exception
{
	const std::string &errorMessage() { return m_error_message; }
	
	template <typename Type>
	GNUC_NORETURN static void raise(const Type &value, const Type &lbound, const Type &ubound)
	{
		throw OutOfBounds((boost::format(
			"value is out of bounds ([%1%, %2%] expected, %3% given)") % lbound % ubound % value).str());
	}
	
	template <typename Type>
	GNUC_NORETURN static void raiseLower(const Type &value, const Type &lbound)
	{
		throw OutOfBounds((boost::format(
			"value is out of bounds ([%1%, ->) expected, %2% given)") % lbound % value).str());
	}
	
	template <typename Type>
	GNUC_NORETURN static void raiseUpper(const Type &value, const Type &ubound)
	{
		throw OutOfBounds((boost::format(
			"value is out of bounds ((<-, %1%] expected, %2% given)") % ubound % value).str());
	}
	
	virtual const char *what() const noexcept OVERRIDE { return m_error_message.c_str(); }

private:
	OutOfBounds(std::string msg) : m_error_message(msg) { }
	
	std::string m_error_message;
};

template <typename TargetT, bool isUnsigned>
struct unsigned_checker
{
	static void apply(const std::string &) { }
};
template <typename TargetT>
struct unsigned_checker<TargetT, true>
{
	static void apply(const std::string &s)
	{
		if (s[0] == '-')
			throw ConversionError(s);
	}
};

template <typename TargetT>
TargetT fromString(const std::string &source)
{
	unsigned_checker<TargetT, boost::is_unsigned<TargetT>::value>::apply(source);
	try
	{
		return boost::lexical_cast<TargetT>(source);
	}
	catch (boost::bad_lexical_cast &)
	{
		throw ConversionError(source);
	}
}

template <typename Type>
void boundsCheck(const Type &value, const Type &lbound, const Type &ubound)
{
	if (value < lbound || value > ubound)
		OutOfBounds::raise(value, lbound, ubound);
}

template <typename Type>
void lowerBoundCheck(const Type &value, const Type &lbound)
{
	if (value < lbound)
		OutOfBounds::raiseLower(value, lbound);
}

template <typename Type>
void upperBoundCheck(const Type &value, const Type &ubound)
{
	if (value > ubound)
		OutOfBounds::raiseUpper(value, ubound);
}

#endif // NCMPCPP_UTILITY_CONVERSION_H
