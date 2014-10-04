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

#include <cassert>
#include <cerrno>
#include <cstring>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/locale/conversion.hpp>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <iostream>
#include <readline/readline.h>

#include "actions.h"
#include "charset.h"
#include "config.h"
#include "display.h"
#include "global.h"
#include "mpdpp.h"
#include "helpers.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "utility/conversion.h"

#include "bindings.h"
#include "browser.h"
#include "clock.h"
#include "help.h"
#include "media_library.h"
#include "lastfm.h"
#include "lyrics.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "sort_playlist.h"
#include "search_engine.h"
#include "sel_items_adder.h"
#include "server_info.h"
#include "song_info.h"
#include "outputs.h"
#include "utility/string.h"
#include "utility/type_conversions.h"
#include "tag_editor.h"
#include "tiny_tag_editor.h"
#include "visualizer.h"
#include "title.h"
#include "tags.h"

#ifdef HAVE_TAGLIB_H
# include "fileref.h"
# include "tag.h"
#endif // HAVE_TAGLIB_H

using Global::myScreen;

namespace {//

enum class Find { Forward, Backward };

boost::array<
	Actions::BaseAction *, static_cast<size_t>(Actions::Type::_numberOfActions)
> AvailableActions;

void populateActions();

void seek();
void findItem(const Find direction);
void listsChangeFinisher();

}

namespace Actions {//

bool OriginalStatusbarVisibility;
bool ExitMainLoop = false;

size_t HeaderHeight;
size_t FooterHeight;
size_t FooterStartY;

void validateScreenSize()
{
	using Global::MainHeight;
	
	if (COLS < 30 || MainHeight < 5)
	{
		NC::destroyScreen();
		std::cout << "Screen is too small to handle ncmpcpp correctly\n";
		exit(1);
	}
}

void initializeScreens()
{
	myHelp = new Help;
	myPlaylist = new Playlist;
	myBrowser = new Browser;
	mySearcher = new SearchEngine;
	myLibrary = new MediaLibrary;
	myPlaylistEditor = new PlaylistEditor;
	myLyrics = new Lyrics;
	mySelectedItemsAdder = new SelectedItemsAdder;
	mySongInfo = new SongInfo;
	myServerInfo = new ServerInfo;
	mySortPlaylistDialog = new SortPlaylistDialog;
	
#	ifdef HAVE_CURL_CURL_H
	myLastfm = new Lastfm;
#	endif // HAVE_CURL_CURL_H
	
#	ifdef HAVE_TAGLIB_H
	myTinyTagEditor = new TinyTagEditor;
	myTagEditor = new TagEditor;
#	endif // HAVE_TAGLIB_H
	
#	ifdef ENABLE_VISUALIZER
	myVisualizer = new Visualizer;
#	endif // ENABLE_VISUALIZER
	
#	ifdef ENABLE_OUTPUTS
	myOutputs = new Outputs;
#	endif // ENABLE_OUTPUTS
	
#	ifdef ENABLE_CLOCK
	myClock = new Clock;
#	endif // ENABLE_CLOCK
	
}

void setResizeFlags()
{
	myHelp->hasToBeResized = 1;
	myPlaylist->hasToBeResized = 1;
	myBrowser->hasToBeResized = 1;
	mySearcher->hasToBeResized = 1;
	myLibrary->hasToBeResized = 1;
	myPlaylistEditor->hasToBeResized = 1;
	myLyrics->hasToBeResized = 1;
	mySelectedItemsAdder->hasToBeResized = 1;
	mySongInfo->hasToBeResized = 1;
	myServerInfo->hasToBeResized = 1;
	mySortPlaylistDialog->hasToBeResized = 1;
	
#	ifdef HAVE_CURL_CURL_H
	myLastfm->hasToBeResized = 1;
#	endif // HAVE_CURL_CURL_H
	
#	ifdef HAVE_TAGLIB_H
	myTinyTagEditor->hasToBeResized = 1;
	myTagEditor->hasToBeResized = 1;
#	endif // HAVE_TAGLIB_H
	
#	ifdef ENABLE_VISUALIZER
	myVisualizer->hasToBeResized = 1;
#	endif // ENABLE_VISUALIZER
	
#	ifdef ENABLE_OUTPUTS
	myOutputs->hasToBeResized = 1;
#	endif // ENABLE_OUTPUTS
	
#	ifdef ENABLE_CLOCK
	myClock->hasToBeResized = 1;
#	endif // ENABLE_CLOCK
}

void resizeScreen(bool reload_main_window)
{
	using Global::MainHeight;
	using Global::wHeader;
	using Global::wFooter;
	
#	if defined(USE_PDCURSES)
	resize_term(0, 0);
#	else
	// update internal screen dimensions
	if (reload_main_window)
	{
		rl_resize_terminal();
		endwin();
		refresh();
	}
#	endif
	
	MainHeight = LINES-(Config.design == Design::Alternative ? 7 : 4);
	
	validateScreenSize();
	
	if (!Config.header_visibility)
		MainHeight += 2;
	if (!Config.statusbar_visibility)
		++MainHeight;
	
	setResizeFlags();
	
	applyToVisibleWindows(&BaseScreen::resize);
	
	if (Config.header_visibility || Config.design == Design::Alternative)
		wHeader->resize(COLS, HeaderHeight);
	
	FooterStartY = LINES-(Config.statusbar_visibility ? 2 : 1);
	wFooter->moveTo(0, FooterStartY);
	wFooter->resize(COLS, Config.statusbar_visibility ? 2 : 1);
	
	applyToVisibleWindows(&BaseScreen::refresh);
	
	Status::Changes::elapsedTime(false);
	Status::Changes::playerState();
	// Note: routines for drawing separator if alternative user
	// interface is active and header is hidden are placed in
	// NcmpcppStatusChanges.StatusFlags
	Status::Changes::flags();
	drawHeader();
	wFooter->refresh();
	refresh();
}

void setWindowsDimensions()
{
	using Global::MainStartY;
	using Global::MainHeight;
	
	MainStartY = Config.design == Design::Alternative ? 5 : 2;
	MainHeight = LINES-(Config.design == Design::Alternative ? 7 : 4);
	
	if (!Config.header_visibility)
	{
		MainStartY -= 2;
		MainHeight += 2;
	}
	if (!Config.statusbar_visibility)
		++MainHeight;
	
	HeaderHeight = Config.design == Design::Alternative ? (Config.header_visibility ? 5 : 3) : 1;
	FooterStartY = LINES-(Config.statusbar_visibility ? 2 : 1);
	FooterHeight = Config.statusbar_visibility ? 2 : 1;
}

bool askYesNoQuestion(const boost::format &fmt, void (*callback)())
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << fmt.str() << " [" << NC::Format::Bold << 'y' << NC::Format::NoBold << '/' << NC::Format::Bold << 'n' << NC::Format::NoBold << "]";
	wFooter->refresh();
	int answer = 0;
	do
	{
		if (callback)
			callback();
		answer = wFooter->readKey();
	}
	while (answer != 'y' && answer != 'n');
	Statusbar::unlock();
	return answer == 'y';
}

bool isMPDMusicDirSet()
{
	if (Config.mpd_music_dir.empty())
	{
		Statusbar::print("Proper mpd_music_dir variable has to be set in configuration file");
		return false;
	}
	return true;
}

BaseAction &get(Actions::Type at)
{
	if (AvailableActions[1] == nullptr)
		populateActions();
	BaseAction *action = AvailableActions[static_cast<size_t>(at)];
	// action should be always present if action type in queried
	assert(action != nullptr);
	return *action;
}

BaseAction *get(const std::string &name)
{
	BaseAction *result = 0;
	if (AvailableActions[1] == nullptr)
		populateActions();
	for (auto it = AvailableActions.begin(); it != AvailableActions.end(); ++it)
	{
		if (*it != nullptr && (*it)->name() == name)
		{
			result = *it;
			break;
		}
	}
	return result;
}

bool MouseEvent::canBeRun() const
{
	return Config.mouse_support;
}

void MouseEvent::run()
{
	using Global::VolumeState;
	
	m_old_mouse_event = m_mouse_event;
	getmouse(&m_mouse_event);
	// workaround shitty ncurses behavior introduced in >=5.8, when we mysteriously get
	// a few times after ncmpcpp startup 2^27 code instead of BUTTON{1,3}_RELEASED. since that
	// 2^27 thing shows constantly instead of BUTTON2_PRESSED, it was redefined to be recognized
	// as BUTTON2_PRESSED. but clearly we don't want to trigger behavior bound to BUTTON2
	// after BUTTON{1,3} was pressed. so, here is the workaround: if last event was BUTTON{1,3}_PRESSED,
	// we MUST get BUTTON{1,3}_RELEASED afterwards. if we get BUTTON2_PRESSED, erroneus behavior
	// is about to occur and we need to prevent that.
	if (m_old_mouse_event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED) && m_mouse_event.bstate & BUTTON2_PRESSED)
		return;
	if (m_mouse_event.bstate & BUTTON1_PRESSED
	&&  m_mouse_event.y == LINES-(Config.statusbar_visibility ? 2 : 1)
	   ) // progressbar
	{
		if (Status::State::player() == MPD::psStop)
			return;
		Mpd.Seek(Status::State::currentSongPosition(),
			Status::State::totalTime()*m_mouse_event.x/double(COLS));
	}
	else if (m_mouse_event.bstate & BUTTON1_PRESSED
	     &&  (Config.statusbar_visibility || Config.design == Design::Alternative)
	     &&  Status::State::player() != MPD::psStop
	     &&  m_mouse_event.y == (Config.design == Design::Alternative ? 1 : LINES-1)
			 &&  m_mouse_event.x < 9
		) // playing/paused
	{
		Mpd.Toggle();
	}
	else if ((m_mouse_event.bstate & BUTTON2_PRESSED || m_mouse_event.bstate & BUTTON4_PRESSED)
	     &&	 (Config.header_visibility || Config.design == Design::Alternative)
	     &&	 m_mouse_event.y == 0 && size_t(m_mouse_event.x) > COLS-VolumeState.length()
	) // volume
	{
		if (m_mouse_event.bstate & BUTTON2_PRESSED)
			get(Type::VolumeDown).execute();
		else
			get(Type::VolumeUp).execute();
	}
	else if (m_mouse_event.bstate & (BUTTON1_PRESSED | BUTTON2_PRESSED | BUTTON3_PRESSED | BUTTON4_PRESSED))
		myScreen->mouseButtonPressed(m_mouse_event);
}

void ScrollUp::run()
{
	myScreen->scroll(NC::Scroll::Up);
	listsChangeFinisher();
}

void ScrollDown::run()
{
	myScreen->scroll(NC::Scroll::Down);
	listsChangeFinisher();
}

bool ScrollUpArtist::canBeRun() const
{
	return proxySongList(myScreen);
}

void ScrollUpArtist::run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	if (pl.empty())
		return;
	size_t pos = pl.choice();
	if (MPD::Song *s = pl.getSong(pos))
	{
		std::string artist = s->getArtist();
		while (pos > 0)
		{
			s = pl.getSong(--pos);
			if (!s || s->getArtist() != artist)
				break;
		}
		pl.highlight(pos);
	}
}

bool ScrollUpAlbum::canBeRun() const
{
	return proxySongList(myScreen);
}

void ScrollUpAlbum::run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	if (pl.empty())
		return;
	size_t pos = pl.choice();
	if (MPD::Song *s = pl.getSong(pos))
	{
		std::string album = s->getAlbum();
		while (pos > 0)
		{
			s = pl.getSong(--pos);
			if (!s || s->getAlbum() != album)
				break;
		}
		pl.highlight(pos);
	}
}

bool ScrollDownArtist::canBeRun() const
{
	return proxySongList(myScreen);
}

void ScrollDownArtist::run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	if (pl.empty())
		return;
	size_t pos = pl.choice();
	if (MPD::Song *s = pl.getSong(pos))
	{
		std::string artist = s->getArtist();
		while (pos < pl.size() - 1)
		{
			s = pl.getSong(++pos);
			if (!s || s->getArtist() != artist)
				break;
		}
		pl.highlight(pos);
	}
}

bool ScrollDownAlbum::canBeRun() const
{
	return proxySongList(myScreen);
}

void ScrollDownAlbum::run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	if (pl.empty())
		return;
	size_t pos = pl.choice();
	if (MPD::Song *s = pl.getSong(pos))
	{
		std::string album = s->getAlbum();
		while (pos < pl.size() - 1)
		{
			s = pl.getSong(++pos);
			if (!s || s->getAlbum() != album)
				break;
		}
		pl.highlight(pos);
	}
}

void PageUp::run()
{
	myScreen->scroll(NC::Scroll::PageUp);
	listsChangeFinisher();
}

void PageDown::run()
{
	myScreen->scroll(NC::Scroll::PageDown);
	listsChangeFinisher();
}

void MoveHome::run()
{
	myScreen->scroll(NC::Scroll::Home);
	listsChangeFinisher();
}

void MoveEnd::run()
{
	myScreen->scroll(NC::Scroll::End);
	listsChangeFinisher();
}

void ToggleInterface::run()
{
	switch (Config.design)
	{
		case Design::Classic:
			Config.design = Design::Alternative;
			Config.statusbar_visibility = false;
			break;
		case Design::Alternative:
			Config.design = Design::Classic;
			Config.statusbar_visibility = OriginalStatusbarVisibility;
			break;
	}
	setWindowsDimensions();
	Progressbar::unlock();
	Statusbar::unlock();
	resizeScreen(false);
	Status::Changes::mixer();
	Status::Changes::elapsedTime(false);
	Statusbar::printf("User interface: %1%", Config.design);
}

bool JumpToParentDirectory::canBeRun() const
{
	return (myScreen == myBrowser)
#	ifdef HAVE_TAGLIB_H
	    || (myScreen->activeWindow() == myTagEditor->Dirs)
#	endif // HAVE_TAGLIB_H
	;
}

void JumpToParentDirectory::run()
{
	if (myScreen == myBrowser)
	{
		if (myBrowser->CurrentDir() != "/")
		{
			myBrowser->main().reset();
			myBrowser->enterPressed();
		}
	}
#	ifdef HAVE_TAGLIB_H
	else if (myScreen == myTagEditor)
	{
		if (myTagEditor->CurrentDir() != "/")
		{
			myTagEditor->Dirs->reset();
			myTagEditor->enterPressed();
		}
	}
#	endif // HAVE_TAGLIB_H
}

void PressEnter::run()
{
	myScreen->enterPressed();
}

void PressSpace::run()
{
	myScreen->spacePressed();
}

bool PreviousColumn::canBeRun() const
{
	auto hc = hasColumns(myScreen);
	return hc && hc->previousColumnAvailable();
}

void PreviousColumn::run()
{
	hasColumns(myScreen)->previousColumn();
}

bool NextColumn::canBeRun() const
{
	auto hc = hasColumns(myScreen);
	return hc && hc->nextColumnAvailable();
}

void NextColumn::run()
{
	hasColumns(myScreen)->nextColumn();
}

bool MasterScreen::canBeRun() const
{
	using Global::myLockedScreen;
	using Global::myInactiveScreen;
	
	return myLockedScreen
	    && myInactiveScreen
	    && myLockedScreen != myScreen
	    && myScreen->isMergable();
}

void MasterScreen::run()
{
	using Global::myInactiveScreen;
	using Global::myLockedScreen;
	
	myInactiveScreen = myScreen;
	myScreen = myLockedScreen;
	drawHeader();
}

bool SlaveScreen::canBeRun() const
{
	using Global::myLockedScreen;
	using Global::myInactiveScreen;
	
	return myLockedScreen
	    && myInactiveScreen
	    && myLockedScreen == myScreen
	    && myScreen->isMergable();
}

void SlaveScreen::run()
{
	using Global::myInactiveScreen;
	using Global::myLockedScreen;
	
	myScreen = myInactiveScreen;
	myInactiveScreen = myLockedScreen;
	drawHeader();
}

void VolumeUp::run()
{
	int volume = std::min(Status::State::volume()+Config.volume_change_step, 100u);
	Mpd.SetVolume(volume);
}

void VolumeDown::run()
{
	int volume = std::max(int(Status::State::volume()-Config.volume_change_step), 0);
	Mpd.SetVolume(volume);
}

bool DeletePlaylistItems::canBeRun() const
{
	return (myScreen == myPlaylist && !myPlaylist->main().empty())
	    || (myScreen->isActiveWindow(myPlaylistEditor->Content) && !myPlaylistEditor->Content.empty());
}

void DeletePlaylistItems::run()
{
	if (myScreen == myPlaylist)
	{
		Statusbar::print("Deleting items...");
		auto delete_fun = boost::bind(&MPD::Connection::Delete, _1, _2);
		deleteSelectedSongs(myPlaylist->main(), delete_fun);
		Statusbar::print("Item(s) deleted");
	}
	else if (myScreen->isActiveWindow(myPlaylistEditor->Content))
	{
		std::string playlist = myPlaylistEditor->Playlists.current().value();
		auto delete_fun = boost::bind(&MPD::Connection::PlaylistDelete, _1, playlist, _2);
		Statusbar::print("Deleting items...");
		deleteSelectedSongs(myPlaylistEditor->Content, delete_fun);
		Statusbar::print("Item(s) deleted");
	}
}

bool DeleteBrowserItems::canBeRun() const
{
	auto check_if_deletion_allowed = []() {
		if (Config.allow_for_physical_item_deletion)
			return true;
		else
		{
			Statusbar::print("Flag \"allow_for_physical_item_deletion\" needs to be enabled in configuration file");
			return false;
		}
	};
	return myScreen == myBrowser
	    && !myBrowser->main().empty()
	    && isMPDMusicDirSet()
	    && check_if_deletion_allowed();
}

void DeleteBrowserItems::run()
{
	boost::format question;
	if (hasSelected(myBrowser->main().begin(), myBrowser->main().end()))
		question = boost::format("Delete selected items?");
	else
	{
		MPD::Item &item = myBrowser->main().current().value();
		std::string iname = item.type == MPD::itSong ? item.song->getName() : item.name;
		question = boost::format("Delete %1% \"%2%\"?")
			% itemTypeToString(item.type) % wideShorten(iname, COLS-question.size()-10);
	}
	bool yes = askYesNoQuestion(question, Status::trace);
	if (yes)
	{
		bool success = true;
		auto list = getSelectedOrCurrent(myBrowser->main().begin(), myBrowser->main().end(), myBrowser->main().currentI());
		for (auto it = list.begin(); it != list.end(); ++it)
		{
			const MPD::Item &i = (*it)->value();
			std::string iname = i.type == MPD::itSong ? i.song->getName() : i.name;
			std::string errmsg;
			if (myBrowser->deleteItem(i, errmsg))
			{
				const char msg[] = "\"%1%\" deleted";
				Statusbar::printf(msg, wideShorten(iname, COLS-const_strlen(msg)));
			}
			else
			{
				Statusbar::print(errmsg);
				success = false;
				break;
			}
		}
		if (success)
		{
			if (myBrowser->isLocal())
				myBrowser->GetDirectory(myBrowser->CurrentDir());
			else
				Mpd.UpdateDirectory(myBrowser->CurrentDir());
		}
	}
	else
		Statusbar::print("Aborted");
}

bool DeleteStoredPlaylist::canBeRun() const
{
	return myScreen->isActiveWindow(myPlaylistEditor->Playlists);
}

void DeleteStoredPlaylist::run()
{
	if (myPlaylistEditor->Playlists.empty())
		return;
	boost::format question;
	if (hasSelected(myPlaylistEditor->Playlists.begin(), myPlaylistEditor->Playlists.end()))
		question = boost::format("Delete selected playlists?");
	else
		question = boost::format("Delete playlist \"%1%\"?")
			% wideShorten(myPlaylistEditor->Playlists.current().value(), COLS-question.size()-10);
	bool yes = askYesNoQuestion(question, Status::trace);
	if (yes)
	{
		auto list = getSelectedOrCurrent(myPlaylistEditor->Playlists.begin(), myPlaylistEditor->Playlists.end(), myPlaylistEditor->Playlists.currentI());
		for (auto it = list.begin(); it != list.end(); ++it)
			Mpd.DeletePlaylist((*it)->value());
		Statusbar::printf("%1% deleted", list.size() == 1 ? "Playlist" : "Playlists");
	}
	else
		Statusbar::print("Aborted");
}

void ReplaySong::run()
{
	if (Status::State::player() != MPD::psStop)
		Mpd.Seek(Status::State::currentSongPosition(), 0);
}

void PreviousSong::run()
{
	Mpd.Prev();
}

void NextSong::run()
{
	Mpd.Next();
}

void Pause::run()
{
	Mpd.Toggle();
}

void SavePlaylist::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Save playlist as: ";
	std::string playlist_name = wFooter->getString();
	Statusbar::unlock();
	if (playlist_name.find("/") != std::string::npos)
	{
		Statusbar::print("Playlist name must not contain slashes");
		return;
	}
	if (!playlist_name.empty())
	{
		if (myPlaylist->main().isFiltered())
		{
			Mpd.StartCommandsList();
			for (size_t i = 0; i < myPlaylist->main().size(); ++i)
				Mpd.AddToPlaylist(playlist_name, myPlaylist->main()[i].value());
			Mpd.CommitCommandsList();
			Statusbar::printf("Filtered items added to playlist \"%1%\"", playlist_name);
		}
		else
		{
			try
			{
				Mpd.SavePlaylist(playlist_name);
				Statusbar::printf("Playlist saved as \"%1%\"", playlist_name);
			}
			catch (MPD::ServerError &e)
			{
				if (e.code() == MPD_SERVER_ERROR_EXIST)
				{
					bool yes = askYesNoQuestion(
						boost::format("Playlist \"%1%\" already exists, overwrite?") % playlist_name,
						Status::trace
					);
					if (yes)
					{
						Mpd.DeletePlaylist(playlist_name);
						Mpd.SavePlaylist(playlist_name);
						Statusbar::print("Playlist overwritten");
					}
					else
						Statusbar::print("Aborted");
					if (myScreen == myPlaylist)
						myPlaylist->EnableHighlighting();
				}
				else
					throw e;
			}
		}
	}
	if (!myBrowser->isLocal()
	&&  myBrowser->CurrentDir() == "/"
	&&  !myBrowser->main().empty())
		myBrowser->GetDirectory(myBrowser->CurrentDir());
}

void Stop::run()
{
	Mpd.Stop();
}

void ExecuteCommand::run()
{
	using Global::wFooter;
	Statusbar::lock();
	Statusbar::put() << NC::Format::Bold << ":" << NC::Format::NoBold;
	wFooter->setGetStringHelper(Statusbar::Helpers::TryExecuteImmediateCommand());
	std::string cmd_name = wFooter->getString();
	wFooter->setGetStringHelper(Statusbar::Helpers::getString);
	Statusbar::unlock();
	if (cmd_name.empty())
		return;
	auto cmd = Bindings.findCommand(cmd_name);
	if (cmd)
	{
		Statusbar::printf(1, "Executing %1%...", cmd_name);
		bool res = cmd->binding().execute();
		Statusbar::printf("Execution of command \"%1%\" %2%.",
			cmd_name, res ? "successful" : "unsuccessful"
		);
	}
	else
		Statusbar::printf("No command named \"%1%\"", cmd_name);
}

bool MoveSortOrderUp::canBeRun() const
{
	return myScreen == mySortPlaylistDialog;
}

void MoveSortOrderUp::run()
{
	mySortPlaylistDialog->moveSortOrderUp();
}

bool MoveSortOrderDown::canBeRun() const
{
	return myScreen == mySortPlaylistDialog;
}

void MoveSortOrderDown::run()
{
	mySortPlaylistDialog->moveSortOrderDown();
}

bool MoveSelectedItemsUp::canBeRun() const
{
	return ((myScreen == myPlaylist
	    &&  !myPlaylist->main().empty()
	    &&  !myPlaylist->isFiltered())
	 ||    (myScreen->isActiveWindow(myPlaylistEditor->Content)
	    &&  !myPlaylistEditor->Content.empty()
	    &&  !myPlaylistEditor->isContentFiltered()));
}

void MoveSelectedItemsUp::run()
{
	if (myScreen == myPlaylist)
	{
		moveSelectedItemsUp(myPlaylist->main(), boost::bind(&MPD::Connection::Move, _1, _2, _3));
	}
	else if (myScreen == myPlaylistEditor)
	{
		assert(!myPlaylistEditor->Playlists.empty());
		std::string playlist = myPlaylistEditor->Playlists.current().value();
		auto move_fun = boost::bind(&MPD::Connection::PlaylistMove, _1, playlist, _2, _3);
		moveSelectedItemsUp(myPlaylistEditor->Content, move_fun);
	}
}

bool MoveSelectedItemsDown::canBeRun() const
{
	return ((myScreen == myPlaylist
	    &&  !myPlaylist->main().empty()
	    &&  !myPlaylist->isFiltered())
	 ||    (myScreen->isActiveWindow(myPlaylistEditor->Content)
	    &&  !myPlaylistEditor->Content.empty()
	    &&  !myPlaylistEditor->isContentFiltered()));
}

void MoveSelectedItemsDown::run()
{
	if (myScreen == myPlaylist)
	{
		moveSelectedItemsDown(myPlaylist->main(), boost::bind(&MPD::Connection::Move, _1, _2, _3));
	}
	else if (myScreen == myPlaylistEditor)
	{
		assert(!myPlaylistEditor->Playlists.empty());
		std::string playlist = myPlaylistEditor->Playlists.current().value();
		auto move_fun = boost::bind(&MPD::Connection::PlaylistMove, _1, playlist, _2, _3);
		moveSelectedItemsDown(myPlaylistEditor->Content, move_fun);
	}
}

bool MoveSelectedItemsTo::canBeRun() const
{
	return myScreen == myPlaylist
	    || myScreen->isActiveWindow(myPlaylistEditor->Content);
}

void MoveSelectedItemsTo::run()
{
	if (myScreen == myPlaylist)
	{
		if (!myPlaylist->main().empty())
			moveSelectedItemsTo(myPlaylist->main(), boost::bind(&MPD::Connection::Move, _1, _2, _3));
	}
	else
	{
		assert(!myPlaylistEditor->Playlists.empty());
		std::string playlist = myPlaylistEditor->Playlists.current().value();
		auto move_fun = boost::bind(&MPD::Connection::PlaylistMove, _1, playlist, _2, _3);
		moveSelectedItemsTo(myPlaylistEditor->Content, move_fun);
	}
}

bool Add::canBeRun() const
{
	return myScreen != myPlaylistEditor
	   || !myPlaylistEditor->Playlists.empty();
}

void Add::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << (myScreen == myPlaylistEditor ? "Add to playlist: " : "Add: ");
	std::string path = wFooter->getString();
	Statusbar::unlock();
	if (!path.empty())
	{
		Statusbar::put() << "Adding...";
		wFooter->refresh();
		if (myScreen == myPlaylistEditor)
			Mpd.AddToPlaylist(myPlaylistEditor->Playlists.current().value(), path);
		else
		{
			const char lastfm_url[] = "lastfm://";
			if (path.compare(0, const_strlen(lastfm_url), lastfm_url) == 0
			||  path.find(".asx", path.length()-4) != std::string::npos
			||  path.find(".cue", path.length()-4) != std::string::npos
			||  path.find(".m3u", path.length()-4) != std::string::npos
			||  path.find(".pls", path.length()-4) != std::string::npos
			||  path.find(".xspf", path.length()-5) != std::string::npos)
				Mpd.LoadPlaylist(path);
			else
				Mpd.Add(path);
		}
	}
}

bool SeekForward::canBeRun() const
{
	return Status::State::player() != MPD::psStop && Status::State::totalTime() > 0;
}

void SeekForward::run()
{
	seek();
}

bool SeekBackward::canBeRun() const
{
	return Status::State::player() != MPD::psStop && Status::State::totalTime() > 0;
}

void SeekBackward::run()
{
	seek();
}

bool ToggleDisplayMode::canBeRun() const
{
	return myScreen == myPlaylist
	    || myScreen == myBrowser
	    || myScreen == mySearcher
	    || myScreen->isActiveWindow(myPlaylistEditor->Content);
}

void ToggleDisplayMode::run()
{
	if (myScreen == myPlaylist)
	{
		switch (Config.playlist_display_mode)
		{
			case DisplayMode::Classic:
				Config.playlist_display_mode = DisplayMode::Columns;
				myPlaylist->main().setItemDisplayer(boost::bind(
					Display::SongsInColumns, _1, myPlaylist->proxySongList()
				));
				if (Config.titles_visibility)
					myPlaylist->main().setTitle(Display::Columns(myPlaylist->main().getWidth()));
				else
					myPlaylist->main().setTitle("");
				break;
			case DisplayMode::Columns:
				Config.playlist_display_mode = DisplayMode::Classic;
				myPlaylist->main().setItemDisplayer(boost::bind(
					Display::Songs, _1, myPlaylist->proxySongList(), Config.song_list_format
				));
				myPlaylist->main().setTitle("");
		}
		Statusbar::printf("Playlist display mode: %1%", Config.playlist_display_mode);
	}
	else if (myScreen == myBrowser)
	{
		switch (Config.browser_display_mode)
		{
			case DisplayMode::Classic:
				Config.browser_display_mode = DisplayMode::Columns;
				if (Config.titles_visibility)
					myBrowser->main().setTitle(Display::Columns(myBrowser->main().getWidth()));
				else
					myBrowser->main().setTitle("");
				break;
			case DisplayMode::Columns:
				Config.browser_display_mode = DisplayMode::Classic;
				myBrowser->main().setTitle("");
				break;
		}
		Statusbar::printf("Browser display mode: %1%", Config.browser_display_mode);
	}
	else if (myScreen == mySearcher)
	{
		switch (Config.search_engine_display_mode)
		{
			case DisplayMode::Classic:
				Config.search_engine_display_mode = DisplayMode::Columns;
				break;
			case DisplayMode::Columns:
				Config.search_engine_display_mode = DisplayMode::Classic;
				break;
		}
		Statusbar::printf("Search engine display mode: %1%", Config.search_engine_display_mode);
		if (mySearcher->main().size() > SearchEngine::StaticOptions)
			mySearcher->main().setTitle(
				   Config.search_engine_display_mode == DisplayMode::Columns
				&& Config.titles_visibility
				? Display::Columns(mySearcher->main().getWidth())
				: ""
			);
	}
	else if (myScreen->isActiveWindow(myPlaylistEditor->Content))
	{
		switch (Config.playlist_editor_display_mode)
		{
			case DisplayMode::Classic:
				Config.playlist_editor_display_mode = DisplayMode::Columns;
				myPlaylistEditor->Content.setItemDisplayer(boost::bind(
					Display::SongsInColumns, _1, myPlaylistEditor->contentProxyList()
				));
				break;
			case DisplayMode::Columns:
				Config.playlist_editor_display_mode = DisplayMode::Classic;
				myPlaylistEditor->Content.setItemDisplayer(boost::bind(
					Display::Songs, _1, myPlaylistEditor->contentProxyList(), Config.song_list_format
				));
				break;
		}
		Statusbar::printf("Playlist editor display mode: %1%", Config.playlist_editor_display_mode);
	}
}

bool ToggleSeparatorsBetweenAlbums::canBeRun() const
{
	return true;
}

void ToggleSeparatorsBetweenAlbums::run()
{
	Config.playlist_separate_albums = !Config.playlist_separate_albums;
	Statusbar::printf("Separators between albums: %1%",
		Config.playlist_separate_albums ? "on" : "off"
	);
}

#ifndef HAVE_CURL_CURL_H
bool ToggleLyricsFetcher::canBeRun() const
{
	return false;
}
#endif // NOT HAVE_CURL_CURL_H

void ToggleLyricsFetcher::run()
{
#	ifdef HAVE_CURL_CURL_H
	myLyrics->ToggleFetcher();
#	endif // HAVE_CURL_CURL_H
}

#ifndef HAVE_CURL_CURL_H
bool ToggleFetchingLyricsInBackground::canBeRun() const
{
	return false;
}
#endif // NOT HAVE_CURL_CURL_H

void ToggleFetchingLyricsInBackground::run()
{
#	ifdef HAVE_CURL_CURL_H
	Config.fetch_lyrics_in_background = !Config.fetch_lyrics_in_background;
	Statusbar::printf("Fetching lyrics for playing songs in background: %1%",
		Config.fetch_lyrics_in_background ? "on" : "off"
	);
#	endif // HAVE_CURL_CURL_H
}

void TogglePlayingSongCentering::run()
{
	Config.autocenter_mode = !Config.autocenter_mode;
	Statusbar::printf("Centering playing song: %1%",
		Config.autocenter_mode ? "on" : "off"
	);
	if (Config.autocenter_mode
	&& Status::State::player() != MPD::psUnknown
	&& !myPlaylist->main().isFiltered())
	{
		auto sp = Status::State::currentSongPosition();
		if (sp >= 0 && size_t(sp) < myPlaylist->main().size())
			myPlaylist->main().highlight(Status::State::currentSongPosition());
	}
}

void UpdateDatabase::run()
{
	if (myScreen == myBrowser)
		Mpd.UpdateDirectory(myBrowser->CurrentDir());
#	ifdef HAVE_TAGLIB_H
	else if (myScreen == myTagEditor)
		Mpd.UpdateDirectory(myTagEditor->CurrentDir());
#	endif // HAVE_TAGLIB_H
	else
		Mpd.UpdateDirectory("/");
}

bool JumpToPlayingSong::canBeRun() const
{
	return ((myScreen == myPlaylist && !myPlaylist->isFiltered())
	    ||  myScreen == myBrowser
	    ||  myScreen == myLibrary)
	  &&   Status::State::player() != MPD::psUnknown;
}

void JumpToPlayingSong::run()
{
	if (myScreen == myPlaylist)
	{
		auto sp = Status::State::currentSongPosition();
		if (sp >= 0 && size_t(sp) < myPlaylist->main().size())
			myPlaylist->main().highlight(Status::State::currentSongPosition());
	}
	else if (myScreen == myBrowser)
	{
		myBrowser->LocateSong(myPlaylist->nowPlayingSong());
		drawHeader();
	}
	else if (myScreen == myLibrary)
	{
		myLibrary->LocateSong(myPlaylist->nowPlayingSong());
	}
}

void ToggleRepeat::run()
{
	Mpd.SetRepeat(!Status::State::repeat());
}

void Shuffle::run()
{
	Mpd.Shuffle();
}

void ToggleRandom::run()
{
	Mpd.SetRandom(!Status::State::random());
}

bool StartSearching::canBeRun() const
{
	return myScreen == mySearcher && !mySearcher->main()[0].isInactive();
}

void StartSearching::run()
{
	mySearcher->main().highlight(SearchEngine::SearchButton);
	mySearcher->main().setHighlighting(0);
	mySearcher->main().refresh();
	mySearcher->main().setHighlighting(1);
	mySearcher->enterPressed();
}

bool SaveTagChanges::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen == myTinyTagEditor
	    || myScreen->activeWindow() == myTagEditor->TagTypes;
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void SaveTagChanges::run()
{
#	ifdef HAVE_TAGLIB_H
	if (myScreen == myTinyTagEditor)
	{
		myTinyTagEditor->main().highlight(myTinyTagEditor->main().size()-2); // Save
		myTinyTagEditor->enterPressed();
	}
	else if (myScreen->activeWindow() == myTagEditor->TagTypes)
	{
		myTagEditor->TagTypes->highlight(myTagEditor->TagTypes->size()-1); // Save
		myTagEditor->enterPressed();
	}
#	endif // HAVE_TAGLIB_H
}

void ToggleSingle::run()
{
	Mpd.SetSingle(!Status::State::single());
}

void ToggleConsume::run()
{
	Mpd.SetConsume(!Status::State::consume());
}

void ToggleCrossfade::run()
{
	Mpd.SetCrossfade(Status::State::crossfade() ? 0 : Config.crossfade_time);
}

void SetCrossfade::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Set crossfade to: ";
	std::string crossfade = wFooter->getString();
	Statusbar::unlock();
	int cf = fromString<unsigned>(crossfade);
	lowerBoundCheck(cf, 1);
	Config.crossfade_time = cf;
	Mpd.SetCrossfade(cf);
}

void SetVolume::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Set volume to: ";
	std::string strvolume = wFooter->getString();
	Statusbar::unlock();
	int volume = fromString<unsigned>(strvolume);
	boundsCheck(volume, 0, 100);
	Mpd.SetVolume(volume);
	Statusbar::printf("Volume set to %1%%%", volume);
}

bool EditSong::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return currentSong(myScreen)
	    && isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditSong::run()
{
#	ifdef HAVE_TAGLIB_H
	auto s = currentSong(myScreen);
	myTinyTagEditor->SetEdited(*s);
	myTinyTagEditor->switchTo();
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryTag::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen->isActiveWindow(myLibrary->Tags)
	    && !myLibrary->Tags.empty()
	    && isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryTag::run()
{
#	ifdef HAVE_TAGLIB_H
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << NC::Format::Bold << tagTypeToString(Config.media_lib_primary_tag) << NC::Format::NoBold << ": ";
	std::string new_tag = wFooter->getString(myLibrary->Tags.current().value().tag());
	Statusbar::unlock();
	if (!new_tag.empty() && new_tag != myLibrary->Tags.current().value().tag())
	{
		Statusbar::print("Updating tags...");
		Mpd.StartSearch(1);
		Mpd.AddSearch(Config.media_lib_primary_tag, myLibrary->Tags.current().value().tag());
		MPD::MutableSong::SetFunction set = tagTypeToSetFunction(Config.media_lib_primary_tag);
		assert(set);
		bool success = true;
		std::string dir_to_update;
		Mpd.CommitSearchSongs([set, &dir_to_update, &new_tag, &success](MPD::Song s) {
			if (!success)
				return;
			MPD::MutableSong ms = s;
			ms.setTags(set, new_tag, Config.tags_separator);
			Statusbar::printf("Updating tags in \"%1%\"...", ms.getName());
			std::string path = Config.mpd_music_dir + ms.getURI();
			if (!Tags::write(ms))
			{
				const char msg[] = "Error while updating tags in \"%1%\"";
				Statusbar::printf(msg, wideShorten(ms.getURI(), COLS-const_strlen(msg)));
				success = false;
			}
			if (dir_to_update.empty())
				dir_to_update = s.getURI();
			else
				dir_to_update = getSharedDirectory(dir_to_update, s.getURI());
		});
		if (success)
		{
			Mpd.UpdateDirectory(dir_to_update);
			Statusbar::print("Tags updated successfully");
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryAlbum::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen->isActiveWindow(myLibrary->Albums)
	    && !myLibrary->Albums.empty()
		&& isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryAlbum::run()
{
#	ifdef HAVE_TAGLIB_H
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << NC::Format::Bold << "Album: " << NC::Format::NoBold;
	std::string new_album = wFooter->getString(myLibrary->Albums.current().value().entry().album());
	Statusbar::unlock();
	if (!new_album.empty() && new_album != myLibrary->Albums.current().value().entry().album())
	{
		bool success = 1;
		Statusbar::print("Updating tags...");
		for (size_t i = 0;  i < myLibrary->Songs.size(); ++i)
		{
			Statusbar::printf("Updating tags in \"%1%\"...", myLibrary->Songs[i].value().getName());
			std::string path = Config.mpd_music_dir + myLibrary->Songs[i].value().getURI();
			TagLib::FileRef f(path.c_str());
			if (f.isNull())
			{
				const char msg[] = "Error while opening file \"%1%\"";
				Statusbar::printf(msg, wideShorten(myLibrary->Songs[i].value().getURI(), COLS-const_strlen(msg)));
				success = 0;
				break;
			}
			f.tag()->setAlbum(ToWString(new_album));
			if (!f.save())
			{
				const char msg[] = "Error while writing tags in \"%1%\"";
				Statusbar::printf(msg, wideShorten(myLibrary->Songs[i].value().getURI(), COLS-const_strlen(msg)));
				success = 0;
				break;
			}
		}
		if (success)
		{
			Mpd.UpdateDirectory(getSharedDirectory(myLibrary->Songs.beginV(), myLibrary->Songs.endV()));
			Statusbar::print("Tags updated successfully");
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditDirectoryName::canBeRun() const
{
	return  ((myScreen == myBrowser
	      && !myBrowser->main().empty()
	      && myBrowser->main().current().value().type == MPD::itDirectory)
#	ifdef HAVE_TAGLIB_H
	    ||   (myScreen->activeWindow() == myTagEditor->Dirs
	      && !myTagEditor->Dirs->empty()
	      && myTagEditor->Dirs->choice() > 0)
#	endif // HAVE_TAGLIB_H
	) &&     isMPDMusicDirSet();
}

void EditDirectoryName::run()
{
	using Global::wFooter;
	
	if (myScreen == myBrowser)
	{
		std::string old_dir = myBrowser->main().current().value().name;
		Statusbar::lock();
		Statusbar::put() << NC::Format::Bold << "Directory: " << NC::Format::NoBold;
		std::string new_dir = wFooter->getString(old_dir);
		Statusbar::unlock();
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir;
			if (!myBrowser->isLocal())
				full_old_dir += Config.mpd_music_dir;
			full_old_dir += old_dir;
			std::string full_new_dir;
			if (!myBrowser->isLocal())
				full_new_dir += Config.mpd_music_dir;
			full_new_dir += new_dir;
			int rename_result = rename(full_old_dir.c_str(), full_new_dir.c_str());
			if (rename_result == 0)
			{
				const char msg[] = "Directory renamed to \"%1%\"";
				Statusbar::printf(msg, wideShorten(new_dir, COLS-const_strlen(msg)));
				if (!myBrowser->isLocal())
					Mpd.UpdateDirectory(getSharedDirectory(old_dir, new_dir));
				myBrowser->GetDirectory(myBrowser->CurrentDir());
			}
			else
			{
				const char msg[] = "Couldn't rename \"%1%\": %s";
				Statusbar::printf(msg, wideShorten(old_dir, COLS-const_strlen(msg)-25), strerror(errno));
			}
		}
	}
#	ifdef HAVE_TAGLIB_H
	else if (myScreen->activeWindow() == myTagEditor->Dirs)
	{
		std::string old_dir = myTagEditor->Dirs->current().value().first;
		Statusbar::lock();
		Statusbar::put() << NC::Format::Bold << "Directory: " << NC::Format::NoBold;
		std::string new_dir = wFooter->getString(old_dir);
		Statusbar::unlock();
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + old_dir;
			std::string full_new_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + new_dir;
			if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
			{
				const char msg[] = "Directory renamed to \"%1%\"";
				Statusbar::printf(msg, wideShorten(new_dir, COLS-const_strlen(msg)));
				Mpd.UpdateDirectory(myTagEditor->CurrentDir());
			}
			else
			{
				const char msg[] = "Couldn't rename \"%1%\": %2%";
				Statusbar::printf(msg, wideShorten(old_dir, COLS-const_strlen(msg)-25), strerror(errno));
			}
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditPlaylistName::canBeRun() const
{
	return   (myScreen->isActiveWindow(myPlaylistEditor->Playlists)
	      && !myPlaylistEditor->Playlists.empty())
	    ||   (myScreen == myBrowser
	      && !myBrowser->main().empty()
		  && myBrowser->main().current().value().type == MPD::itPlaylist);
}

void EditPlaylistName::run()
{
	using Global::wFooter;
	
	std::string old_name;
	if (myScreen->isActiveWindow(myPlaylistEditor->Playlists))
		old_name = myPlaylistEditor->Playlists.current().value();
	else
		old_name = myBrowser->main().current().value().name;
	Statusbar::lock();
	Statusbar::put() << NC::Format::Bold << "Playlist: " << NC::Format::NoBold;
	std::string new_name = wFooter->getString(old_name);
	Statusbar::unlock();
	if (!new_name.empty() && new_name != old_name)
	{
		Mpd.Rename(old_name, new_name);
		const char msg[] = "Playlist renamed to \"%1%\"";
		Statusbar::printf(msg, wideShorten(new_name, COLS-const_strlen(msg)));
		if (!myBrowser->isLocal())
			myBrowser->GetDirectory("/");
	}
}

bool EditLyrics::canBeRun() const
{
	return myScreen == myLyrics;
}

void EditLyrics::run()
{
	myLyrics->Edit();
}

bool JumpToBrowser::canBeRun() const
{
	return currentSong(myScreen);
}

void JumpToBrowser::run()
{
	auto s = currentSong(myScreen);
	myBrowser->LocateSong(*s);
}

bool JumpToMediaLibrary::canBeRun() const
{
	return currentSong(myScreen);
}

void JumpToMediaLibrary::run()
{
	auto s = currentSong(myScreen);
	myLibrary->LocateSong(*s);
}

bool JumpToPlaylistEditor::canBeRun() const
{
	return myScreen == myBrowser
	    && myBrowser->main().current().value().type == MPD::itPlaylist;
}

void JumpToPlaylistEditor::run()
{
	myPlaylistEditor->Locate(myBrowser->main().current().value().name);
}

void ToggleScreenLock::run()
{
	using Global::wFooter;
	using Global::myLockedScreen;
	
	if (myLockedScreen != 0)
	{
		BaseScreen::unlock();
		Actions::setResizeFlags();
		myScreen->resize();
		Statusbar::print("Screen unlocked");
	}
	else
	{
		int part = Config.locked_screen_width_part*100;
		if (Config.ask_for_locked_screen_width_part)
		{
			Statusbar::lock();
			Statusbar::put() << "% of the locked screen's width to be reserved (20-80): ";
			std::string strpart = wFooter->getString(boost::lexical_cast<std::string>(part));
			Statusbar::unlock();
			part = fromString<unsigned>(strpart);
		}
		boundsCheck(part, 20, 80);
		Config.locked_screen_width_part = part/100.0;
		if (myScreen->lock())
			Statusbar::printf("Screen locked (with %1%%% width)", part);
		else
			Statusbar::print("Current screen can't be locked");
	}
}

bool JumpToTagEditor::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return currentSong(myScreen)
	    && isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void JumpToTagEditor::run()
{
#	ifdef HAVE_TAGLIB_H
	auto s = currentSong(myScreen);
	myTagEditor->LocateSong(*s);
#	endif // HAVE_TAGLIB_H
}

bool JumpToPositionInSong::canBeRun() const
{
	return Status::State::player() != MPD::psStop && Status::State::totalTime() > 0;
}

void JumpToPositionInSong::run()
{
	using Global::wFooter;
	
	const MPD::Song s = myPlaylist->nowPlayingSong();
	
	Statusbar::lock();
	Statusbar::put() << "Position to go (in %/m:ss/seconds(s)): ";
	std::string strpos = wFooter->getString();
	Statusbar::unlock();
	
	boost::regex rx;
	boost::smatch what;
	
	if (boost::regex_match(strpos, what, rx.assign("([0-9]+):([0-9]{2})"))) // mm:ss
	{
		int mins = fromString<int>(what[1]);
		int secs = fromString<int>(what[2]);
		boundsCheck(secs, 0, 60);
		Mpd.Seek(s.getPosition(), mins * 60 + secs);
	}
	else if (boost::regex_match(strpos, what, rx.assign("([0-9]+)s"))) // position in seconds
	{
		int secs = fromString<int>(what[1]);
		Mpd.Seek(s.getPosition(), secs);
	}
	else if (boost::regex_match(strpos, what, rx.assign("([0-9]+)[%]{0,1}"))) // position in %
	{
		int percent = fromString<int>(what[1]);
		boundsCheck(percent, 0, 100);
		int secs = (percent * s.getDuration()) / 100.0;
		Mpd.Seek(s.getPosition(), secs);
	}
	else
		Statusbar::print("Invalid format ([m]:[ss], [s]s, [%]%, [%] accepted)");
}

bool ReverseSelection::canBeRun() const
{
	auto w = hasSongs(myScreen);
	return w && w->allowsSelection();
}

void ReverseSelection::run()
{
	auto w = hasSongs(myScreen);
	w->reverseSelection();
	Statusbar::print("Selection reversed");
}

bool RemoveSelection::canBeRun() const
{
	return proxySongList(myScreen);
}

void RemoveSelection::run()
{
	auto pl = proxySongList(myScreen);
	for (size_t i = 0; i < pl.size(); ++i)
		pl.setSelected(i, false);
	Statusbar::print("Selection removed");
}

bool SelectAlbum::canBeRun() const
{
	auto w = hasSongs(myScreen);
	return w && w->allowsSelection() && w->proxySongList();
}

void SelectAlbum::run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	if (pl.empty())
		return;
	size_t pos = pl.choice();
	if (MPD::Song *s = pl.getSong(pos))
	{
		std::string album = s->getAlbum();
		// select song under cursor
		pl.setSelected(pos, true);
		// go up
		while (pos > 0)
		{
			s = pl.getSong(--pos);
			if (!s || s->getAlbum() != album)
				break;
			else
				pl.setSelected(pos, true);
		}
		// go down
		pos = pl.choice();
		while (pos < pl.size() - 1)
		{
			s = pl.getSong(++pos);
			if (!s || s->getAlbum() != album)
				break;
			else
				pl.setSelected(pos, true);
		}
		Statusbar::print("Album around cursor position selected");
	}
}

bool AddSelectedItems::canBeRun() const
{
	return myScreen != mySelectedItemsAdder;
}

void AddSelectedItems::run()
{
	mySelectedItemsAdder->switchTo();
}

void CropMainPlaylist::run()
{
	auto &w = myPlaylist->main();
	// cropping doesn't make sense in this case
	if (w.size() <= 1)
		return;
	bool yes = true;
	if (Config.ask_before_clearing_playlists)
		yes = askYesNoQuestion("Do you really want to crop main playlist?", Status::trace);
	if (yes)
	{
		Statusbar::print("Cropping playlist...");
		selectCurrentIfNoneSelected(w);
		cropPlaylist(w, boost::bind(&MPD::Connection::Delete, _1, _2));
		Statusbar::print("Playlist cropped");
	}
}

bool CropPlaylist::canBeRun() const
{
	return myScreen == myPlaylistEditor;
}

void CropPlaylist::run()
{
	auto &w = myPlaylistEditor->Content;
	// cropping doesn't make sense in this case
	if (w.size() <= 1)
		return;
	assert(!myPlaylistEditor->Playlists.empty());
	std::string playlist = myPlaylistEditor->Playlists.current().value();
	bool yes = true;
	if (Config.ask_before_clearing_playlists)
		yes = askYesNoQuestion(
			boost::format("Do you really want to crop playlist \"%1%\"?") % playlist,
			Status::trace
		);
	if (yes)
	{
		selectCurrentIfNoneSelected(w);
		Statusbar::printf("Cropping playlist \"%1%\"...", playlist);
		cropPlaylist(w, boost::bind(&MPD::Connection::PlaylistDelete, _1, playlist, _2));
		Statusbar::printf("Playlist \"%1%\" cropped", playlist);
	}
}

void ClearMainPlaylist::run()
{
	bool yes = true;
	if (Config.ask_before_clearing_playlists)
		yes = askYesNoQuestion("Do you really want to clear main playlist?", Status::trace);
	if (yes)
	{
		auto delete_fun = boost::bind(&MPD::Connection::Delete, _1, _2);
		auto clear_fun = boost::bind(&MPD::Connection::ClearMainPlaylist, _1);
		Statusbar::printf("Deleting items...");
		clearPlaylist(myPlaylist->main(), delete_fun, clear_fun);
		Statusbar::printf("Items deleted");
		myPlaylist->main().reset();
	}
}

bool ClearPlaylist::canBeRun() const
{
	return myScreen == myPlaylistEditor;
}

void ClearPlaylist::run()
{
	assert(!myPlaylistEditor->Playlists.empty());
	std::string playlist = myPlaylistEditor->Playlists.current().value();
	bool yes = true;
	if (Config.ask_before_clearing_playlists)
		yes = askYesNoQuestion(
			boost::format("Do you really want to clear playlist \"%1%\"?") % playlist,
			Status::trace
		);
	if (yes)
	{
		auto delete_fun = boost::bind(&MPD::Connection::PlaylistDelete, _1, playlist, _2);
		auto clear_fun = boost::bind(&MPD::Connection::ClearPlaylist, _1, playlist);
		Statusbar::printf("Deleting items from \"%1%\"...", playlist);
		clearPlaylist(myPlaylistEditor->Content, delete_fun, clear_fun);
		Statusbar::printf("Items deleted from \"%1%\"", playlist);
	}
}

bool SortPlaylist::canBeRun() const
{
	return myScreen == myPlaylist;
}

void SortPlaylist::run()
{
	mySortPlaylistDialog->switchTo();
}

bool ReversePlaylist::canBeRun() const
{
	return myScreen == myPlaylist;
}

void ReversePlaylist::run()
{
	myPlaylist->Reverse();
}

bool ApplyFilter::canBeRun() const
{
	auto w = dynamic_cast<Filterable *>(myScreen);
	return w && w->allowsFiltering();
}

void ApplyFilter::run()
{
	using Global::wFooter;
	
	Filterable *f = dynamic_cast<Filterable *>(myScreen);
	std::string filter = f->currentFilter();
	// if filter is already here, apply it
	if (!filter.empty())
	{
		f->applyFilter(filter);
		myScreen->refreshWindow();
	}

	Statusbar::lock();
	Statusbar::put() << NC::Format::Bold << "Apply filter: " << NC::Format::NoBold;
	wFooter->setGetStringHelper(Statusbar::Helpers::ApplyFilterImmediately(f, filter));
	wFooter->getString(filter);
	wFooter->setGetStringHelper(Statusbar::Helpers::getString);
	Statusbar::unlock();
	
	filter = f->currentFilter();
 	if (filter.empty())
	{
		myPlaylist->main().clearFilterResults();
		Statusbar::printf("Filtering disabled");
	}
 	else
	{
		// apply filter here so even if old one wasn't modified
		// (and callback wasn't invoked), it still gets applied.
		f->applyFilter(filter);
		Statusbar::printf("Using filter \"%1%\"", filter);
	}
	
	if (myScreen == myPlaylist)
	{
		myPlaylist->EnableHighlighting();
		myPlaylist->reloadTotalLength();
		drawHeader();
	}
	listsChangeFinisher();
}

bool Find::canBeRun() const
{
	return myScreen == myHelp
	    || myScreen == myLyrics
#	ifdef HAVE_CURL_CURL_H
	    || myScreen == myLastfm
#	endif // HAVE_CURL_CURL_H
	;
}

void Find::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Find: ";
	std::string findme = wFooter->getString();
	Statusbar::unlock();
	
	Statusbar::print("Searching...");
	auto s = static_cast<Screen<NC::Scrollpad> *>(myScreen);
	s->main().removeProperties();
	if (findme.empty() || s->main().setProperties(NC::Format::Reverse, findme, NC::Format::NoReverse))
		Statusbar::print("Done");
	else
		Statusbar::print("No matching patterns found");
	s->main().flush();
}

bool FindItemBackward::canBeRun() const
{
	auto w = dynamic_cast<Searchable *>(myScreen);
	return w && w->allowsSearching();
}

void FindItemForward::run()
{
	findItem(::Find::Forward);
	listsChangeFinisher();
}

bool FindItemForward::canBeRun() const
{
	auto w = dynamic_cast<Searchable *>(myScreen);
	return w && w->allowsSearching();
}

void FindItemBackward::run()
{
	findItem(::Find::Backward);
	listsChangeFinisher();
}

bool NextFoundItem::canBeRun() const
{
	return dynamic_cast<Searchable *>(myScreen);
}

void NextFoundItem::run()
{
	Searchable *w = dynamic_cast<Searchable *>(myScreen);
	w->nextFound(Config.wrapped_search);
	listsChangeFinisher();
}

bool PreviousFoundItem::canBeRun() const
{
	return dynamic_cast<Searchable *>(myScreen);
}

void PreviousFoundItem::run()
{
	Searchable *w = dynamic_cast<Searchable *>(myScreen);
	w->prevFound(Config.wrapped_search);
	listsChangeFinisher();
}

void ToggleFindMode::run()
{
	Config.wrapped_search = !Config.wrapped_search;
	Statusbar::printf("Search mode: %1%",
		Config.wrapped_search ? "Wrapped" : "Normal"
	);
}

void ToggleReplayGainMode::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Replay gain mode? [" << NC::Format::Bold << 'o' << NC::Format::NoBold << "ff/" << NC::Format::Bold << 't' << NC::Format::NoBold << "rack/" << NC::Format::Bold << 'a' << NC::Format::NoBold << "lbum]";
	wFooter->refresh();
	int answer = 0;
	do
	{
		Status::trace();
		answer = wFooter->readKey();
	}
	while (answer != 'o' && answer != 't' && answer != 'a');
	Statusbar::unlock();
	Mpd.SetReplayGainMode(answer == 't' ? MPD::rgmTrack : (answer == 'a' ? MPD::rgmAlbum : MPD::rgmOff));
	Statusbar::printf("Replay gain mode: %1%", Mpd.GetReplayGainMode());
}

void ToggleSpaceMode::run()
{
	Config.space_selects = !Config.space_selects;
	Statusbar::printf("Space mode: %1% item", Config.space_selects ? "select" : "add");
}

void ToggleAddMode::run()
{
	std::string mode_desc;
	switch (Config.space_add_mode)
	{
		case SpaceAddMode::AddRemove:
			Config.space_add_mode = SpaceAddMode::AlwaysAdd;
			mode_desc = "always add an item to playlist";
			break;
		case SpaceAddMode::AlwaysAdd:
			Config.space_add_mode = SpaceAddMode::AddRemove;
			mode_desc = "add an item to playlist or remove if already added";
			break;
	}
	Statusbar::printf("Add mode: %1%", mode_desc);
}

void ToggleMouse::run()
{
	Config.mouse_support = !Config.mouse_support;
	mousemask(Config.mouse_support ? ALL_MOUSE_EVENTS : 0, 0);
	Statusbar::printf("Mouse support %1%",
		Config.mouse_support ? "enabled" : "disabled"
	);
}

void ToggleBitrateVisibility::run()
{
	Config.display_bitrate = !Config.display_bitrate;
	Statusbar::printf("Bitrate visibility %1%",
		Config.display_bitrate ? "enabled" : "disabled"
	);
}

void AddRandomItems::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Add random? [" << NC::Format::Bold << 's' << NC::Format::NoBold << "ongs/" << NC::Format::Bold << 'a' << NC::Format::NoBold << "rtists/al" << NC::Format::Bold << 'b' << NC::Format::NoBold << "ums] ";
	wFooter->refresh();
	int answer = 0;
	do
	{
		Status::trace();
		answer = wFooter->readKey();
	}
	while (answer != 's' && answer != 'a' && answer != 'b');
	Statusbar::unlock();
	
	mpd_tag_type tag_type = MPD_TAG_ARTIST;
	std::string tag_type_str ;
	if (answer != 's')
	{
		tag_type = charToTagType(answer);
		tag_type_str = boost::locale::to_lower(tagTypeToString(tag_type));
	}
	else
		tag_type_str = "song";
	
	Statusbar::lock();
	Statusbar::put() << "Number of random " << tag_type_str << "s: ";
	std::string strnum = wFooter->getString();
	Statusbar::unlock();
	size_t number = fromString<size_t>(strnum);
	if (number && (answer == 's' ? Mpd.AddRandomSongs(number) : Mpd.AddRandomTag(tag_type, number)))
	{
		Statusbar::printf("%1% random %2%%3% added to playlist",
			number, tag_type_str, number == 1 ? "" : "s"
		);
	}
}

bool ToggleBrowserSortMode::canBeRun() const
{
	return myScreen == myBrowser;
}

void ToggleBrowserSortMode::run()
{
	switch (Config.browser_sort_mode)
	{
		case SortMode::Name:
			Config.browser_sort_mode = SortMode::ModificationTime;
			Statusbar::print("Sort songs by: modification time");
			break;
		case SortMode::ModificationTime:
			Config.browser_sort_mode = SortMode::CustomFormat;
			Statusbar::print("Sort songs by: custom format");
			break;
		case SortMode::CustomFormat:
			Config.browser_sort_mode = SortMode::NoOp;
			Statusbar::print("Do not sort songs");
			break;
		case SortMode::NoOp:
			Config.browser_sort_mode = SortMode::Name;
			Statusbar::print("Sort songs by: name");
	}
	withUnfilteredMenuReapplyFilter(myBrowser->main(), [] {
		if (Config.browser_sort_mode != SortMode::NoOp)
			std::sort(myBrowser->main().begin()+(myBrowser->CurrentDir() != "/"), myBrowser->main().end(),
				LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the, Config.browser_sort_mode)
			);
	});
}

bool ToggleLibraryTagType::canBeRun() const
{
	return (myScreen->isActiveWindow(myLibrary->Tags))
	    || (myLibrary->Columns() == 2 && myScreen->isActiveWindow(myLibrary->Albums));
}

void ToggleLibraryTagType::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Tag type? [" << NC::Format::Bold << 'a' << NC::Format::NoBold << "rtist/album" << NC::Format::Bold << 'A' << NC::Format::NoBold << "rtist/" << NC::Format::Bold << 'y' << NC::Format::NoBold << "ear/" << NC::Format::Bold << 'g' << NC::Format::NoBold << "enre/" << NC::Format::Bold << 'c' << NC::Format::NoBold << "omposer/" << NC::Format::Bold << 'p' << NC::Format::NoBold << "erformer] ";
	wFooter->refresh();
	int answer = 0;
	do
	{
		Status::trace();
		answer = wFooter->readKey();
	}
	while (answer != 'a' && answer != 'A' && answer != 'y' && answer != 'g' && answer != 'c' && answer != 'p');
	Statusbar::unlock();
	mpd_tag_type new_tagitem = charToTagType(answer);
	if (new_tagitem != Config.media_lib_primary_tag)
	{
		Config.media_lib_primary_tag = new_tagitem;
		std::string item_type = tagTypeToString(Config.media_lib_primary_tag);
		myLibrary->Tags.setTitle(Config.titles_visibility ? item_type + "s" : "");
		myLibrary->Tags.reset();
		item_type = boost::locale::to_lower(item_type);
		std::string and_mtime = Config.media_library_sort_by_mtime ?
		                        " and mtime" :
		                        "";
		if (myLibrary->Columns() == 2)
		{
			myLibrary->Songs.clear();
			myLibrary->Albums.reset();
			myLibrary->Albums.clear();
			myLibrary->Albums.setTitle(Config.titles_visibility ? "Albums (sorted by " + item_type + and_mtime + ")" : "");
			myLibrary->Albums.display();
		}
		else
		{
			myLibrary->Tags.clear();
			myLibrary->Tags.display();
		}
		Statusbar::printf("Switched to the list of %1%s", item_type);
	}
}

bool ToggleMediaLibrarySortMode::canBeRun() const
{
	return myScreen == myLibrary;
}

void ToggleMediaLibrarySortMode::run()
{
	myLibrary->toggleSortMode();
}

bool RefetchLyrics::canBeRun() const
{
#	ifdef HAVE_CURL_CURL_H
	return myScreen == myLyrics;
#	else
	return false;
#	endif // HAVE_CURL_CURL_H
}

void RefetchLyrics::run()
{
#	ifdef HAVE_CURL_CURL_H
	myLyrics->Refetch();
#	endif // HAVE_CURL_CURL_H
}

bool SetSelectedItemsPriority::canBeRun() const
{
	if (Mpd.Version() < 17)
	{
		Statusbar::print("Priorities are supported in MPD >= 0.17.0");
		return false;
	}
	return myScreen == myPlaylist && !myPlaylist->main().empty();
}

void SetSelectedItemsPriority::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Set priority [0-255]: ";
	std::string strprio = wFooter->getString();
	Statusbar::unlock();
	unsigned prio = fromString<unsigned>(strprio);
	boundsCheck(prio, 0u, 255u);
	myPlaylist->SetSelectedItemsPriority(prio);
}

bool SetVisualizerSampleMultiplier::canBeRun() const
{
#	ifdef ENABLE_VISUALIZER
	return myScreen == myVisualizer;
#	else
	return false;
#	endif // ENABLE_VISUALIZER
}

void SetVisualizerSampleMultiplier::run()
{
#	ifdef ENABLE_VISUALIZER
	using Global::wFooter;

	Statusbar::lock();
	Statusbar::put() << "Set visualizer sample multiplier: ";
	std::string smultiplier = wFooter->getString();
	Statusbar::unlock();

	double multiplier = fromString<double>(smultiplier);
	lowerBoundCheck(multiplier, 0.0);
	Config.visualizer_sample_multiplier = multiplier;
#	endif // ENABLE_VISUALIZER
}

bool FilterPlaylistOnPriorities::canBeRun() const
{
	return myScreen == myPlaylist;
}

void FilterPlaylistOnPriorities::run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Show songs with priority higher than: ";
	std::string strprio = wFooter->getString();
	Statusbar::unlock();
	unsigned prio = fromString<unsigned>(strprio);
	boundsCheck(prio, 0u, 255u);
	myPlaylist->main().filter(myPlaylist->main().begin(), myPlaylist->main().end(),
		[prio](const NC::Menu<MPD::Song>::Item &s) {
			return s.value().getPrio() > prio;
	});
	Statusbar::printf("Playlist filtered (songs with priority higher than %1%)", prio);
}

void ShowSongInfo::run()
{
	mySongInfo->switchTo();
}

bool ShowArtistInfo::canBeRun() const
{
	#ifdef HAVE_CURL_CURL_H
	return myScreen == myLastfm
	  ||   (myScreen->isActiveWindow(myLibrary->Tags)
	    && !myLibrary->Tags.empty()
	    && Config.media_lib_primary_tag == MPD_TAG_ARTIST)
	  ||   currentSong(myScreen);
#	else
	return false;
#	endif // NOT HAVE_CURL_CURL_H
}

void ShowArtistInfo::run()
{
#	ifdef HAVE_CURL_CURL_H
	if (myScreen == myLastfm)
	{
		myLastfm->switchTo();
		return;
	}
	
	std::string artist;
	if (myScreen->isActiveWindow(myLibrary->Tags))
	{
		assert(!myLibrary->Tags.empty());
		assert(Config.media_lib_primary_tag == MPD_TAG_ARTIST);
		artist = myLibrary->Tags.current().value().tag();
	}
	else
	{
		auto s = currentSong(myScreen);
		assert(s);
		artist = s->getArtist();
	}
	
	if (!artist.empty())
	{
		myLastfm->queueJob(LastFm::ArtistInfo(artist, Config.lastfm_preferred_language));
		myLastfm->switchTo();
	}
#	endif // HAVE_CURL_CURL_H
}

void ShowLyrics::run()
{
	myLyrics->switchTo();
}

void Quit::run()
{
	ExitMainLoop = true;
}

void NextScreen::run()
{
	if (Config.screen_switcher_previous)
	{
		if (auto tababble = dynamic_cast<Tabbable *>(myScreen))
			tababble->switchToPreviousScreen();
	}
	else if (!Config.screen_sequence.empty())
	{
		const auto &seq = Config.screen_sequence;
		auto screen_type = std::find(seq.begin(), seq.end(), myScreen->type());
		if (++screen_type == seq.end())
			toScreen(seq.front())->switchTo();
		else
			toScreen(*screen_type)->switchTo();
	}
}

void PreviousScreen::run()
{
	if (Config.screen_switcher_previous)
	{
		if (auto tababble = dynamic_cast<Tabbable *>(myScreen))
			tababble->switchToPreviousScreen();
	}
	else if (!Config.screen_sequence.empty())
	{
		const auto &seq = Config.screen_sequence;
		auto screen_type = std::find(seq.begin(), seq.end(), myScreen->type());
		if (screen_type == seq.begin())
			toScreen(seq.back())->switchTo();
		else
			toScreen(*--screen_type)->switchTo();
	}
}

bool ShowHelp::canBeRun() const
{
	return myScreen != myHelp
#	ifdef HAVE_TAGLIB_H
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowHelp::run()
{
	myHelp->switchTo();
}

bool ShowPlaylist::canBeRun() const
{
	return myScreen != myPlaylist
#	ifdef HAVE_TAGLIB_H
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowPlaylist::run()
{
	myPlaylist->switchTo();
}

bool ShowBrowser::canBeRun() const
{
	return myScreen != myBrowser
#	ifdef HAVE_TAGLIB_H
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowBrowser::run()
{
	myBrowser->switchTo();
}

bool ChangeBrowseMode::canBeRun() const
{
	return myScreen == myBrowser;
}

void ChangeBrowseMode::run()
{
	myBrowser->ChangeBrowseMode();
}

bool ShowSearchEngine::canBeRun() const
{
	return myScreen != mySearcher
#	ifdef HAVE_TAGLIB_H
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowSearchEngine::run()
{
	mySearcher->switchTo();
}

bool ResetSearchEngine::canBeRun() const
{
	return myScreen == mySearcher;
}

void ResetSearchEngine::run()
{
	mySearcher->reset();
}

bool ShowMediaLibrary::canBeRun() const
{
	return myScreen != myLibrary
#	ifdef HAVE_TAGLIB_H
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowMediaLibrary::run()
{
	myLibrary->switchTo();
}

bool ToggleMediaLibraryColumnsMode::canBeRun() const
{
	return myScreen == myLibrary;
}

void ToggleMediaLibraryColumnsMode::run()
{
	myLibrary->toggleColumnsMode();
	myLibrary->refresh();
}

bool ShowPlaylistEditor::canBeRun() const
{
	return myScreen != myPlaylistEditor
#	ifdef HAVE_TAGLIB_H
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowPlaylistEditor::run()
{
	myPlaylistEditor->switchTo();
}

bool ShowTagEditor::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen != myTagEditor
	    && myScreen != myTinyTagEditor;
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void ShowTagEditor::run()
{
#	ifdef HAVE_TAGLIB_H
	if (isMPDMusicDirSet())
		myTagEditor->switchTo();
#	endif // HAVE_TAGLIB_H
}

bool ShowOutputs::canBeRun() const
{
#	ifdef ENABLE_OUTPUTS
	return myScreen != myOutputs
#	ifdef HAVE_TAGLIB_H	
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
#	else
	return false;
#	endif // ENABLE_OUTPUTS
}

void ShowOutputs::run()
{
#	ifdef ENABLE_OUTPUTS
	myOutputs->switchTo();
#	endif // ENABLE_OUTPUTS
}

bool ShowVisualizer::canBeRun() const
{
#	ifdef ENABLE_VISUALIZER
	return myScreen != myVisualizer
#	ifdef HAVE_TAGLIB_H
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
#	else
	return false;
#	endif // ENABLE_VISUALIZER
}

void ShowVisualizer::run()
{
#	ifdef ENABLE_VISUALIZER
	myVisualizer->switchTo();
#	endif // ENABLE_VISUALIZER
}

bool ShowClock::canBeRun() const
{
#	ifdef ENABLE_CLOCK
	return myScreen != myClock
#	ifdef HAVE_TAGLIB_H
	    && myScreen != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
#	else
	return false;
#	endif // ENABLE_CLOCK
}

void ShowClock::run()
{
#	ifdef ENABLE_CLOCK
	myClock->switchTo();
#	endif // ENABLE_CLOCK
}

#ifdef HAVE_TAGLIB_H
bool ShowServerInfo::canBeRun() const
{
	return myScreen != myTinyTagEditor;
}
#endif // HAVE_TAGLIB_H

void ShowServerInfo::run()
{
	myServerInfo->switchTo();
}

}

namespace {//

void populateActions()
{
	auto insert_action = [](Actions::BaseAction *a) {
		AvailableActions[static_cast<size_t>(a->type())] = a;
	};
	insert_action(new Actions::Dummy());
	insert_action(new Actions::MouseEvent());
	insert_action(new Actions::ScrollUp());
	insert_action(new Actions::ScrollDown());
	insert_action(new Actions::ScrollUpArtist());
	insert_action(new Actions::ScrollUpAlbum());
	insert_action(new Actions::ScrollDownArtist());
	insert_action(new Actions::ScrollDownAlbum());
	insert_action(new Actions::PageUp());
	insert_action(new Actions::PageDown());
	insert_action(new Actions::MoveHome());
	insert_action(new Actions::MoveEnd());
	insert_action(new Actions::ToggleInterface());
	insert_action(new Actions::JumpToParentDirectory());
	insert_action(new Actions::PressEnter());
	insert_action(new Actions::PressSpace());
	insert_action(new Actions::PreviousColumn());
	insert_action(new Actions::NextColumn());
	insert_action(new Actions::MasterScreen());
	insert_action(new Actions::SlaveScreen());
	insert_action(new Actions::VolumeUp());
	insert_action(new Actions::VolumeDown());
	insert_action(new Actions::DeletePlaylistItems());
	insert_action(new Actions::DeleteStoredPlaylist());
	insert_action(new Actions::DeleteBrowserItems());
	insert_action(new Actions::ReplaySong());
	insert_action(new Actions::PreviousSong());
	insert_action(new Actions::NextSong());
	insert_action(new Actions::Pause());
	insert_action(new Actions::Stop());
	insert_action(new Actions::ExecuteCommand());
	insert_action(new Actions::SavePlaylist());
	insert_action(new Actions::MoveSortOrderUp());
	insert_action(new Actions::MoveSortOrderDown());
	insert_action(new Actions::MoveSelectedItemsUp());
	insert_action(new Actions::MoveSelectedItemsDown());
	insert_action(new Actions::MoveSelectedItemsTo());
	insert_action(new Actions::Add());
	insert_action(new Actions::SeekForward());
	insert_action(new Actions::SeekBackward());
	insert_action(new Actions::ToggleDisplayMode());
	insert_action(new Actions::ToggleSeparatorsBetweenAlbums());
	insert_action(new Actions::ToggleLyricsFetcher());
	insert_action(new Actions::ToggleFetchingLyricsInBackground());
	insert_action(new Actions::TogglePlayingSongCentering());
	insert_action(new Actions::UpdateDatabase());
	insert_action(new Actions::JumpToPlayingSong());
	insert_action(new Actions::ToggleRepeat());
	insert_action(new Actions::Shuffle());
	insert_action(new Actions::ToggleRandom());
	insert_action(new Actions::StartSearching());
	insert_action(new Actions::SaveTagChanges());
	insert_action(new Actions::ToggleSingle());
	insert_action(new Actions::ToggleConsume());
	insert_action(new Actions::ToggleCrossfade());
	insert_action(new Actions::SetCrossfade());
	insert_action(new Actions::SetVolume());
	insert_action(new Actions::EditSong());
	insert_action(new Actions::EditLibraryTag());
	insert_action(new Actions::EditLibraryAlbum());
	insert_action(new Actions::EditDirectoryName());
	insert_action(new Actions::EditPlaylistName());
	insert_action(new Actions::EditLyrics());
	insert_action(new Actions::JumpToBrowser());
	insert_action(new Actions::JumpToMediaLibrary());
	insert_action(new Actions::JumpToPlaylistEditor());
	insert_action(new Actions::ToggleScreenLock());
	insert_action(new Actions::JumpToTagEditor());
	insert_action(new Actions::JumpToPositionInSong());
	insert_action(new Actions::ReverseSelection());
	insert_action(new Actions::RemoveSelection());
	insert_action(new Actions::SelectAlbum());
	insert_action(new Actions::AddSelectedItems());
	insert_action(new Actions::CropMainPlaylist());
	insert_action(new Actions::CropPlaylist());
	insert_action(new Actions::ClearMainPlaylist());
	insert_action(new Actions::ClearPlaylist());
	insert_action(new Actions::SortPlaylist());
	insert_action(new Actions::ReversePlaylist());
	insert_action(new Actions::ApplyFilter());
	insert_action(new Actions::Find());
	insert_action(new Actions::FindItemForward());
	insert_action(new Actions::FindItemBackward());
	insert_action(new Actions::NextFoundItem());
	insert_action(new Actions::PreviousFoundItem());
	insert_action(new Actions::ToggleFindMode());
	insert_action(new Actions::ToggleReplayGainMode());
	insert_action(new Actions::ToggleSpaceMode());
	insert_action(new Actions::ToggleAddMode());
	insert_action(new Actions::ToggleMouse());
	insert_action(new Actions::ToggleBitrateVisibility());
	insert_action(new Actions::AddRandomItems());
	insert_action(new Actions::ToggleBrowserSortMode());
	insert_action(new Actions::ToggleLibraryTagType());
	insert_action(new Actions::ToggleMediaLibrarySortMode());
	insert_action(new Actions::RefetchLyrics());
	insert_action(new Actions::SetSelectedItemsPriority());
	insert_action(new Actions::SetVisualizerSampleMultiplier());
	insert_action(new Actions::FilterPlaylistOnPriorities());
	insert_action(new Actions::ShowSongInfo());
	insert_action(new Actions::ShowArtistInfo());
	insert_action(new Actions::ShowLyrics());
	insert_action(new Actions::Quit());
	insert_action(new Actions::NextScreen());
	insert_action(new Actions::PreviousScreen());
	insert_action(new Actions::ShowHelp());
	insert_action(new Actions::ShowPlaylist());
	insert_action(new Actions::ShowBrowser());
	insert_action(new Actions::ChangeBrowseMode());
	insert_action(new Actions::ShowSearchEngine());
	insert_action(new Actions::ResetSearchEngine());
	insert_action(new Actions::ShowMediaLibrary());
	insert_action(new Actions::ToggleMediaLibraryColumnsMode());
	insert_action(new Actions::ShowPlaylistEditor());
	insert_action(new Actions::ShowTagEditor());
	insert_action(new Actions::ShowOutputs());
	insert_action(new Actions::ShowVisualizer());
	insert_action(new Actions::ShowClock());
	insert_action(new Actions::ShowServerInfo());
}

void seek()
{
	using Global::wHeader;
	using Global::wFooter;
	using Global::Timer;
	using Global::SeekingInProgress;
	
	if (!Status::State::totalTime())
	{
		Statusbar::print("Unknown item length");
		return;
	}
	
	Progressbar::lock();
	Statusbar::lock();

	unsigned songpos = Status::State::elapsedTime();
	auto t = Timer;
	
	int old_timeout = wFooter->getTimeout();
	wFooter->setTimeout(500);
	
	auto seekForward = &Actions::get(Actions::Type::SeekForward);
	auto seekBackward = &Actions::get(Actions::Type::SeekBackward);
	
	SeekingInProgress = true;
	while (true)
	{
		Status::trace();
		
		unsigned howmuch = Config.incremental_seeking
		                 ? (Timer-t).seconds()/2+Config.seek_time
		                 : Config.seek_time;
		
		Key input = Key::read(*wFooter);
		auto k = Bindings.get(input);
		if (k.first == k.second || !k.first->isSingle()) // no single action?
			break;
		auto a = k.first->action();
		if (a == seekForward)
		{
			if (songpos < Status::State::totalTime())
				songpos = std::min(songpos + howmuch, Status::State::totalTime());
		}
		else if (a == seekBackward)
		{
			if (songpos > 0)
			{
				if (songpos < howmuch)
					songpos = 0;
				else
					songpos -= howmuch;
			}
		}
		else
			break;
		
		*wFooter << NC::Format::Bold;
		std::string tracklength;
		switch (Config.design)
		{
			case Design::Classic:
				tracklength = " [";
				if (Config.display_remaining_time)
				{
					tracklength += "-";
					tracklength += MPD::Song::ShowTime(Status::State::totalTime()-songpos);
				}
				else
					tracklength += MPD::Song::ShowTime(songpos);
				tracklength += "/";
				tracklength += MPD::Song::ShowTime(Status::State::totalTime());
				tracklength += "]";
				*wFooter << NC::XY(wFooter->getWidth()-tracklength.length(), 1) << tracklength;
				break;
			case Design::Alternative:
				if (Config.display_remaining_time)
				{
					tracklength = "-";
					tracklength += MPD::Song::ShowTime(Status::State::totalTime()-songpos);
				}
				else
					tracklength = MPD::Song::ShowTime(songpos);
				tracklength += "/";
				tracklength += MPD::Song::ShowTime(Status::State::totalTime());
				*wHeader << NC::XY(0, 0) << tracklength << " ";
				wHeader->refresh();
				break;
		}
		*wFooter << NC::Format::NoBold;
		Progressbar::draw(songpos, Status::State::totalTime());
		wFooter->refresh();
	}
	SeekingInProgress = false;
	Mpd.Seek(Status::State::currentSongPosition(), songpos);
	
	wFooter->setTimeout(old_timeout);
	
	Progressbar::unlock();
	Statusbar::unlock();
}

void findItem(const Find direction)
{
	using Global::wFooter;
	
	Searchable *w = dynamic_cast<Searchable *>(myScreen);
	assert(w);
	assert(w->allowsSearching());
	
	Statusbar::lock();
	Statusbar::put() << "Find " << (direction == Find::Forward ? "forward" : "backward") << ": ";
	std::string findme = wFooter->getString();
	Statusbar::unlock();
	
	if (!findme.empty())
		Statusbar::print("Searching...");
	
	bool success = w->search(findme);
	
	if (findme.empty())
		return;
	
	if (success)
		Statusbar::print("Searching finished");
	else
		Statusbar::printf("Unable to find \"%1%\"", findme);
	
	if (direction == ::Find::Forward)
 		w->nextFound(Config.wrapped_search);
 	else
 		w->prevFound(Config.wrapped_search);
	
	if (myScreen == myPlaylist)
		myPlaylist->EnableHighlighting();
}

void listsChangeFinisher()
{
	if (myScreen == myLibrary
	||  myScreen == myPlaylistEditor
#	ifdef HAVE_TAGLIB_H
	||  myScreen == myTagEditor
#	endif // HAVE_TAGLIB_H
	   )
	{
		if (myScreen->activeWindow() == &myLibrary->Tags)
		{
			myLibrary->Albums.clear();
			myLibrary->Albums.refresh();
			myLibrary->Songs.clear();
			myLibrary->Songs.refresh();
			myLibrary->updateTimer();
		}
		else if (myScreen->activeWindow() == &myLibrary->Albums)
		{
			myLibrary->Songs.clear();
			myLibrary->Songs.refresh();
			myLibrary->updateTimer();
		}
		else if (myScreen->isActiveWindow(myPlaylistEditor->Playlists))
		{
			myPlaylistEditor->Content.clear();
			myPlaylistEditor->Content.refresh();
			myPlaylistEditor->updateTimer();
		}
#		ifdef HAVE_TAGLIB_H
		else if (myScreen->activeWindow() == myTagEditor->Dirs)
		{
			myTagEditor->Tags->clear();
		}
#		endif // HAVE_TAGLIB_H
	}
}

}
