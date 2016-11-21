/***************************************************************************
 *   Copyright (C) 2008-2016 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_LYRICS_H
#define NCMPCPP_LYRICS_H


#include <boost/optional.hpp>
#include <future>
#include <memory>
#include <queue>

#include "interfaces.h"
#include "lyrics_fetcher.h"
#include "screen.h"
#include "song.h"
#include "utility/shared_resource.h"

struct Lyrics: Screen<NC::Scrollpad>, Tabbable
{
	Lyrics();
	
	// Screen<NC::Scrollpad> implementation
	virtual void resize() override;
	virtual void switchTo() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::Lyrics; }
	
	virtual void update() override;
	
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return true; }

	// other members
	void fetch(const MPD::Song &s);
	void refetchCurrent();
	void edit();
	void toggleFetcher();

	void fetchInBackground(const MPD::Song &s, bool notify_);
	boost::optional<std::string> tryTakeConsumerMessage();

private:
	struct ConsumerState
	{
		struct Song
		{
			Song()
				: m_notify(false)
			{ }

			Song(const MPD::Song &s, bool notify_)
				: m_song(s), m_notify(notify_)
			{ }

			const MPD::Song &song() const { return m_song; }
			bool notify() const { return m_notify; }

		private:
			MPD::Song m_song;
			bool m_notify;
		};

		ConsumerState()
			: running(false)
		{ }

		bool running;
		std::queue<Song> songs;
		boost::optional<std::string> message;
	};

	void clearWorker();

	bool m_refresh_window;
	size_t m_scroll_begin;

	std::shared_ptr<Shared<NC::Buffer>> m_shared_buffer;

	MPD::Song m_song;
	LyricsFetcher *m_fetcher;
	std::future<boost::optional<std::string>> m_worker;

	Shared<ConsumerState> m_consumer_state;
};

extern Lyrics *myLyrics;

#endif // NCMPCPP_LYRICS_H
