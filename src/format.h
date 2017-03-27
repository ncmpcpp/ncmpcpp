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

#ifndef NCMPCPP_HAVE_FORMAT_H
#define NCMPCPP_HAVE_FORMAT_H

#include <boost/variant.hpp>

#include "curses/menu.h"
#include "song.h"

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

inline bool operator==(const SongTag &lhs, const SongTag &rhs) {
	return lhs.function() == rhs.function()
		&& lhs.delimiter() == rhs.delimiter();
}
inline bool operator!=(const SongTag &lhs, const SongTag &rhs) {
	return !(lhs == rhs);
}

template <typename CharT>
using TagVector = std::vector<
	std::pair<
		boost::optional<SongTag>,
		std::basic_string<CharT>
		>
	>;

enum class Result { Empty, Missing, Ok };

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

template <typename CharT, typename VisitorT>
void visit(VisitorT &visitor, const AST<CharT> &ast);

template <typename CharT, typename ItemT>
void print(const AST<CharT> &ast, NC::Menu<ItemT> &menu, const MPD::Song *song,
           NC::BasicBuffer<CharT> *buffer, const unsigned flags = Flags::All);

template <typename CharT>
void print(const AST<CharT> &ast, NC::BasicBuffer<CharT> &buffer,
           const MPD::Song *song, const unsigned flags = Flags::All);

template <typename CharT>
std::basic_string<CharT> stringify(const AST<CharT> &ast, const MPD::Song *song);

template <typename CharT>
TagVector<CharT> flatten(const AST<CharT> &ast, const MPD::Song &song);

AST<char> parse(const std::string &s, const unsigned flags = Flags::All);
AST<wchar_t> parse(const std::wstring &ws, const unsigned flags = Flags::All);

}

#endif // NCMPCPP_HAVE_FORMAT_H
