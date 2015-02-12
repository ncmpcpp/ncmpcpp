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
#include <boost/lexical_cast.hpp>
#include <cassert>
#include <unordered_map>
#include "actions.h"
#include "macro_utilities.h"
#include "utility/wide_string.h"

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

class KeySequence
{
protected:
	std::vector<Key> sequence;

public:
	KeySequence() { }

	KeySequence(std::vector<Key> sequence) : sequence(std::move(sequence)) { }

	KeySequence(const Key &key)
	{
		sequence.push_back(key);
	}

	bool startsWith(const KeySequence &ks) const
	{
		if (ks.count() > count())
			return false;
		for (auto i = 0, e = ks.count(); i < e; ++i)
		{
			if (sequence[i] != ks.sequence[i])
				return false;
		}
		return true;
	}

	bool empty() const
	{
		return sequence.empty();
	}

	int count() const
	{
		return sequence.size();
	}

	size_t hash() const
	{
		size_t h = 0;
		for (auto k : sequence)
		{
			h <<= 16;
			h |= (k.getChar() << 1) | (k.getType() == Key::Standard);
		}
		return h;
	}

	std::string toString(bool *print_backspace) const
	{
		std::string result;
		for (auto key : sequence) {
			if (key == Key(KEY_UP, Key::NCurses))
				result += "Up";
			else if (key == Key(KEY_DOWN, Key::NCurses))
				result += "Down";
			else if (key == Key(KEY_PPAGE, Key::NCurses))
				result += "Page Up";
			else if (key == Key(KEY_NPAGE, Key::NCurses))
				result += "Page Down";
			else if (key == Key(KEY_HOME, Key::NCurses))
				result += "Home";
			else if (key == Key(KEY_END, Key::NCurses))
				result += "End";
			else if (key == Key(KEY_SPACE, Key::Standard))
				result += "Space";
			else if (key == Key(KEY_ENTER, Key::Standard))
				result += "Enter";
			else if (key == Key(KEY_IC, Key::NCurses))
				result += "Insert";
			else if (key == Key(KEY_DC, Key::NCurses))
				result += "Delete";
			else if (key == Key(KEY_RIGHT, Key::NCurses))
				result += "Right";
			else if (key == Key(KEY_LEFT, Key::NCurses))
				result += "Left";
			else if (key == Key(KEY_TAB, Key::Standard))
				result += "Tab";
			else if (key == Key(KEY_SHIFT_TAB, Key::NCurses))
				result += "Shift-Tab";
			else if (key >= Key(KEY_CTRL_A, Key::Standard) && key <= Key(KEY_CTRL_Z, Key::Standard))
			{
				result += "Ctrl-";
				result += key.getChar()+64;
			}
			else if (key >= Key(KEY_F1, Key::NCurses) && key <= Key(KEY_F12, Key::NCurses))
			{
				result += "F";
				result += boost::lexical_cast<std::string>(key.getChar()-264);
			}
			else if ((key == Key(KEY_BACKSPACE, Key::NCurses) || key == Key(KEY_BACKSPACE_2, Key::Standard)))
			{
				// since some terminals interpret KEY_BACKSPACE as backspace and other need KEY_BACKSPACE_2,
				// actions have to be bound to either of them, but we want to display "Backspace" only once,
				// hance this 'print_backspace' switch.
				if (!print_backspace || *print_backspace)
				{
					result += "Backspace";
					if (print_backspace)
						*print_backspace = false;
				}
			}
			else
				result += ToString(std::wstring(1, key.getChar()));
		}
		return result;
	}

	friend bool operator==(const KeySequence &k1, const KeySequence &k2);
};

class MutableKeySequence : public KeySequence
{
public:
	void append(const Key &key)
	{
		sequence.push_back(key);
	}

	void reset()
	{
		sequence.clear();
	}
};

/// Keybindings configuration
class BindingsConfiguration
{
	struct KeyHash {
		size_t operator()(const Key &k) const {
			return (k.getChar() << 1) | (k.getType() == Key::Standard);
		}
		size_t operator()(const KeySequence &ks) const {
            return ks.hash();
		}
	};
	typedef std::unordered_map<std::string, Command> CommandsSet;
	typedef std::unordered_map<KeySequence, std::vector<Binding>, KeyHash> BindingsMap;
	
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
		return get(KeySequence(k));
	}

	std::pair<BindingIterator, BindingIterator> get(const KeySequence &ks) {
		std::pair<BindingIterator, BindingIterator> result;
		auto it = m_bindings.find(ks);
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

	std::vector<KeySequence> getPrefix(KeySequence &ks)
	{
		std::vector<KeySequence> prefixSet;
		for (auto s : m_bindings) {
			if (s.first.startsWith(ks)) {
				prefixSet.push_back(ks);
			}
		}
		return prefixSet;
	}
	
private:
	bool notBound(const Key &k) const {
		return k != Key::noOp && m_bindings.find(k) == m_bindings.end();
	}

	bool notBound(const KeySequence &ks) const {
		return !ks.empty() && m_bindings.find(ks) == m_bindings.end();
	}
	
	template <typename ArgT>
	void bind(Key k, ArgT &&t) {
		m_bindings[KeySequence(k)].push_back(std::forward<ArgT>(t));
	}

	template <typename ArgT>
	void bind(KeySequence ks, ArgT &&t) {
		m_bindings[ks].push_back(std::forward<ArgT>(t));
	}
	
	BindingsMap m_bindings;
	CommandsSet m_commands;
};

extern BindingsConfiguration Bindings;

#endif // NCMPCPP_BINDINGS_H
