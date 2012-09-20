/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
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

#ifndef _BINDINGS_H
#define _BINDINGS_H

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
	typedef std::vector<Action *> ActionChain;
	
	Binding(ActionType at) : m_is_single(true), m_action(Action::Get(at)) { }
	Binding(const ActionChain &actions) {
		assert(actions.size() > 0);
		if (actions.size() == 1) {
			m_is_single = true;
			m_action = actions[0];
		} else {
			m_is_single = false;
			m_chain = new ActionChain(actions);
		}
	}
	
	bool execute() const {
		bool result = false;
		if (m_is_single) {
			assert(m_action);
			result = m_action->Execute();
		} else {
			for (auto it = m_chain->begin(); it != m_chain->end(); ++it)
				if (!(*it)->Execute())
					break;
			result = true;
		}
		return result;
	}
	
	bool isSingle() const {
		return m_is_single;
	}
	ActionChain *chain() const {
		assert(!m_is_single);
		return m_chain;
	}
	Action *action() const {
		assert(m_is_single);
		return m_action;
	}
	
private:
	bool m_is_single;
	union {
		Action *m_action;
		ActionChain *m_chain;
	};
};

/// Represents executable command
struct Command
{
	Command(const Binding &binding_, bool immediate_)
	: m_binding(binding_), m_immediate(immediate_) { }
	
	const Binding &binding() const { return m_binding; }
	bool immediate() const { return m_immediate; }
	
private:
	Binding m_binding;
	bool m_immediate;
};

/// Keybindings configuration
class BindingsConfiguration
{
	typedef std::unordered_map<std::string, Command> CommandsSet;
	typedef std::multimap<Key, Binding> BindingsMap;
	
public:
	typedef BindingsMap::iterator BindingIterator;
	typedef BindingsMap::const_iterator ConstBindingIterator;
	
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
		return m_bindings.equal_range(k);
	}
	
	ConstBindingIterator begin() const { return m_bindings.begin(); }
	ConstBindingIterator end() const { return m_bindings.end(); }
	
private:
	bool notBound(const Key &k) const {
		return k != Key::noOp && m_bindings.find(k) == m_bindings.end();
	}
	
	template <typename T> void bind(Key k, T t) {
		m_bindings.insert(std::make_pair(k, Binding(t)));
	}
	
	BindingsMap m_bindings;
	CommandsSet m_commands;
};

extern BindingsConfiguration Bindings;

#endif // _BINDINGS_H
