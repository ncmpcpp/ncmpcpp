/*
 * Copyright (c) 2014-2017, Andrzej Rybczak <electricityispower@gmail.com>
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

#ifndef NCMPCPP_UTILITY_OPTION_PARSER_H
#define NCMPCPP_UTILITY_OPTION_PARSER_H

#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <cassert>
#include <stdexcept>
#include <unordered_map>

[[noreturn]] inline void invalid_value(const std::string &v)
{
	throw std::runtime_error("invalid value: " + v);
}

template <typename DestT>
DestT verbose_lexical_cast(const std::string &v)
{
	try {
		return boost::lexical_cast<DestT>(v);
	} catch (boost::bad_lexical_cast &) {
		invalid_value(v);
	}
}

template <typename ValueT, typename ConvertT>
std::vector<ValueT> list_of(const std::string &v, ConvertT convert)
{
	std::vector<ValueT> result;
	boost::tokenizer<boost::escaped_list_separator<char>> elems(v);
	for (auto &value : elems)
		result.push_back(convert(boost::trim_copy(value)));
	if (result.empty())
		throw std::runtime_error("empty list");
	return result;
}

template <typename ValueT>
std::vector<ValueT> list_of(const std::string &v)
{
	return list_of<ValueT>(v, verbose_lexical_cast<ValueT>);
}

bool yes_no(const std::string &v);

////////////////////////////////////////////////////////////////////////////////

class option_parser
{
	template <typename DestT>
	struct worker
	{
		template <typename MapT>
		worker(DestT *dest, MapT &&map)
			: m_dest(dest), m_map(std::forward<MapT>(map)), m_dest_set(false)
		{ }

		void operator()(std::string value)
		{
			if (m_dest_set)
				throw std::runtime_error("option already set");
			assign<DestT, void>::apply(m_dest, m_map, value);
			m_dest_set = true;
		}

	private:
		template <typename ValueT, typename VoidT>
		struct assign {
			static void apply(ValueT *dest,
			                  std::function<DestT(std::string)> &map,
			                  std::string &value)	{
				*dest = map(std::move(value));
			}
		};
		template <typename VoidT>
		struct assign<void, VoidT> {
			static void apply(void *,
			                  std::function<void(std::string)> &map,
			                  std::string &value) {
				map(std::move(value));
			}
		};

		DestT *m_dest;
		std::function<DestT(std::string)> m_map;
		bool m_dest_set;
	};

	struct parser {
		template <typename StringT, typename SetterT>
		parser(StringT &&default_, SetterT &&setter_)
			: m_used(false)
			, m_default_value(std::forward<StringT>(default_))
			, m_worker(std::forward<SetterT>(setter_))
		{ }

		bool used() const
		{
			return m_used;
		}

		void parse(std::string v)
		{
			m_worker(std::move(v));
			m_used = true;
		}

		void parse_default() const
		{
			assert(!m_used);
			m_worker(m_default_value);
		}

	private:
		bool m_used;
		std::string m_default_value;
		std::function<void(std::string)> m_worker;
	};

	std::unordered_map<std::string, parser> m_parsers;

public:
	template <typename DestT, typename MapT>
	void add(std::string option, DestT *dest, std::string default_, MapT &&map)
	{
		assert(m_parsers.count(option) == 0);
		m_parsers.emplace(
			std::move(option),
			parser(
				std::move(default_),
				worker<DestT>(dest, std::forward<MapT>(map))));
	}

	template <typename DestT>
	void add(std::string option, DestT *dest, std::string default_)
	{
		add(std::move(option), dest, std::move(default_), verbose_lexical_cast<DestT>);
	}

	bool run(std::istream &is, bool warn_on_errors);
	bool initialize_undefined(bool warn_on_errors);
};

#endif // NCMPCPP_UTILITY_OPTION_PARSER_H
