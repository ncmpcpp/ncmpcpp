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

#ifndef NCMPCPP_LASTFM_H
#define NCMPCPP_LASTFM_H

#include "config.h"

#ifdef HAVE_CURL_CURL_H

#include <memory>
#include <boost/thread/future.hpp>

#include "interfaces.h"
#include "lastfm_service.h"
#include "screen.h"
#include "utility/wide_string.h"

struct Lastfm: Screen<NC::Scrollpad>, Tabbable
{
	Lastfm();
	
	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::Lastfm; }
	
	virtual void update() OVERRIDE;
	
	virtual void enterPressed() OVERRIDE { }
	virtual void spacePressed() OVERRIDE { }
	
	virtual bool isMergable() OVERRIDE { return true; }
	
	template <typename ServiceT>
	void queueJob(ServiceT &&service)
	{
		typedef typename std::remove_reference<ServiceT>::type ServiceNoRef;
		
		auto old_service = dynamic_cast<ServiceNoRef *>(m_service.get());
		// if the same service and arguments were used, leave old info
		if (old_service != nullptr && *old_service == service)
			return;
		
		m_service = std::make_shared<ServiceNoRef>(std::forward<ServiceT>(service));
		m_worker = boost::async(boost::launch::async, boost::bind(&LastFm::Service::fetch, m_service.get()));
		
		w.clear();
		w << "Fetching information...";
		w.flush();
		m_title = ToWString(m_service->name());
	}
	
protected:
	virtual bool isLockable() OVERRIDE { return false; }
	
private:
	void getResult();
	
	std::wstring m_title;
	
	std::shared_ptr<LastFm::Service> m_service;
	boost::future<LastFm::Service::Result> m_worker;
};

extern Lastfm *myLastfm;

#endif // HAVE_CURL_CURL_H

#endif // NCMPCPP_LASTFM_H

