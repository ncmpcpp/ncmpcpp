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

#ifndef NCMPCPP_MACRO_UTILITIES_H
#define NCMPCPP_MACRO_UTILITIES_H

#include <cassert>
#include "actions.h"
#include "screens/screen_type.h"

namespace Actions {

struct PushCharacters: BaseAction
{
	PushCharacters(NC::Window **w, std::vector<NC::Key::Type> &&queue);

private:
	virtual void run() override;
	
	NC::Window **m_window;
	std::vector<NC::Key::Type> m_queue;
};

struct RequireRunnable: BaseAction
{
	RequireRunnable(std::shared_ptr<BaseAction> action);
	
private:
	virtual bool canBeRun() override;
	virtual void run() override { }
	
	std::shared_ptr<BaseAction> m_action;
};

struct RequireScreen: BaseAction
{
	RequireScreen(ScreenType screen_type);
	
private:
	virtual bool canBeRun() override;
	virtual void run() override { }
	
	ScreenType m_screen_type;
};

struct RunExternalCommand: BaseAction
{
	RunExternalCommand(std::string &&command);
	
private:
	virtual void run() override;
	
	std::string m_command;
};

}

#endif // NCMPCPP_MACRO_UTILITIES_H
