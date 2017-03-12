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

#include "bindings.h"
#include "global.h"
#include "macro_utilities.h"
#include "utility/string.h"
#include "utility/wide_string.h"

namespace Actions {

PushCharacters::PushCharacters(NC::Window **w, std::vector<NC::Key::Type> &&queue)
	: BaseAction(Type::MacroUtility, "push_characters")
	, m_window(w)
	, m_queue(queue)
{
	assert(w != nullptr);
	std::vector<std::string> keys;
	for (const auto &key : queue)
		keys.push_back(ToString(keyToWString(key)));
	m_name += " \"";
	m_name += join<std::string>(keys, ", ");
	m_name += "\"";
}

void PushCharacters::run()
{
	for (auto it = m_queue.begin(); it != m_queue.end(); ++it)
		(*m_window)->pushChar(*it);
}

RequireRunnable::RequireRunnable(std::shared_ptr<BaseAction> action)
	: BaseAction(Type::MacroUtility, "require_runnable")
	, m_action(std::move(action))
{
	assert(m_action != nullptr);
	m_name += " \"";
	m_name += m_action->name();
	m_name += "\"";
}

bool RequireRunnable::canBeRun()
{
	return m_action->canBeRun();
}

RequireScreen::RequireScreen(ScreenType screen_type)
	: BaseAction(Type::MacroUtility, "require_screen")
	, m_screen_type(screen_type)
{
	m_name += " \"";
	m_name +=	screenTypeToString(m_screen_type);
	m_name += "\"";
}

bool RequireScreen::canBeRun()
{
	return Global::myScreen->type() == m_screen_type;
}

RunExternalCommand::RunExternalCommand(std::string &&command)
	: BaseAction(Type::MacroUtility, "run_external_command")
	, m_command(std::move(command))
{
	m_name += " \"";
	m_name += m_command;
	m_name += "\"";
}

void RunExternalCommand::run()
{
	GNUC_UNUSED int res;
	res = std::system(m_command.c_str());
}

}
