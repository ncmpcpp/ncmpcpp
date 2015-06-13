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

#ifndef NCMPCPP_UTILITY_FUNCTIONAL_H
#define NCMPCPP_UTILITY_FUNCTIONAL_H

#include <boost/locale/encoding_utf.hpp>
#include <utility>

template <typename BaseT>
struct reversed_iteration
{
	reversed_iteration(BaseT &base_)
	: base(base_) { }

	BaseT &base;
};

template <typename BaseT>
reversed_iteration<BaseT> reverse_iteration(BaseT &base_) {
	return reversed_iteration<BaseT>(base_);
}
template <typename BaseT>
auto begin(reversed_iteration<BaseT> &rev) -> decltype(rev.base.rbegin()) {
	return rev.base.rbegin();
}
template <typename BaseT>
auto begin(reversed_iteration<const BaseT> &rev) -> decltype(rev.base.rbegin()) {
	return rev.base.rbegin();
}
template <typename BaseT>
auto end(reversed_iteration<BaseT> &rev) -> decltype(rev.base.rend()) {
	return rev.base.rend();
}
template <typename BaseT>
auto end(reversed_iteration<const BaseT> &rev) -> decltype(rev.base.rend()) {
	return rev.base.rend();
}

template <typename ValueT>
struct pointer_extractor
{
	typedef pointer_extractor type;
	ValueT *operator()(ValueT &v) const { return &v; }
};

/// Map over the first element in range satisfying the predicate.
template <typename InputIterator, typename PredicateT, typename MapT>
InputIterator find_map_first(InputIterator first, InputIterator last, PredicateT &&p, MapT &&f)
{
	auto it = std::find(first, last, std::forward<PredicateT>(p));
	if (it != last)
		f(*it);
	return it;
}

/// Map over all elements in range satisfying the predicate.
template <typename InputIterator, typename PredicateT, typename MapT>
void find_map_all(InputIterator first, InputIterator last, PredicateT &&p, MapT &&f)
{
	InputIterator it = first;
	do
		it = find_map_first(it, last, p, f);
	while (it != last);
}

// identity function object
struct id_
{
	template <typename ValueT>
	constexpr auto operator()(ValueT &&v) const noexcept
		-> decltype(std::forward<ValueT>(v))
	{
		return std::forward<ValueT>(v);
	}
};

// convert string to appropriate type
template <typename TargetT, typename SourceT>
struct convertString
{
	static std::basic_string<TargetT> apply(const std::basic_string<SourceT> &s)
	{
		return boost::locale::conv::utf_to_utf<TargetT>(s);
	}
};
template <typename TargetT>
struct convertString<TargetT, TargetT>
{
	static const std::basic_string<TargetT> &apply(const std::basic_string<TargetT> &s)
	{
		return s;
	}
};


#endif // NCMPCPP_UTILITY_FUNCTIONAL_H
