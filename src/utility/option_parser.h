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

#ifndef NCMPCPP_UTILITY_OPTION_PARSER_H
#define NCMPCPP_UTILITY_OPTION_PARSER_H

#include <boost/lexical_cast.hpp>
#include <cassert>
#include <stdexcept>
#include <unordered_map>

#include "utility/functional.h"

struct option_parser
{
	typedef std::function<void(std::string &&)> parser_t;
	typedef std::function<void()> default_t;

	template <typename DestT, typename SourceT>
	struct assign_value_once
	{
		typedef DestT dest_type;
		typedef typename std::decay<SourceT>::type source_type;

		template <typename ArgT>
		assign_value_once(DestT &dest, ArgT &&value)
		: m_dest(dest), m_source(std::make_shared<source_type>(std::forward<ArgT>(value))) { }

		void operator()()
		{
			assert(m_source.get() != nullptr);
			m_dest = std::move(*m_source);
			m_source.reset();
		}

	private:
		DestT &m_dest;
		std::shared_ptr<source_type> m_source;
	};

	template <typename IntermediateT, typename DestT, typename TransformT>
	struct parse_and_transform
	{
		template <typename ArgT>
		parse_and_transform(DestT &dest, ArgT &&map)
		: m_dest(dest), m_map(std::forward<ArgT>(map)) { }

		void operator()(std::string &&v)
		{
			try {
				m_dest = m_map(boost::lexical_cast<IntermediateT>(v));
			} catch (boost::bad_lexical_cast &) {
				throw std::runtime_error("invalid value: " + v);
			}
		}

	private:
		DestT &m_dest;
		TransformT m_map;
	};

	struct worker
	{
		worker() { }

		template <typename ParserT, typename DefaultT>
		worker(ParserT &&p, DefaultT &&d)
		: m_defined(false), m_parser(std::forward<ParserT>(p))
		, m_default(std::forward<DefaultT>(d)) { }

		template <typename ValueT>
		void parse(ValueT &&value)
		{
			if (m_defined)
				throw std::runtime_error("option already defined");
			m_parser(std::forward<ValueT>(value));
			m_defined = true;
		}

		bool defined() const { return m_defined; }
		void run_default() const { m_default(); }

	private:
		bool m_defined;
		parser_t m_parser;
		default_t m_default;
	};

	template <typename ParserT, typename DefaultT>
	void add(const std::string &option, ParserT &&p, DefaultT &&d)
	{
		assert(m_parsers.count(option) == 0);
		m_parsers[option] = worker(std::forward<ParserT>(p), std::forward<DefaultT>(d));
	}

	template <typename WorkerT>
	void add(const std::string &option, WorkerT &&w)
	{
		assert(m_parsers.count(option) == 0);
		m_parsers[option] = std::forward<WorkerT>(w);
	}

	bool run(std::istream &is);

private:
	std::unordered_map<std::string, worker> m_parsers;
};

template <typename IntermediateT, typename ArgT, typename TransformT>
option_parser::parser_t assign(ArgT &arg, TransformT &&map = id_())
{
	return option_parser::parse_and_transform<IntermediateT, ArgT, TransformT>(
		arg, std::forward<TransformT>(map)
	);
}

template <typename ArgT, typename ValueT>
option_parser::default_t defaults_to(ArgT &arg, ValueT &&value)
{
	return option_parser::assign_value_once<ArgT, ValueT>(
		arg, std::forward<ValueT>(value)
	);
}

template <typename IntermediateT, typename ArgT, typename ValueT, typename TransformT>
option_parser::worker assign_default(ArgT &arg, ValueT &&value, TransformT &&map)
{
	return option_parser::worker(
		assign<IntermediateT>(arg, std::forward<TransformT>(map)),
		defaults_to(arg, map(std::forward<ValueT>(value)))
	);
}

template <typename ArgT, typename ValueT>
option_parser::worker assign_default(ArgT &arg, ValueT &&value)
{
	return assign_default<ArgT>(arg, std::forward<ValueT>(value), id_());
}

// workers for specific types

option_parser::worker yes_no(bool &arg, bool value);

#endif // NCMPCPP_UTILITY_OPTION_PARSER_H
