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

#ifndef NCMPCPP_BINDINGS_H
#define NCMPCPP_BINDINGS_H

#include <algorithm>
#include <boost/bind.hpp>
#include <cassert>
#include <unordered_map>
#include "actions.h"
#include "macro_utilities.h"

/// Key for binding actions to it. Supports non-ascii characters.
struct Key
{
	enum Type { Standard, NCurses };
	
	Key(wchar_t ch, Type ct) : m_char(ch), m_type(ct) { }
	
	wchar_t getChar() const {
		return m_char;
	}
	Type getType() const {
		return m_type;
	}
	
#	define KEYS_DEFINE_OPERATOR(CMP) \
	bool operator CMP (const Key &k) const { \
		if (m_char CMP k.m_char) \
			return true; \
		if (m_char != k.m_char) \
			return false; \
		return m_type CMP k.m_type; \
	}
	KEYS_DEFINE_OPERATOR(<);
	KEYS_DEFINE_OPERATOR(<=);
	KEYS_DEFINE_OPERATOR(>);
	KEYS_DEFINE_OPERATOR(>=);
#	undef KEYS_DEFINE_OPERATOR
	
	bool operator==(const Key &k) const {
		return m_char == k.m_char && m_type == k.m_type;
	}
	bool operator!=(const Key &k) const {
		return !(*this == k);
	}
	
	static Key read(NC::Window &w);
	static Key noOp;
	
	private:
		wchar_t m_char;
		Type m_type;
};

/// Represents either single action or chain of actions bound to a certain key
struct Binding
{
	typedef std::vector<Actions::BaseAction *> ActionChain;
	
	template <typename ArgT>
	Binding(ArgT &&actions)
	: m_actions(std::forward<ArgT>(actions)) {
		assert(!m_actions.empty());
	}
	Binding(Actions::Type at)
	: Binding(ActionChain({&Actions::get(at)})) { }
	
	bool execute() const {
		return std::all_of(m_actions.begin(), m_actions.end(),
			boost::bind(&Actions::BaseAction::execute, _1)
		);
	}
	
	bool isSingle() const {
		return m_actions.size() == 1;
	}

	Actions::BaseAction *action() const {
		assert(isSingle());
		return m_actions[0];
	}

private:
	ActionChain m_actions;
};

/// Represents executable command
struct Command
{
	template <typename ArgT>
	Command(ArgT &&binding_, bool immediate_)
	: m_binding(std::forward<ArgT>(binding_)), m_immediate(immediate_) { }
	
	const Binding &binding() const { return m_binding; }
	bool immediate() const { return m_immediate; }
	
private:
	Binding m_binding;
	bool m_immediate;
};

/// Keybindings configuration
class BindingsConfiguration
{
	struct KeyHash {
		size_t operator()(const Key &k) const {
			return (k.getChar() << 1) | (k.getType() == Key::Standard);
		}
	};
	typedef std::unordered_map<std::string, Command> CommandsSet;
	typedef std::unordered_map<Key, std::vector<Binding>, KeyHash> BindingsMap;
	
public:
	typedef BindingsMap::value_type::second_type::iterator BindingIterator;
	typedef BindingsMap::value_type::second_type::const_iterator ConstBindingIterator;
	
	bool read(const std::string &file);
	void generateDefaults();
	
	const Command *findCommand(const std::string &name) {
		const Command *ptr = 0;
		auto it = m_commands.find(name);
		if (it != m_commands.end())
			ptr = &it->second;
		return ptr;
	}
	
	std::pair<BindingIterator, BindingIterator> get(const Key &k) {
		std::pair<BindingIterator, BindingIterator> result;
		auto it = m_bindings.find(k);
		if (it != m_bindings.end()) {
			result.first = it->second.begin();
			result.second = it->second.end();
		} else {
			auto list_end = m_bindings.begin()->second.end();
			result.first = list_end;
			result.second = list_end;
		}
		return result;
	}
	
	BindingsMap::const_iterator begin() const { return m_bindings.begin(); }
	BindingsMap::const_iterator end() const { return m_bindings.end(); }
	
private:
	bool notBound(const Key &k) const {
		return k != Key::noOp && m_bindings.find(k) == m_bindings.end();
	}
	
	template <typename ArgT>
	void bind(Key k, ArgT &&t) {
		m_bindings[k].push_back(std::forward<ArgT>(t));
	}
	
	BindingsMap m_bindings;
	CommandsSet m_commands;
};

extern BindingsConfiguration Bindings;

#endif // NCMPCPP_BINDINGS_H
