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

#ifndef NCMPCPP_BINDINGS_H
#define NCMPCPP_BINDINGS_H

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include "actions.h"
#include "macro_utilities.h"

NC::Key::Type readKey(NC::Window &w);
std::wstring keyToWString(const NC::Key::Type key);

/// Represents either single action or chain of actions bound to a certain key
struct Binding
{
	typedef std::vector<std::shared_ptr<Actions::BaseAction>> ActionChain;

	template <typename ArgT>
	Binding(ArgT &&actions_)
	: m_actions(std::forward<ArgT>(actions_)) {
		assert(!m_actions.empty());
	}
	Binding(Actions::Type at)
	: Binding(ActionChain({Actions::get_(at)})) { }

	bool execute() const {
		return std::all_of(m_actions.begin(), m_actions.end(),
			std::bind(&Actions::BaseAction::execute, std::placeholders::_1)
		);
	}

	bool isSingle() const {
		return m_actions.size() == 1;
	}

	Actions::BaseAction &action() const {
		assert(isSingle());
		assert(m_actions[0] != nullptr);
		return *m_actions[0];
	}

	const ActionChain &actions() const {
		return m_actions;
	}

private:
	ActionChain m_actions;
};

/// Represents executable command
struct Command
{
	template <typename ArgT>
	Command(ArgT &&binding_, bool immediate_)
	: m_impl(std::forward<ArgT>(binding_), immediate_) { }

	const Binding &binding() const { return std::get<0>(m_impl); }
	bool immediate() const { return std::get<1>(m_impl); }

private:
	std::tuple<Binding, bool> m_impl;
};

/// Keybindings configuration
class BindingsConfiguration
{
	typedef std::unordered_map<std::string, Command> CommandsSet;
	typedef std::unordered_map<NC::Key::Type, std::vector<Binding>> BindingsMap;

public:
	typedef BindingsMap::value_type::second_type::iterator BindingIterator;
	typedef BindingsMap::value_type::second_type::const_iterator ConstBindingIterator;
	typedef std::pair<BindingIterator, BindingIterator> BindingIteratorPair;

	bool read(const std::vector<std::string> &binding_paths);
	void generateDefaults();

	const Command *findCommand(const std::string &name);
	BindingIteratorPair get(const NC::Key::Type &k);

	BindingsMap::const_iterator begin() const { return m_bindings.begin(); }
	BindingsMap::const_iterator end() const { return m_bindings.end(); }

private:
	bool notBound(const NC::Key::Type &k) const {
		return k != NC::Key::None && m_bindings.find(k) == m_bindings.end();
	}

	template <typename ArgT>
	void bind(NC::Key::Type k, ArgT &&t) {
		m_bindings[k].push_back(std::forward<ArgT>(t));
	}

	bool read(const std::string &file);

	BindingsMap m_bindings;
	CommandsSet m_commands;
};

extern BindingsConfiguration Bindings;

#endif // NCMPCPP_BINDINGS_H
