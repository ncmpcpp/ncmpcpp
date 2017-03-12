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

#ifndef NCMPCPP_SERVER_INFO_H
#define NCMPCPP_SERVER_INFO_H

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "interfaces.h"
#include "screens/screen.h"

struct ServerInfo: Screen<NC::Scrollpad>, Tabbable
{
	ServerInfo();
	
	// Screen<NC::Scrollpad> implementation
	virtual void switchTo() override;
	virtual void resize() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::ServerInfo; }
	
	virtual void update() override;
	
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return false; }
	
private:
	void SetDimensions();
	
	boost::posix_time::ptime m_timer;

	std::vector<std::string> m_url_handlers;
	std::vector<std::string> m_tag_types;
	
	size_t m_width;
	size_t m_height;
};

extern ServerInfo *myServerInfo;

#endif // NCMPCPP_SERVER_INFO_H

