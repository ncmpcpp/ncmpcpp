/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <thread>

#include "curses/scrollpad.h"
#include "screens/browser.h"
#include "charset.h"
#include "curl_handle.h"
#include "format_impl.h"
#include "global.h"
#include "helpers.h"
#include "macro_utilities.h"
#include "screens/lyrics.h"
#include "screens/playlist.h"
#include "settings.h"
#include "song.h"
#include "statusbar.h"
#include "title.h"
#include "screens/screen_switcher.h"
#include "utility/string.h"

using Global::MainHeight;
using Global::MainStartY;

Lyrics *myLyrics;

namespace {

std::string removeExtension(std::string filename)
{
	size_t dot = filename.rfind('.');
	if (dot != std::string::npos)
		filename.resize(dot);
	return filename;
}

std::string lyricsFilename(const MPD::Song &s)
{
	std::string filename;
	if (Config.store_lyrics_in_song_dir && !s.isStream())
	{
		if (s.isFromDatabase())
			filename = Config.mpd_music_dir + "/";
		filename += removeExtension(s.getURI());
		removeExtension(filename);
	}
	else
	{
		std::string artist = s.getArtist();
		std::string title  = s.getTitle();
		if (artist.empty() || title.empty())
			filename = removeExtension(s.getName());
		else
			filename = artist + " - " + title;
		removeInvalidCharsFromFilename(filename, Config.generate_win32_compatible_filenames);
		filename = Config.lyrics_directory + "/" + filename;
	}
	filename += ".txt";
	return filename;
}

bool loadLyrics(NC::Scrollpad &w, const std::string &filename)
{
	std::ifstream input(filename);
	if (input.is_open())
	{
		std::string line;
		bool first_line = true;
		while (std::getline(input, line))
		{
			// Remove carriage returns as they mess up the display.
			boost::remove_erase(line, '\r');
			if (!first_line)
				w << '\n';
			w << Charset::utf8ToLocale(line);
			first_line = false;
		}
		return true;
	}
	else
		return false;
}

bool saveLyrics(const std::string &filename, const std::string &lyrics)
{
	std::ofstream output(filename);
	if (output.is_open())
	{
		output << lyrics;
		output.close();
		return true;
	}
	else
		return false;
}

boost::optional<std::string> downloadLyrics(
	const MPD::Song &s,
	std::shared_ptr<Shared<NC::Buffer>> shared_buffer,
	std::shared_ptr<std::atomic<bool>> download_stopper,
	LyricsFetcher *current_fetcher)
{
	std::string s_artist = s.getArtist();
	std::string s_title  = s.getTitle();
	// If artist or title is empty, use filename. This should give reasonable
	// results for google search based lyrics fetchers.
	if (s_artist.empty() || s_title.empty())
	{
		s_artist.clear();
		s_title = s.getName();
		// Get rid of underscores to improve search results.
		std::replace_if(s_title.begin(), s_title.end(), boost::is_any_of("-_"), ' ');
		size_t dot = s_title.rfind('.');
		if (dot != std::string::npos)
			s_title.resize(dot);
	}

	auto fetch_lyrics = [&](auto &fetcher_) {
		{
			if (shared_buffer)
			{
				auto buf = shared_buffer->acquire();
				*buf << "Fetching lyrics from "
				     << NC::Format::Bold
				     << fetcher_->name()
				     << NC::Format::NoBold << "... ";
			}
		}
		auto result_ = fetcher_->fetch(s_artist, s_title, s);
		if (result_.first == false)
		{
			if (shared_buffer)
			{
				auto buf = shared_buffer->acquire();
				*buf << NC::Color::Red
				     << result_.second
				     << NC::Color::End
				     << '\n';
			}
		}
		return result_;
	};

	LyricsFetcher::Result fetcher_result;
	if (current_fetcher == nullptr)
	{
		for (auto &fetcher : Config.lyrics_fetchers)
		{
			if (download_stopper && download_stopper->load())
				return boost::none;
			fetcher_result = fetch_lyrics(fetcher);
			if (fetcher_result.first)
				break;
		}
	}
	else
		fetcher_result = fetch_lyrics(current_fetcher);

	boost::optional<std::string> result;
	if (fetcher_result.first)
		result = std::move(fetcher_result.second);
	return result;
}

}

Lyrics::Lyrics()
	: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border()))
	, m_refresh_window(false)
	, m_scroll_begin(0)
	, m_fetcher(nullptr)
{ }

void Lyrics::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

void Lyrics::update()
{
	if (m_worker.valid())
	{
		auto buffer = m_shared_buffer->acquire();
		if (!buffer->empty())
		{
			w << *buffer;
			buffer->clear();
			m_refresh_window = true;
		}

		if (m_worker.is_ready())
		{
			auto lyrics = m_worker.get();
			if (lyrics)
			{
				w.clear();
				w << Charset::utf8ToLocale(*lyrics);
				std::string filename = lyricsFilename(m_song);
				if (!saveLyrics(filename, *lyrics))
					Statusbar::printf("Couldn't save lyrics as \"%1%\": %2%",
					                  filename, strerror(errno));
			}
			else
				w << "\nLyrics were not found.\n";
			clearWorker();
			m_refresh_window = true;
		}
	}

	if (m_refresh_window)
	{
		m_refresh_window = false;
		w.flush();
		w.refresh();
	}
}

void Lyrics::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
	{
		SwitchTo::execute(this);
		m_scroll_begin = 0;
		drawHeader();
	}
	else
		switchToPreviousScreen();
}

std::wstring Lyrics::title()
{
	std::wstring result = L"Lyrics";
	if (!m_song.empty())
	{
		result += L": ";
		result += Scroller(
			Format::stringify<wchar_t>(Format::parse(L"{%a - %t}|{%f}"), &m_song),
			m_scroll_begin,
			COLS - result.length() - (Config.design == Design::Alternative
			                          ? 2
			                          : Global::VolumeState.length()));
	}
	return result;
}

void Lyrics::fetch(const MPD::Song &s)
{
	if (!m_worker.valid() || s != m_song)
	{
		stopDownload();
		w.clear();
		w.reset();
		m_song = s;
		if (loadLyrics(w, lyricsFilename(m_song)))
		{
			clearWorker();
			m_refresh_window = true;
		}
		else
		{
			m_download_stopper = std::make_shared<std::atomic<bool>>(false);
			m_shared_buffer = std::make_shared<Shared<NC::Buffer>>();
			m_worker = boost::async(
				boost::launch::async,
				std::bind(downloadLyrics,
				          m_song, m_shared_buffer, m_download_stopper, m_fetcher));
		}
	}
}

void Lyrics::refetchCurrent()
{
	std::string filename = lyricsFilename(m_song);
	if (std::remove(filename.c_str()) == -1 && errno != ENOENT)
	{
		const char msg[] = "Couldn't remove \"%1%\": %2%";
		Statusbar::printf(msg, wideShorten(filename, COLS - const_strlen(msg) - 25),
		                  strerror(errno));
	}
	else
	{
		clearWorker();
		fetch(m_song);
	}
}

void Lyrics::edit()
{
	if (Config.external_editor.empty())
	{
		Statusbar::print("external_editor variable has to be set in configuration file");
		return;
	}

	Statusbar::print("Opening lyrics in external editor...");

	std::string filename = lyricsFilename(m_song);
	escapeSingleQuotes(filename);
	if (Config.use_console_editor)
	{
		runExternalConsoleCommand(Config.external_editor + " '" + filename + "'");
		fetch(m_song);
	}
	else
		runExternalCommand(Config.external_editor + " '" + filename + "'", false);
}

void Lyrics::toggleFetcher()
{
	if (m_fetcher != nullptr)
	{
		auto fetcher = std::find_if(Config.lyrics_fetchers.begin(),
		                            Config.lyrics_fetchers.end(),
		                            [this](auto &f) { return f.get() == m_fetcher; });
		assert(fetcher != Config.lyrics_fetchers.end());
		++fetcher;
		if (fetcher != Config.lyrics_fetchers.end())
			m_fetcher = fetcher->get();
		else
			m_fetcher = nullptr;
	}
	else
	{
		assert(!Config.lyrics_fetchers.empty());
		m_fetcher = Config.lyrics_fetchers[0].get();
	}

	if (m_fetcher != nullptr)
		Statusbar::printf("Using lyrics fetcher: %s", m_fetcher->name());
	else
		Statusbar::print("Using all lyrics fetchers");
}

/* For HTTP(S) streams, fetchInBackground makes ncmpcpp crash: the lyrics_file
 * gets set to the stream URL, which is too long, hence
 * boost::filesystem::exists() fails.
 * Possible solutions:
 * - truncate the URL and use that as a filename. Problem: resulting filename
 *   might not be unique.
 * - generate filenames in a different way for streams. Problem: what is a good
 *   method for this? Perhaps hashing -- but then the lyrics filenames are not
 *   informative.
 * - skip fetching lyrics for streams with URLs that are too long. Problem:
 *   this leads to inconsistent behavior since lyrics will be fetched for some
 *   streams but not others.
 * - avoid fetching lyrics for streams altogether.
 *
 * We choose the last solution, and ignore streams when fetching lyrics. This
 * is because fetching lyrics for a stream may not be accurate (streams do not
 * always provide the necessary metadata to look up lyrics reliably).
 * Furthermore, fetching lyrics for streams is not necessarily desirable.
 */
void Lyrics::fetchInBackground(const MPD::Song &s, bool notify_)
{
	auto consumer_impl = [this] {
		std::string lyrics_file;
		while (true)
		{
			ConsumerState::Song cs;
			{
				auto consumer = m_consumer_state.acquire();
				assert(consumer->running);
				if (consumer->songs.empty())
				{
					consumer->running = false;
					break;
				}
				lyrics_file = lyricsFilename(consumer->songs.front().song());

				// For long filenames (e.g. http(s) stream URLs), boost::filesystem::exists() fails.
				// This if condition is fine, because evaluation order is guaranteed.
				if (!consumer->songs.front().song().isStream() && !boost::filesystem::exists(lyrics_file))
				{
					cs = consumer->songs.front();
					if (cs.notify())
					{
						consumer->message = "Fetching lyrics for \""
							+ Format::stringify<char>(Config.song_status_format, &cs.song())
							+ "\"...";
					}
				}
				consumer->songs.pop();
			}
			if (!cs.song().empty() && !cs.song().isStream())
			{
				auto lyrics = downloadLyrics(cs.song(), nullptr, nullptr, m_fetcher);
				if (lyrics)
					saveLyrics(lyrics_file, *lyrics);
			}
		}
	};

	auto consumer = m_consumer_state.acquire();
	consumer->songs.emplace(s, notify_);
	// Start the consumer if it's not running.
	if (!consumer->running)
	{
		std::thread t(consumer_impl);
		t.detach();
		consumer->running = true;
	}
}

boost::optional<std::string> Lyrics::tryTakeConsumerMessage()
{
	boost::optional<std::string> result;
	auto consumer = m_consumer_state.acquire();
	if (consumer->message)
	{
		result = std::move(consumer->message);
		consumer->message = boost::none;
	}
	return result;
}

void Lyrics::clearWorker()
{
	m_shared_buffer.reset();
	m_worker = boost::BOOST_THREAD_FUTURE<boost::optional<std::string>>();
}

void Lyrics::stopDownload()
{
	if (m_download_stopper)
		m_download_stopper->store(true);
}
