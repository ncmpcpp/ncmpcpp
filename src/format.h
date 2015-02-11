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

#ifndef NCMPCPP_HAVE_FORMAT_H
#define NCMPCPP_HAVE_FORMAT_H

#include <boost/variant.hpp>

#include "menu.h"
#include "song.h"
#include "strbuffer.h"
#include "utility/functional.h"
#include "utility/wide_string.h"

namespace Format {

namespace Flags {
const unsigned None = 0;
const unsigned Color = 1;
const unsigned Format = 2;
const unsigned OutputSwitch = 4;
const unsigned Tag = 8;
const unsigned All = Color | Format | OutputSwitch | Tag;
}

enum class ListType { Group, FirstOf, AST };

template <ListType, typename> struct List;
template <typename CharT> using Group = List<ListType::Group, CharT>;
template <typename CharT> using FirstOf = List<ListType::FirstOf, CharT>;
template <typename CharT> using AST = List<ListType::AST, CharT>;

struct OutputSwitch { };

struct SongTag
{
	SongTag(MPD::Song::GetFunction function_, unsigned delimiter_ = 0)
	: m_function(function_), m_delimiter(delimiter_)
	{ }

	MPD::Song::GetFunction function() const { return m_function; }
	unsigned delimiter() const { return m_delimiter; }

private:
	MPD::Song::GetFunction m_function;
	unsigned m_delimiter;
};

enum class Result { Empty, Missing, Ok };

// Commutative binary operation such that:
// - Empty + Empty = Empty
// - Empty + Missing = Missing
// - Empty + Ok = Ok
// - Missing + Missing = Missing
// - Missing + Ok = Missing
// - Ok + Ok = Ok
inline Result &operator+=(Result &base, Result result)
{
	if (base == Result::Missing || result == Result::Missing)
		base = Result::Missing;
	else if (base == Result::Ok || result == Result::Ok)
		base = Result::Ok;
	return base;
}

/*inline std::ostream &operator<<(std::ostream &os, Result r)
{
	switch (r)
	{
		case Result::Empty:
			os << "empty";
			break;
		case Result::Missing:
			os << "missing";
			break;
		case Result::Ok:
			os << "ok";
			break;
	}
	return os;
}*/

template <typename CharT>
using Expression = boost::variant<
	std::basic_string<CharT>,
	NC::Color,
	NC::Format,
	OutputSwitch,
	SongTag,
	boost::recursive_wrapper<FirstOf<CharT>>,
	boost::recursive_wrapper<Group<CharT>>
>;

template <ListType Type, typename CharT>
struct List
{
	typedef std::vector<Expression<CharT>> Base;

	List() { }
	List(Base &&base_)
	: m_base(std::move(base_))
	{ }

	Base &base() { return m_base; }
	const Base &base() const { return m_base; }

private:
	Base m_base;
};

template <typename CharT, typename OutputT, typename SecondOutputT = OutputT>
struct Printer: boost::static_visitor<Result>
{
	typedef std::basic_string<CharT> StringT;

	Printer(OutputT &os, const MPD::Song *song, SecondOutputT *second_os, const unsigned flags)
	: m_output(os)
	, m_song(song)
	, m_output_switched(false)
	, m_second_os(second_os)
	, m_no_output(0)
	, m_flags(flags)
	{ }

	Result operator()(const StringT &s)
	{
		if (!s.empty())
		{
			output(s);
			return Result::Ok;
		}
		else
			return Result::Empty;
	}

	Result operator()(const NC::Color &c)
	{
		if (m_flags & Flags::Color)
			output(c);
		return Result::Empty;
	}

	Result operator()(NC::Format fmt)
	{
		if (m_flags & Flags::Format)
			output(fmt);
		return Result::Empty;
	}

	Result operator()(OutputSwitch)
	{
		if (!m_no_output)
			m_output_switched = true;
		return Result::Ok;
	}

	Result operator()(const SongTag &st)
	{
		StringT tags;
		if (m_flags & Flags::Tag && m_song != nullptr)
		{
			tags = convertString<CharT, char>::apply(
				m_song->getTags(st.function())
			);
		}
		if (!tags.empty())
		{
			if (st.delimiter() > 0)
			{
				// shorten length by chopping off the tail
				if (st.function() == &MPD::Song::getLength)
					tags.resize(st.delimiter());
				else
					tags = wideShorten(tags, st.delimiter());
			}
			output(tags);
			return Result::Ok;
		}
		else
			return Result::Missing;
	}

	// If all Empty -> Empty, if any Ok -> continue with Ok, if any Missing -> stop with Empty.
	Result operator()(const Group<CharT> &group)
	{
		auto visit = [this, &group] {
			Result result = Result::Empty;
			for (const auto &ex : group.base())
			{
				result += boost::apply_visitor(*this, ex);
				if (result == Result::Missing)
				{
					result = Result::Empty;
					break;
				}
			}
			return result;
		};

		++m_no_output;
		Result result = visit();
		--m_no_output;
		if (!m_no_output && result == Result::Ok)
			visit();
		return result;
	}

	// If all Empty or Missing -> Empty, if any Ok -> stop with Ok.
	Result operator()(const FirstOf<CharT> &first_of)
	{
		for (const auto &ex : first_of.base())
		{
			if (boost::apply_visitor(*this, ex) == Result::Ok)
				return Result::Ok;
		}
		return Result::Empty;
	}

private:
	// generic version for streams (buffers, menus)
	template <typename ValueT, typename OutputStreamT>
	struct output_ {
		static void exec(OutputStreamT &os, const ValueT &value) {
			os << value;
		}
	};
	// specialization for strings (input/output)
	template <typename SomeCharT, typename OtherCharT>
	struct output_<std::basic_string<SomeCharT>, std::basic_string<OtherCharT>> {
		typedef std::basic_string<SomeCharT> SomeString;
 		typedef std::basic_string<OtherCharT> OtherString;

		// compile only if string types are the same
		static typename std::enable_if<
			std::is_same<SomeString, OtherString>::value,
			void
		>::type exec(SomeString &result, const OtherString &s) {
			result += s;
		}
	};
	// when writing to a string, we should ignore all other
	// properties. if this code is reached, throw an exception.
	template <typename ValueT, typename SomeCharT>
	struct output_<ValueT, std::basic_string<SomeCharT>> {
		static void exec(std::basic_string<CharT> &, const ValueT &) {
			throw std::logic_error("non-string property can't be appended to the string");
		}
	};

	template <typename ValueT>
	void output(const ValueT &value) const
	{
		if (!m_no_output)
		{
			if (m_output_switched && m_second_os != nullptr)
				output_<ValueT, SecondOutputT>::exec(*m_second_os, value);
			else
				output_<ValueT, OutputT>::exec(m_output, value);
		}
	}

	OutputT &m_output;
	const MPD::Song *m_song;

	bool m_output_switched;
	SecondOutputT *m_second_os;

	unsigned m_no_output;
	const unsigned m_flags;
};

template <typename CharT, typename VisitorT>
void visit(VisitorT &visitor, const AST<CharT> &ast)
{
	for (const auto &ex : ast.base())
		boost::apply_visitor(visitor, ex);
}

template <typename CharT, typename ItemT>
void print(const AST<CharT> &ast, NC::Menu<ItemT> &menu, const MPD::Song *song,
           NC::BasicBuffer<CharT> *buffer, const unsigned flags = Flags::All)
{
	Printer<CharT, NC::Menu<ItemT>, NC::Buffer> printer(menu, song, buffer, flags);
	visit(printer, ast);
}

template <typename CharT>
void print(const AST<CharT> &ast, NC::BasicBuffer<CharT> &buffer,
           const MPD::Song *song, const unsigned flags = Flags::All)
{
	Printer<CharT, NC::BasicBuffer<CharT>> printer(buffer, song, &buffer, flags);
	visit(printer, ast);
}

template <typename CharT>
std::basic_string<CharT> stringify(const AST<CharT> &ast, const MPD::Song *song)
{
	std::basic_string<CharT> result;
	Printer<CharT, std::basic_string<CharT>> printer(result, song, &result, Flags::Tag);
	visit(printer, ast);
	return result;
}

AST<char> parse(const std::string &s, const unsigned flags = Flags::All);
AST<wchar_t> parse(const std::wstring &ws, const unsigned flags = Flags::All);

}

#endif // NCMPCPP_HAVE_FORMAT_H
