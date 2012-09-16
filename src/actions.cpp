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

#include <cassert>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <iostream>

#include "actions.h"
#include "charset.h"
#include "config.h"
#include "display.h"
#include "global.h"
#include "mpdpp.h"
#include "helpers.h"
#include "statusbar.h"
#include "utility/comparators.h"

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

using namespace std::placeholders;
using Global::myScreen;

bool Action::OriginalStatusbarVisibility;
bool Action::ExitMainLoop = false;

size_t Action::HeaderHeight;
size_t Action::FooterHeight;
size_t Action::FooterStartY;

namespace {//

std::map<ActionType, Action *> Actions;

void insertAction(Action *a);
void populateActions();

}

void Action::ValidateScreenSize()
{
	using Global::MainHeight;
	
	if (COLS < 30 || MainHeight < 5)
	{
		NC::destroyScreen();
		std::cout << "Screen is too small to handle ncmpcpp correctly\n";
		exit(1);
	}
}

void Action::InitializeScreens()
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

void Action::SetResizeFlags()
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

void Action::ResizeScreen(bool reload_main_window)
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
		endwin();
		refresh();
		// get rid of KEY_RESIZE as it sometimes
		// corrupts our new cool ReadKey() function
		// because KEY_RESIZE doesn't come from stdin
		// and thus select cannot detect it
		timeout(10);
		getch();
		timeout(-1);
	}
#	endif
	
	MainHeight = LINES-(Config.new_design ? 7 : 4);
	
	ValidateScreenSize();
	
	if (!Config.header_visibility)
		MainHeight += 2;
	if (!Config.statusbar_visibility)
		++MainHeight;
	
	SetResizeFlags();
	
	applyToVisibleWindows(&BaseScreen::resize);
	
	if (Config.header_visibility || Config.new_design)
		wHeader->resize(COLS, HeaderHeight);
	
	FooterStartY = LINES-(Config.statusbar_visibility ? 2 : 1);
	wFooter->moveTo(0, FooterStartY);
	wFooter->resize(COLS, Config.statusbar_visibility ? 2 : 1);
	
	applyToVisibleWindows(&BaseScreen::refresh);
	
	Status::Changes::elapsedTime();
	if (!Mpd.isPlaying())
		Status::Changes::playerState();
	// Note: routines for drawing separator if alternative user
	// interface is active and header is hidden are placed in
	// NcmpcppStatusChanges.StatusFlags
	Status::Changes::flags();
	drawHeader();
	wFooter->refresh();
	refresh();
}

void Action::SetWindowsDimensions()
{
	using Global::MainStartY;
	using Global::MainHeight;
	
	MainStartY = Config.new_design ? 5 : 2;
	MainHeight = LINES-(Config.new_design ? 7 : 4);
	
	if (!Config.header_visibility)
	{
		MainStartY -= 2;
		MainHeight += 2;
	}
	if (!Config.statusbar_visibility)
		++MainHeight;
	
	HeaderHeight = Config.new_design ? (Config.header_visibility ? 5 : 3) : 1;
	FooterStartY = LINES-(Config.statusbar_visibility ? 2 : 1);
	FooterHeight = Config.statusbar_visibility ? 2 : 1;
}

void Action::Seek()
{
	using Global::wHeader;
	using Global::wFooter;
	using Global::Timer;
	using Global::SeekingInProgress;
	
	if (!Mpd.GetTotalTime())
	{
		Statusbar::msg("Unknown item length");
		return;
	}
	
	Progressbar::lock();
	Statusbar::lock();
	
	int songpos = Mpd.GetElapsedTime();
	timeval t = Timer;
	
	int old_timeout = wFooter->getTimeout();
	wFooter->setTimeout(500);
	
	SeekingInProgress = true;
	while (true)
	{
		Status::trace();
		myPlaylist->UpdateTimer();
		
		int howmuch = Config.incremental_seeking ? (Timer.tv_sec-t.tv_sec)/2+Config.seek_time : Config.seek_time;
		
		Key input = Key::read(*wFooter);
		auto k = Bindings.get(input);
		if (k.first == k.second || !k.first->second.isSingle()) // no single action?
			break;
		Action *a = k.first->second.action();
		if (dynamic_cast<SeekForward *>(a))
		{
			if (songpos < Mpd.GetTotalTime())
			{
				songpos += howmuch;
				if (songpos > Mpd.GetTotalTime())
					songpos = Mpd.GetTotalTime();
			}
		}
		else if (dynamic_cast<SeekBackward *>(a))
		{
			if (songpos > 0)
			{
				songpos -= howmuch;
				if (songpos < 0)
					songpos = 0;
			}
		}
		else
			break;
		
		*wFooter << NC::fmtBold;
		std::string tracklength;
		if (Config.new_design)
		{
			if (Config.display_remaining_time)
			{
				tracklength = "-";
				tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime()-songpos);
			}
			else
				tracklength = MPD::Song::ShowTime(songpos);
			tracklength += "/";
			tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime());
			*wHeader << NC::XY(0, 0) << tracklength << " ";
			wHeader->refresh();
		}
		else
		{
			tracklength = " [";
			if (Config.display_remaining_time)
			{
				tracklength += "-";
				tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime()-songpos);
			}
			else
				tracklength += MPD::Song::ShowTime(songpos);
			tracklength += "/";
			tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime());
			tracklength += "]";
			*wFooter << NC::XY(wFooter->getWidth()-tracklength.length(), 1) << tracklength;
		}
		*wFooter << NC::fmtBoldEnd;
		Progressbar::draw(songpos, Mpd.GetTotalTime());
		wFooter->refresh();
	}
	SeekingInProgress = false;
	Mpd.Seek(songpos);
	
	wFooter->setTimeout(old_timeout);
	
	Progressbar::unlock();
	Statusbar::unlock();
}

void Action::FindItem(const FindDirection fd)
{
	using Global::wFooter;
	
	Searchable *w = dynamic_cast<Searchable *>(myScreen);
	assert(w);
	assert(w->allowsSearching());
	
	Statusbar::lock();
	Statusbar::put() << "Find " << (fd == fdForward ? "forward" : "backward") << ": ";
	std::string findme = wFooter->getString();
	Statusbar::unlock();
	
	if (!findme.empty())
		Statusbar::msg("Searching...");
	
	bool success = w->search(findme);
	
	if (findme.empty())
		return;
	
	if (success)
		Statusbar::msg("Searching finished");
	else
		Statusbar::msg("Unable to find \"%s\"", findme.c_str());
	
 	if (fd == fdForward)
 		w->nextFound(Config.wrapped_search);
 	else
 		w->prevFound(Config.wrapped_search);
	
	if (myScreen == myPlaylist)
		myPlaylist->EnableHighlighting();
}

void Action::ListsChangeFinisher()
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
			myLibrary->Songs.clear();
		}
		else if (myScreen->activeWindow() == &myLibrary->Albums)
		{
			myLibrary->Songs.clear();
		}
		else if (myScreen->isActiveWindow(myPlaylistEditor->Playlists))
		{
			myPlaylistEditor->Content.clear();
		}
#		ifdef HAVE_TAGLIB_H
		else if (myScreen->activeWindow() == myTagEditor->Dirs)
		{
			myTagEditor->Tags->clear();
		}
#		endif // HAVE_TAGLIB_H
	}
}

bool Action::ConnectToMPD()
{
	if (!Mpd.Connect())
	{
		std::cout << "Couldn't connect to MPD ";
		std::cout << "(host = " << Mpd.GetHostname() << ", port = " << Mpd.GetPort() << ")";
		std::cout << ": " << Mpd.GetErrorMessage() << std::endl;
		return false;
	}
	return true;
}

bool Action::AskYesNoQuestion(const std::string &question, void (*callback)())
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << question << " [" << NC::fmtBold << 'y' << NC::fmtBoldEnd << '/' << NC::fmtBold << 'n' << NC::fmtBoldEnd << "]";
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

bool Action::isMPDMusicDirSet()
{
	if (Config.mpd_music_dir.empty())
	{
		Statusbar::msg("Proper mpd_music_dir variable has to be set in configuration file");
		return false;
	}
	return true;
}

Action *Action::Get(ActionType at)
{
	if (Actions.empty())
		populateActions();
	return Actions[at];
}

Action *Action::Get(const std::string &name)
{
	Action *result = 0;
	if (Actions.empty())
		populateActions();
	for (auto it = Actions.begin(); it != Actions.end(); ++it)
	{
		if (it->second->Name() == name)
		{
			result = it->second;
			break;
		}
	}
	return result;
}

bool MouseEvent::canBeRun() const
{
	return Config.mouse_support;
}

void MouseEvent::Run()
{
	using Global::VolumeState;
	
	itsOldMouseEvent = itsMouseEvent;
	getmouse(&itsMouseEvent);
	// workaround shitty ncurses behavior introduced in >=5.8, when we mysteriously get
	// a few times after ncmpcpp startup 2^27 code instead of BUTTON{1,3}_RELEASED. since that
	// 2^27 thing shows constantly instead of BUTTON2_PRESSED, it was redefined to be recognized
	// as BUTTON2_PRESSED. but clearly we don't want to trigger behavior bound to BUTTON2
	// after BUTTON{1,3} was pressed. so, here is the workaround: if last event was BUTTON{1,3}_PRESSED,
	// we MUST get BUTTON{1,3}_RELEASED afterwards. if we get BUTTON2_PRESSED, erroneus behavior
	// is about to occur and we need to prevent that.
	if (itsOldMouseEvent.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED) && itsMouseEvent.bstate & BUTTON2_PRESSED)
		return;
	if (itsMouseEvent.bstate & BUTTON1_PRESSED
	&&  itsMouseEvent.y == LINES-(Config.statusbar_visibility ? 2 : 1)
	   ) // progressbar
	{
		if (!Mpd.isPlaying())
			return;
		Mpd.Seek(Mpd.GetTotalTime()*itsMouseEvent.x/double(COLS));
	}
	else if (itsMouseEvent.bstate & BUTTON1_PRESSED
	     &&	 (Config.statusbar_visibility || Config.new_design)
	     &&	 Mpd.isPlaying()
	     &&	 itsMouseEvent.y == (Config.new_design ? 1 : LINES-1) && itsMouseEvent.x < 9
		) // playing/paused
	{
		Mpd.Toggle();
	}
	else if ((itsMouseEvent.bstate & BUTTON2_PRESSED || itsMouseEvent.bstate & BUTTON4_PRESSED)
	     &&	 Config.header_visibility
	     &&	 itsMouseEvent.y == 0 && size_t(itsMouseEvent.x) > COLS-VolumeState.length()
	) // volume
	{
		if (itsMouseEvent.bstate & BUTTON2_PRESSED)
			Mpd.SetVolume(Mpd.GetVolume()-2);
		else
			Mpd.SetVolume(Mpd.GetVolume()+2);
	}
	else if (itsMouseEvent.bstate & (BUTTON1_PRESSED | BUTTON2_PRESSED | BUTTON3_PRESSED | BUTTON4_PRESSED))
		myScreen->mouseButtonPressed(itsMouseEvent);
}

void ScrollUp::Run()
{
	myScreen->scroll(NC::wUp);
	ListsChangeFinisher();
}

void ScrollDown::Run()
{
	myScreen->scroll(NC::wDown);
	ListsChangeFinisher();
}

bool ScrollUpArtist::canBeRun() const
{
	return proxySongList(myScreen).get();
}

void ScrollUpArtist::Run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	size_t pos = pl->choice();
	if (MPD::Song *s = pl->getSong(pos))
	{
		std::string artist = s->getArtist();
		while (pos > 0)
		{
			s = pl->getSong(--pos);
			if (!s || s->getArtist() != artist)
				break;
		}
		pl->highlight(pos);
	}
}

bool ScrollUpAlbum::canBeRun() const
{
	return proxySongList(myScreen).get();
}

void ScrollUpAlbum::Run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	size_t pos = pl->choice();
	if (MPD::Song *s = pl->getSong(pos))
	{
		std::string album = s->getAlbum();
		while (pos > 0)
		{
			s = pl->getSong(--pos);
			if (!s || s->getAlbum() != album)
				break;
		}
		pl->highlight(pos);
	}
}

bool ScrollDownArtist::canBeRun() const
{
	return proxySongList(myScreen).get();
}

void ScrollDownArtist::Run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	size_t pos = pl->choice();
	if (MPD::Song *s = pl->getSong(pos))
	{
		std::string artist = s->getArtist();
		while (pos < pl->size() - 1)
		{
			s = pl->getSong(++pos);
			if (!s || s->getArtist() != artist)
				break;
		}
		pl->highlight(pos);
	}
}

bool ScrollDownAlbum::canBeRun() const
{
	return proxySongList(myScreen).get();
}

void ScrollDownAlbum::Run()
{
	auto pl = proxySongList(myScreen);
	assert(pl);
	size_t pos = pl->choice();
	if (MPD::Song *s = pl->getSong(pos))
	{
		std::string album = s->getAlbum();
		while (pos < pl->size() - 1)
		{
			s = pl->getSong(++pos);
			if (!s || s->getAlbum() != album)
				break;
		}
		pl->highlight(pos);
	}
}

void PageUp::Run()
{
	myScreen->scroll(NC::wPageUp);
	ListsChangeFinisher();
}

void PageDown::Run()
{
	myScreen->scroll(NC::wPageDown);
	ListsChangeFinisher();
}

void MoveHome::Run()
{
	myScreen->scroll(NC::wHome);
	ListsChangeFinisher();
}

void MoveEnd::Run()
{
	myScreen->scroll(NC::wEnd);
	ListsChangeFinisher();
}

void ToggleInterface::Run()
{
	Config.new_design = !Config.new_design;
	Config.statusbar_visibility = Config.new_design ? 0 : OriginalStatusbarVisibility;
	SetWindowsDimensions();
	Progressbar::unlock();
	Statusbar::unlock();
	ResizeScreen(false);
	if (!Mpd.isPlaying())
		Status::Changes::mixer();
	Statusbar::msg("User interface: %s", Config.new_design ? "Alternative" : "Classic");
}

bool JumpToParentDirectory::canBeRun() const
{
	return (myScreen == myBrowser)
#	ifdef HAVE_TAGLIB_H
	    || (myScreen->activeWindow() == myTagEditor->Dirs)
#	endif // HAVE_TAGLIB_H
	;
}

void JumpToParentDirectory::Run()
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

void PressEnter::Run()
{
	myScreen->enterPressed();
}

void PressSpace::Run()
{
	myScreen->spacePressed();
}

bool PreviousColumn::canBeRun() const
{
	auto hc = hasColumns(myScreen);
	return hc && hc->previousColumnAvailable();
}

void PreviousColumn::Run()
{
	hasColumns(myScreen)->previousColumn();
}

bool NextColumn::canBeRun() const
{
	auto hc = hasColumns(myScreen);
	return hc && hc->nextColumnAvailable();
}

void NextColumn::Run()
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

void MasterScreen::Run()
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

void SlaveScreen::Run()
{
	using Global::myInactiveScreen;
	using Global::myLockedScreen;
	
	myScreen = myInactiveScreen;
	myInactiveScreen = myLockedScreen;
	drawHeader();
}

void VolumeUp::Run()
{
	Mpd.SetVolume(Mpd.GetVolume()+1);
}

void VolumeDown::Run()
{
	Mpd.SetVolume(Mpd.GetVolume()-1);
}

void Delete::Run()
{
	if (myScreen == myPlaylist && !myPlaylist->main().empty())
	{
		Statusbar::msg("Deleting items...");
		auto delete_fun = std::bind(&MPD::Connection::Delete, _1, _2);
		if (deleteSelectedSongs(myPlaylist->main(), delete_fun))
			Statusbar::msg("Item(s) deleted");
	}
#	ifndef WIN32
	else if (myScreen == myBrowser && !myBrowser->main().empty())
	{
		if (!myBrowser->isLocal() && !isMPDMusicDirSet())
			return;
		
		std::string question;
		if (myBrowser->main().hasSelected())
			question = "Delete selected items?";
		else
		{
			MPD::Item &item = myBrowser->main().current().value();
			std::string name = item.type == MPD::itSong ? item.song->getName() : item.name;
			question = "Delete ";
			question += itemTypeToString(item.type);
			question += " \"";
			question += ToString(wideShorten(ToWString(name), COLS-question.size()-10));
			question += "\"?";
		}
		bool yes = AskYesNoQuestion(question, Status::trace);
		if (yes)
		{
			bool success = true;
			auto list = getSelectedOrCurrent(myBrowser->main().begin(), myBrowser->main().end(), myBrowser->main().currentI());
			for (auto it = list.begin(); it != list.end(); ++it)
			{
				const MPD::Item &i = (*it)->value();
				std::string name = i.type == MPD::itSong ? i.song->getName() : i.name;
				if (myBrowser->deleteItem(i))
				{
					const char msg[] = "\"%ls\" deleted";
					Statusbar::msg(msg, wideShorten(ToWString(name), COLS-const_strlen(msg)).c_str());
				}
				else
				{
					const char msg[] = "Couldn't delete \"%ls\": %s";
					Statusbar::msg(msg, wideShorten(ToWString(name), COLS-const_strlen(msg)-25).c_str(), strerror(errno));
					success = false;
					break;
				}
			}
			if (success)
			{
				if (!myBrowser->isLocal())
					Mpd.UpdateDirectory(myBrowser->CurrentDir());
			}
		}
		else
			Statusbar::msg("Aborted");
	}
#	endif // !WIN32
	else if (myScreen == myPlaylistEditor && !myPlaylistEditor->Content.empty())
	{
		if (myScreen->isActiveWindow(myPlaylistEditor->Playlists))
		{
			std::string question;
			if (myPlaylistEditor->Playlists.hasSelected())
				question = "Delete selected playlists?";
			else
			{
				question = "Delete playlist \"";
				question += ToString(wideShorten(ToWString(myPlaylistEditor->Playlists.current().value()), COLS-question.size()-10));
				question += "\"?";
			}
			bool yes = AskYesNoQuestion(question, Status::trace);
			if (yes)
			{
				auto list = getSelectedOrCurrent(myPlaylistEditor->Playlists.begin(), myPlaylistEditor->Playlists.end(), myPlaylistEditor->Playlists.currentI());
				Mpd.StartCommandsList();
				for (auto it = list.begin(); it != list.end(); ++it)
					Mpd.DeletePlaylist((*it)->value());
				if (Mpd.CommitCommandsList())
					Statusbar::msg("Playlist%s deleted", list.size() == 1 ? "" : "s");
			}
			else
				Statusbar::msg("Aborted");
		}
		else if (myScreen->isActiveWindow(myPlaylistEditor->Content))
		{
			std::string playlist = myPlaylistEditor->Playlists.current().value();
			auto delete_fun = std::bind(&MPD::Connection::PlaylistDelete, _1, playlist, _2);
			Statusbar::msg("Deleting items...");
			if (deleteSelectedSongs(myPlaylistEditor->Content, delete_fun))
				Statusbar::msg("Item(s) deleted");
		}
	}
}

void ReplaySong::Run()
{
	if (Mpd.isPlaying())
		Mpd.Seek(0);
}

void PreviousSong::Run()
{
	Mpd.Prev();
}

void NextSong::Run()
{
	Mpd.Next();
}

void Pause::Run()
{
	Mpd.Toggle();
}

void SavePlaylist::Run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Save playlist as: ";
	std::string playlist_name = wFooter->getString();
	Statusbar::unlock();
	if (playlist_name.find("/") != std::string::npos)
	{
		Statusbar::msg("Playlist name must not contain slashes");
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
			if (Mpd.GetErrorMessage().empty())
				Statusbar::msg("Filtered items added to playlist \"%s\"", playlist_name.c_str());
		}
		else
		{
			int result = Mpd.SavePlaylist(playlist_name);
			if (result == MPD_ERROR_SUCCESS)
			{
				Statusbar::msg("Playlist saved as \"%s\"", playlist_name.c_str());
			}
			else if (result == MPD_SERVER_ERROR_EXIST)
			{
				bool yes = AskYesNoQuestion("Playlist \"" + playlist_name + "\" already exists, overwrite?", Status::trace);
				if (yes)
				{
					Mpd.DeletePlaylist(playlist_name);
					if (Mpd.SavePlaylist(playlist_name) == MPD_ERROR_SUCCESS)
						Statusbar::msg("Playlist overwritten");
				}
				else
					Statusbar::msg("Aborted");
				if (myScreen == myPlaylist)
					myPlaylist->EnableHighlighting();
			}
		}
	}
	if (!myBrowser->isLocal()
	&&  myBrowser->CurrentDir() == "/"
	&&  !myBrowser->main().empty())
		myBrowser->GetDirectory(myBrowser->CurrentDir());
}

void Stop::Run()
{
	Mpd.Stop();
}

bool MoveSortOrderUp::canBeRun() const
{
	return myScreen == mySortPlaylistDialog;
}

void MoveSortOrderUp::Run()
{
	mySortPlaylistDialog->moveSortOrderUp();
}

bool MoveSortOrderDown::canBeRun() const
{
	return myScreen == mySortPlaylistDialog;
}

void MoveSortOrderDown::Run()
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

void MoveSelectedItemsUp::Run()
{
	if (myScreen == myPlaylist)
	{
		moveSelectedItemsUp(myPlaylist->main(), std::bind(&MPD::Connection::Move, _1, _2, _3));
	}
	else if (myScreen == myPlaylistEditor)
	{
		assert(!myPlaylistEditor->Playlists.empty());
		std::string playlist = myPlaylistEditor->Playlists.current().value();
		auto move_fun = std::bind(&MPD::Connection::PlaylistMove, _1, playlist, _2, _3);
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

void MoveSelectedItemsDown::Run()
{
	if (myScreen == myPlaylist)
	{
		moveSelectedItemsDown(myPlaylist->main(), std::bind(&MPD::Connection::Move, _1, _2, _3));
	}
	else if (myScreen == myPlaylistEditor)
	{
		assert(!myPlaylistEditor->Playlists.empty());
		std::string playlist = myPlaylistEditor->Playlists.current().value();
		auto move_fun = std::bind(&MPD::Connection::PlaylistMove, _1, playlist, _2, _3);
		moveSelectedItemsDown(myPlaylistEditor->Content, move_fun);
	}
}

bool MoveSelectedItemsTo::canBeRun() const
{
	return myScreen == myPlaylist
	    || myScreen->isActiveWindow(myPlaylistEditor->Content);
}

void MoveSelectedItemsTo::Run()
{
	if (myScreen == myPlaylist)
		moveSelectedItemsTo(myPlaylist->main(), std::bind(&MPD::Connection::Move, _1, _2, _3));
	else
	{
		assert(!myPlaylistEditor->Playlists.empty());
		std::string playlist = myPlaylistEditor->Playlists.current().value();
		auto move_fun = std::bind(&MPD::Connection::PlaylistMove, _1, playlist, _2, _3);
		moveSelectedItemsTo(myPlaylistEditor->Content, move_fun);
	}
}

bool Add::canBeRun() const
{
	return myScreen != myPlaylistEditor
	   || !myPlaylistEditor->Playlists.empty();
}

void Add::Run()
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
	return Mpd.isPlaying() && Mpd.GetTotalTime() > 0;
}

void SeekForward::Run()
{
	Seek();
}

bool SeekBackward::canBeRun() const
{
	return Mpd.isPlaying() && Mpd.GetTotalTime() > 0;
}

void SeekBackward::Run()
{
	Seek();
}

bool ToggleDisplayMode::canBeRun() const
{
	return myScreen == myPlaylist
	    || myScreen == myBrowser
	    || myScreen == mySearcher
	    || myScreen->isActiveWindow(myPlaylistEditor->Content);
}

void ToggleDisplayMode::Run()
{
	if (myScreen == myPlaylist)
	{
		Config.columns_in_playlist = !Config.columns_in_playlist;
		Statusbar::msg("Playlist display mode: %s", Config.columns_in_playlist ? "Columns" : "Classic");
		
		if (Config.columns_in_playlist)
		{
			myPlaylist->main().setItemDisplayer(std::bind(Display::SongsInColumns, _1, myPlaylist));
			if (Config.titles_visibility)
				myPlaylist->main().setTitle(Display::Columns(myPlaylist->main().getWidth()));
			else
				myPlaylist->main().setTitle("");
		}
		else
		{
			myPlaylist->main().setItemDisplayer(std::bind(Display::Songs, _1, myPlaylist, Config.song_list_format));
			myPlaylist->main().setTitle("");
		}
	}
	else if (myScreen == myBrowser)
	{
		Config.columns_in_browser = !Config.columns_in_browser;
		Statusbar::msg("Browser display mode: %s", Config.columns_in_browser ? "Columns" : "Classic");
		myBrowser->main().setTitle(Config.columns_in_browser && Config.titles_visibility ? Display::Columns(myBrowser->main().getWidth()) : "");
	}
	else if (myScreen == mySearcher)
	{
		Config.columns_in_search_engine = !Config.columns_in_search_engine;
		Statusbar::msg("Search engine display mode: %s", Config.columns_in_search_engine ? "Columns" : "Classic");
		if (mySearcher->main().size() > SearchEngine::StaticOptions)
			mySearcher->main().setTitle(Config.columns_in_search_engine && Config.titles_visibility ? Display::Columns(mySearcher->main().getWidth()) : "");
	}
	else if (myScreen->isActiveWindow(myPlaylistEditor->Content))
	{
		Config.columns_in_playlist_editor = !Config.columns_in_playlist_editor;
		Statusbar::msg("Playlist editor display mode: %s", Config.columns_in_playlist_editor ? "Columns" : "Classic");
		if (Config.columns_in_playlist_editor)
			myPlaylistEditor->Content.setItemDisplayer(std::bind(Display::SongsInColumns, _1, myPlaylistEditor));
		else
			myPlaylistEditor->Content.setItemDisplayer(std::bind(Display::Songs, _1, myPlaylistEditor, Config.song_list_format));
	}
}

bool ToggleSeparatorsBetweenAlbums::canBeRun() const
{
	return true;
}

void ToggleSeparatorsBetweenAlbums::Run()
{
	Config.playlist_separate_albums = !Config.playlist_separate_albums;
	Statusbar::msg("Separators between albums: %s", Config.playlist_separate_albums ? "On" : "Off");
}

#ifndef HAVE_CURL_CURL_H
bool ToggleLyricsFetcher::canBeRun() const
{
	return false;
}
#endif // NOT HAVE_CURL_CURL_H

void ToggleLyricsFetcher::Run()
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

void ToggleFetchingLyricsInBackground::Run()
{
#	ifdef HAVE_CURL_CURL_H
	Config.fetch_lyrics_in_background = !Config.fetch_lyrics_in_background;
	Statusbar::msg("Fetching lyrics for playing songs in background: %s", Config.fetch_lyrics_in_background ? "On" : "Off");
#	endif // HAVE_CURL_CURL_H
}

void TogglePlayingSongCentering::Run()
{
	Config.autocenter_mode = !Config.autocenter_mode;
	Statusbar::msg("Centering playing song: %s", Config.autocenter_mode ? "On" : "Off");
	if (Config.autocenter_mode && Mpd.isPlaying() && !myPlaylist->main().isFiltered())
		myPlaylist->main().highlight(Mpd.GetCurrentlyPlayingSongPos());
}

void UpdateDatabase::Run()
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
	  &&   Mpd.isPlaying();
}

void JumpToPlayingSong::Run()
{
	if (myScreen == myPlaylist)
		myPlaylist->main().highlight(Mpd.GetCurrentlyPlayingSongPos());
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

void ToggleRepeat::Run()
{
	Mpd.SetRepeat(!Mpd.GetRepeat());
}

void Shuffle::Run()
{
	Mpd.Shuffle();
}

void ToggleRandom::Run()
{
	Mpd.SetRandom(!Mpd.GetRandom());
}

bool StartSearching::canBeRun() const
{
	return myScreen == mySearcher && !mySearcher->main()[0].isInactive();
}

void StartSearching::Run()
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

void SaveTagChanges::Run()
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

void ToggleSingle::Run()
{
	Mpd.SetSingle(!Mpd.GetSingle());
}

void ToggleConsume::Run()
{
	Mpd.SetConsume(!Mpd.GetConsume());
}

void ToggleCrossfade::Run()
{
	Mpd.SetCrossfade(Mpd.GetCrossfade() ? 0 : Config.crossfade_time);
}

void SetCrossfade::Run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Set crossfade to: ";
	std::string crossfade = wFooter->getString(3);
	Statusbar::unlock();
	int cf = stringToInt(crossfade);
	if (cf > 0)
	{
		Config.crossfade_time = cf;
		Mpd.SetCrossfade(cf);
	}
}

bool EditSong::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return isMPDMusicDirSet() && currentSong(myScreen);
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditSong::Run()
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
	return isMPDMusicDirSet()
	   &&  myScreen->isActiveWindow(myLibrary->Tags)
	   && !myLibrary->Tags.empty();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryTag::Run()
{
#	ifdef HAVE_TAGLIB_H
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << NC::fmtBold << tagTypeToString(Config.media_lib_primary_tag) << NC::fmtBoldEnd << ": ";
	std::string new_tag = wFooter->getString(myLibrary->Tags.current().value());
	Statusbar::unlock();
	if (!new_tag.empty() && new_tag != myLibrary->Tags.current().value())
	{
		Statusbar::msg("Updating tags...");
		Mpd.StartSearch(1);
		Mpd.AddSearch(Config.media_lib_primary_tag, myLibrary->Tags.current().value());
		MPD::MutableSong::SetFunction set = tagTypeToSetFunction(Config.media_lib_primary_tag);
		assert(set);
		bool success = true;
		MPD::SongList songs = Mpd.CommitSearchSongs();
		for (auto s = songs.begin(); s != songs.end(); ++s)
		{
			MPD::MutableSong es = *s;
			es.setTags(set, new_tag, Config.tags_separator);
			Statusbar::msg("Updating tags in \"%s\"...", es.getName().c_str());
			std::string path = Config.mpd_music_dir + es.getURI();
			if (!Tags::write(es))
			{
				const char msg[] = "Error while updating tags in \"%ls\"";
				Statusbar::msg(msg, wideShorten(ToWString(es.getURI()), COLS-const_strlen(msg)).c_str());
				success = false;
				break;
			}
		}
		if (success)
		{
			Mpd.UpdateDirectory(getSharedDirectory(songs.begin(), songs.end()));
			Statusbar::msg("Tags updated successfully");
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryAlbum::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return isMPDMusicDirSet()
	    && myScreen->isActiveWindow(myLibrary->Albums)
	    && !myLibrary->Albums.empty();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryAlbum::Run()
{
#	ifdef HAVE_TAGLIB_H
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << NC::fmtBold << "Album: " << NC::fmtBoldEnd;
	std::string new_album = wFooter->getString(myLibrary->Albums.current().value().Album);
	Statusbar::unlock();
	if (!new_album.empty() && new_album != myLibrary->Albums.current().value().Album)
	{
		bool success = 1;
		Statusbar::msg("Updating tags...");
		for (size_t i = 0;  i < myLibrary->Songs.size(); ++i)
		{
			Statusbar::msg("Updating tags in \"%s\"...", myLibrary->Songs[i].value().getName().c_str());
			std::string path = Config.mpd_music_dir + myLibrary->Songs[i].value().getURI();
			TagLib::FileRef f(path.c_str());
			if (f.isNull())
			{
				const char msg[] = "Error while opening file \"%ls\"";
				Statusbar::msg(msg, wideShorten(ToWString(myLibrary->Songs[i].value().getURI()), COLS-const_strlen(msg)).c_str());
				success = 0;
				break;
			}
			f.tag()->setAlbum(ToWString(new_album));
			if (!f.save())
			{
				const char msg[] = "Error while writing tags in \"%ls\"";
				Statusbar::msg(msg, wideShorten(ToWString(myLibrary->Songs[i].value().getURI()), COLS-const_strlen(msg)).c_str());
				success = 0;
				break;
			}
		}
		if (success)
		{
			Mpd.UpdateDirectory(getSharedDirectory(myLibrary->Songs.beginV(), myLibrary->Songs.endV()));
			Statusbar::msg("Tags updated successfully");
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditDirectoryName::canBeRun() const
{
	return   isMPDMusicDirSet()
	  &&     ((myScreen == myBrowser
	      && !myBrowser->main().empty()
		  && myBrowser->main().current().value().type == MPD::itDirectory)
#	ifdef HAVE_TAGLIB_H
	    ||   (myScreen->activeWindow() == myTagEditor->Dirs
	      && !myTagEditor->Dirs->empty()
	      && myTagEditor->Dirs->choice() > 0)
#	endif // HAVE_TAGLIB_H
	);
}

void EditDirectoryName::Run()
{
	using Global::wFooter;
	
	if (myScreen == myBrowser)
	{
		std::string old_dir = myBrowser->main().current().value().name;
		Statusbar::lock();
		Statusbar::put() << NC::fmtBold << "Directory: " << NC::fmtBoldEnd;
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
				const char msg[] = "Directory renamed to \"%ls\"";
				Statusbar::msg(msg, wideShorten(ToWString(new_dir), COLS-const_strlen(msg)).c_str());
				if (!myBrowser->isLocal())
					Mpd.UpdateDirectory(getSharedDirectory(old_dir, new_dir));
				myBrowser->GetDirectory(myBrowser->CurrentDir());
			}
			else
			{
				const char msg[] = "Couldn't rename \"%ls\": %s";
				Statusbar::msg(msg, wideShorten(ToWString(old_dir), COLS-const_strlen(msg)-25).c_str(), strerror(errno));
			}
		}
	}
#	ifdef HAVE_TAGLIB_H
	else if (myScreen->activeWindow() == myTagEditor->Dirs)
	{
		std::string old_dir = myTagEditor->Dirs->current().value().first;
		Statusbar::lock();
		Statusbar::put() << NC::fmtBold << "Directory: " << NC::fmtBoldEnd;
		std::string new_dir = wFooter->getString(old_dir);
		Statusbar::unlock();
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + old_dir;
			std::string full_new_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + new_dir;
			if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
			{
				const char msg[] = "Directory renamed to \"%ls\"";
				Statusbar::msg(msg, wideShorten(ToWString(new_dir), COLS-const_strlen(msg)).c_str());
				Mpd.UpdateDirectory(myTagEditor->CurrentDir());
			}
			else
			{
				const char msg[] = "Couldn't rename \"%ls\": %s";
				Statusbar::msg(msg, wideShorten(ToWString(old_dir), COLS-const_strlen(msg)-25).c_str(), strerror(errno));
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

void EditPlaylistName::Run()
{
	using Global::wFooter;
	
	std::string old_name;
	if (myScreen->isActiveWindow(myPlaylistEditor->Playlists))
		old_name = myPlaylistEditor->Playlists.current().value();
	else
		old_name = myBrowser->main().current().value().name;
	Statusbar::lock();
	Statusbar::put() << NC::fmtBold << "Playlist: " << NC::fmtBoldEnd;
	std::string new_name = wFooter->getString(old_name);
	Statusbar::unlock();
	if (!new_name.empty() && new_name != old_name)
	{
		if (Mpd.Rename(old_name, new_name))
		{
			const char msg[] = "Playlist renamed to \"%ls\"";
			Statusbar::msg(msg, wideShorten(ToWString(new_name), COLS-const_strlen(msg)).c_str());
			if (!myBrowser->isLocal())
				myBrowser->GetDirectory("/");
		}
	}
}

bool EditLyrics::canBeRun() const
{
	return myScreen == myLyrics;
}

void EditLyrics::Run()
{
	myLyrics->Edit();
}

bool JumpToBrowser::canBeRun() const
{
	return currentSong(myScreen);
}

void JumpToBrowser::Run()
{
	auto s = currentSong(myScreen);
	myBrowser->LocateSong(*s);
}

bool JumpToMediaLibrary::canBeRun() const
{
	return currentSong(myScreen);
}

void JumpToMediaLibrary::Run()
{
	auto s = currentSong(myScreen);
	myLibrary->LocateSong(*s);
}

bool JumpToPlaylistEditor::canBeRun() const
{
	return myScreen == myBrowser
	    && myBrowser->main().current().value().type == MPD::itPlaylist;
}

void JumpToPlaylistEditor::Run()
{
	myPlaylistEditor->Locate(myBrowser->main().current().value().name);
}

void ToggleScreenLock::Run()
{
	using Global::wFooter;
	using Global::myLockedScreen;
	
	if (myLockedScreen != 0)
	{
		BaseScreen::unlock();
		Action::SetResizeFlags();
		myScreen->resize();
		Statusbar::msg("Screen unlocked");
	}
	else
	{
		int part = Config.locked_screen_width_part*100;
		if (Config.ask_for_locked_screen_width_part)
		{
			Statusbar::lock();
			Statusbar::put() << "% of the locked screen's width to be reserved (20-80): ";
			std::string str_part = wFooter->getString(intTo<std::string>::apply(Config.locked_screen_width_part*100));
			Statusbar::unlock();
			if (str_part.empty())
				return;
			part = stringToInt(str_part);
		}
		if (part < 20 || part > 80)
		{
			Statusbar::msg("Number is out of range");
			return;
		}
		Config.locked_screen_width_part = part/100.0;
		if (myScreen->lock())
			Statusbar::msg("Screen locked (with %d%% width)", part);
		else
			Statusbar::msg("Current screen can't be locked");
	}
}

bool JumpToTagEditor::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return isMPDMusicDirSet() && currentSong(myScreen);
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void JumpToTagEditor::Run()
{
#	ifdef HAVE_TAGLIB_H
	auto s = currentSong(myScreen);
	myTagEditor->LocateSong(*s);
#	endif // HAVE_TAGLIB_H
}

bool JumpToPositionInSong::canBeRun() const
{
	return Mpd.isPlaying() && Mpd.GetTotalTime() > 0;
}

void JumpToPositionInSong::Run()
{
	using Global::wFooter;
	
	const MPD::Song s = myPlaylist->nowPlayingSong();
	
	Statusbar::lock();
	Statusbar::put() << "Position to go (in %/mm:ss/seconds(s)): ";
	std::string position = wFooter->getString();
	Statusbar::unlock();
	
	if (position.empty())
		return;
	
	int newpos = 0;
	if (position.find(':') != std::string::npos) // probably time in mm:ss
	{
		newpos = stringToInt(position)*60 + stringToInt(position.substr(position.find(':')+1));
		if (newpos >= 0 && newpos <= Mpd.GetTotalTime())
			Mpd.Seek(newpos);
		else
			Statusbar::msg("Out of bounds, 0:00-%s possible for mm:ss, %s given", s.getLength().c_str(), MPD::Song::ShowTime(newpos).c_str());
	}
	else if (position.find('s') != std::string::npos) // probably position in seconds
	{
		newpos = stringToInt(position);
		if (newpos >= 0 && newpos <= Mpd.GetTotalTime())
			Mpd.Seek(newpos);
		else
			Statusbar::msg("Out of bounds, 0-%d possible for seconds, %d given", s.getDuration(), newpos);
	}
	else
	{
		newpos = stringToInt(position);
		if (newpos >= 0 && newpos <= 100)
			Mpd.Seek(Mpd.GetTotalTime()*newpos/100.0);
		else
			Statusbar::msg("Out of bounds, 0-100 possible for %%, %d given", newpos);
	}
}

bool ReverseSelection::canBeRun() const
{
	auto w = hasSongs(myScreen);
	return w && w->allowsSelection();
}

void ReverseSelection::Run()
{
	auto w = hasSongs(myScreen);
	w->reverseSelection();
	Statusbar::msg("Selection reversed");
}

bool RemoveSelection::canBeRun() const
{
	return proxySongList(myScreen).get();
}

void RemoveSelection::Run()
{
	auto pl = proxySongList(myScreen);
	for (size_t i = 0; i < pl->size(); ++i)
		pl->setSelected(i, false);
	Statusbar::msg("Selection removed");
}

bool SelectAlbum::canBeRun() const
{
	auto w = hasSongs(myScreen);
	return w && w->allowsSelection() && w->getProxySongList().get();
}

void SelectAlbum::Run()
{
	auto pl = proxySongList(myScreen);
	size_t pos = pl->choice();
	if (MPD::Song *s = pl->getSong(pos))
	{
		std::string album = s->getAlbum();
		// select song under cursor
		pl->setSelected(pos, true);
		// go up
		while (pos > 0)
		{
			s = pl->getSong(--pos);
			if (!s || s->getAlbum() != album)
				break;
			else
				pl->setSelected(pos, true);
		}
		// go down
		pos = pl->choice();
		while (pos < pl->size() - 1)
		{
			s = pl->getSong(++pos);
			if (!s || s->getAlbum() != album)
				break;
			else
				pl->setSelected(pos, true);
		}
		Statusbar::msg("Album around cursor position selected");
	}
}

bool AddSelectedItems::canBeRun() const
{
	return myScreen != mySelectedItemsAdder;
}

void AddSelectedItems::Run()
{
	mySelectedItemsAdder->switchTo();
}

void CropMainPlaylist::Run()
{
	bool yes = true;
	if (Config.ask_before_clearing_main_playlist)
		yes = AskYesNoQuestion("Do you really want to crop main playlist?", Status::trace);
	if (yes)
	{
		Statusbar::msg("Cropping playlist...");
		if (cropPlaylist(myPlaylist->main(), std::bind(&MPD::Connection::Delete, _1, _2)))
			Statusbar::msg("Cropping playlist...");
	}
}

bool CropPlaylist::canBeRun() const
{
	return myScreen == myPlaylistEditor;
}

void CropPlaylist::Run()
{
	assert(!myPlaylistEditor->Playlists.empty());
	std::string playlist = myPlaylistEditor->Playlists.current().value();
	bool yes = true;
	if (Config.ask_before_clearing_main_playlist)
		yes = AskYesNoQuestion("Do you really want to crop playlist \"" + playlist + "\"?", Status::trace);
	if (yes)
	{
		auto delete_fun = std::bind(&MPD::Connection::PlaylistDelete, _1, playlist, _2);
		Statusbar::msg("Cropping playlist \"%s\"...", playlist.c_str());
		if (cropPlaylist(myPlaylistEditor->Content, delete_fun))
			Statusbar::msg("Playlist \"%s\" cropped", playlist.c_str());
	}
}

void ClearMainPlaylist::Run()
{
	bool yes = true;
	if (Config.ask_before_clearing_main_playlist)
		yes = AskYesNoQuestion("Do you really want to clear main playlist?", Status::trace);
	if (yes)
	{
		auto delete_fun = std::bind(&MPD::Connection::Delete, _1, _2);
		auto clear_fun = std::bind(&MPD::Connection::ClearMainPlaylist, _1);
		Statusbar::msg("Deleting items...");
		if (clearPlaylist(myPlaylist->main(), delete_fun, clear_fun))
			Statusbar::msg("Items deleted");
	}
}

bool ClearPlaylist::canBeRun() const
{
	return myScreen == myPlaylistEditor;
}

void ClearPlaylist::Run()
{
	assert(!myPlaylistEditor->Playlists.empty());
	std::string playlist = myPlaylistEditor->Playlists.current().value();
	bool yes = true;
	if (Config.ask_before_clearing_main_playlist)
		yes = AskYesNoQuestion("Do you really want to clear playlist \"" + playlist + "\"?", Status::trace);
	if (yes)
	{
		auto delete_fun = std::bind(&MPD::Connection::PlaylistDelete, _1, playlist, _2);
		auto clear_fun = std::bind(&MPD::Connection::ClearPlaylist, _1, playlist);
		Statusbar::msg("Deleting items from \"%s\"...", playlist.c_str());
		if (clearPlaylist(myPlaylistEditor->Content, delete_fun, clear_fun))
			Statusbar::msg("Items deleted from \"%s\"", playlist.c_str());
	}
}

bool SortPlaylist::canBeRun() const
{
	return myScreen == myPlaylist;
}

void SortPlaylist::Run()
{
	mySortPlaylistDialog->switchTo();
}

bool ReversePlaylist::canBeRun() const
{
	return myScreen == myPlaylist;
}

void ReversePlaylist::Run()
{
	myPlaylist->Reverse();
}

bool ApplyFilter::canBeRun() const
{
	auto w = dynamic_cast<Filterable *>(myScreen);
	return w && w->allowsFiltering();
}

void ApplyFilter::Run()
{
	using Global::wFooter;
	
	Filterable *f = dynamic_cast<Filterable *>(myScreen);
	std::string filter = f->currentFilter();
	
	Statusbar::lock();
	Statusbar::put() << NC::fmtBold << "Apply filter: " << NC::fmtBoldEnd;
	wFooter->setGetStringHelper(Statusbar::Helpers::ApplyFilterImmediately(f, ToWString(filter)));
	wFooter->getString(filter);
	wFooter->setGetStringHelper(Statusbar::Helpers::getString);
	Statusbar::unlock();
	
	filter = f->currentFilter();
 	if (filter.empty())
	{
		myPlaylist->main().clearFilterResults();
		Statusbar::msg("Filtering disabled");
	}
 	else
		Statusbar::msg("Using filter \"%s\"", filter.c_str());
	
	if (myScreen == myPlaylist)
	{
		myPlaylist->EnableHighlighting();
		Playlist::ReloadTotalLength = true;
		drawHeader();
	}
	ListsChangeFinisher();
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

void Find::Run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Find: ";
	std::string findme = wFooter->getString();
	Statusbar::unlock();
	
	Statusbar::msg("Searching...");
	auto s = static_cast<Screen<NC::Scrollpad> *>(myScreen);
	s->main().removeFormatting();
	Statusbar::msg("%s", findme.empty() || s->main().setFormatting(NC::fmtReverse, ToWString(findme), NC::fmtReverseEnd, 0) ? "Done" : "No matching patterns found");
	s->main().flush();
}

bool FindItemBackward::canBeRun() const
{
	auto w = dynamic_cast<Searchable *>(myScreen);
	return w && w->allowsSearching();
}

void FindItemForward::Run()
{
	FindItem(fdForward);
	ListsChangeFinisher();
}

bool FindItemForward::canBeRun() const
{
	auto w = dynamic_cast<Searchable *>(myScreen);
	return w && w->allowsSearching();
}

void FindItemBackward::Run()
{
	FindItem(fdBackward);
	ListsChangeFinisher();
}

bool NextFoundItem::canBeRun() const
{
	return dynamic_cast<Searchable *>(myScreen);
}

void NextFoundItem::Run()
{
	Searchable *w = dynamic_cast<Searchable *>(myScreen);
	w->nextFound(Config.wrapped_search);
	ListsChangeFinisher();
}

bool PreviousFoundItem::canBeRun() const
{
	return dynamic_cast<Searchable *>(myScreen);
}

void PreviousFoundItem::Run()
{
	Searchable *w = dynamic_cast<Searchable *>(myScreen);
	w->prevFound(Config.wrapped_search);
	ListsChangeFinisher();
}

void ToggleFindMode::Run()
{
	Config.wrapped_search = !Config.wrapped_search;
	Statusbar::msg("Search mode: %s", Config.wrapped_search ? "Wrapped" : "Normal");
}

void ToggleReplayGainMode::Run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Replay gain mode? [" << NC::fmtBold << 'o' << NC::fmtBoldEnd << "ff/" << NC::fmtBold << 't' << NC::fmtBoldEnd << "rack/" << NC::fmtBold << 'a' << NC::fmtBoldEnd << "lbum]";
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
	Statusbar::msg("Replay gain mode: %s", Mpd.GetReplayGainMode().c_str());
}

void ToggleSpaceMode::Run()
{
	Config.space_selects = !Config.space_selects;
	Statusbar::msg("Space mode: %s item", Config.space_selects ? "Select" : "Add");
}

void ToggleAddMode::Run()
{
	Config.ncmpc_like_songs_adding = !Config.ncmpc_like_songs_adding;
	Statusbar::msg("Add mode: %s", Config.ncmpc_like_songs_adding ? "Add item to playlist, remove if already added" : "Always add item to playlist");
}

void ToggleMouse::Run()
{
	Config.mouse_support = !Config.mouse_support;
	mousemask(Config.mouse_support ? ALL_MOUSE_EVENTS : 0, 0);
	Statusbar::msg("Mouse support %s", Config.mouse_support ? "enabled" : "disabled");
}

void ToggleBitrateVisibility::Run()
{
	Config.display_bitrate = !Config.display_bitrate;
	Statusbar::msg("Bitrate visibility %s", Config.display_bitrate ? "enabled" : "disabled");
}

void AddRandomItems::Run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Add random? [" << NC::fmtBold << 's' << NC::fmtBoldEnd << "ongs/" << NC::fmtBold << 'a' << NC::fmtBoldEnd << "rtists/al" << NC::fmtBold << 'b' << NC::fmtBoldEnd << "ums] ";
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
		tag_type_str = lowercase(tagTypeToString(tag_type));
	}
	else
		tag_type_str = "song";
	
	Statusbar::lock();
	Statusbar::put() << "Number of random " << tag_type_str << "s: ";
	size_t number = stringToLongInt(wFooter->getString());
	Statusbar::unlock();
	if (number && (answer == 's' ? Mpd.AddRandomSongs(number) : Mpd.AddRandomTag(tag_type, number)))
		Statusbar::msg("%zu random %s%s added to playlist", number, tag_type_str.c_str(), number == 1 ? "" : "s");
}

bool ToggleBrowserSortMode::canBeRun() const
{
	return myScreen == myBrowser;
}

void ToggleBrowserSortMode::Run()
{
	switch (Config.browser_sort_mode)
	{
		case smName:
			if (!myBrowser->isLocal())
			{
				Config.browser_sort_mode = smMTime;
				Statusbar::msg("Sort songs by: Modification time");
				break;
			}
		// local browser doesn't support sorting by mtime, so we just skip it.
		case smMTime:
			Config.browser_sort_mode = smCustomFormat;
			Statusbar::msg("Sort songs by: Custom format");
			break;
		case smCustomFormat:
			Config.browser_sort_mode = smName;
			Statusbar::msg("Sort songs by: Name");
			break;
	}
	std::sort(myBrowser->main().begin()+(myBrowser->CurrentDir() != "/"), myBrowser->main().end(),
		LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the, Config.browser_sort_mode));
}

bool ToggleLibraryTagType::canBeRun() const
{
	return (myScreen->isActiveWindow(myLibrary->Tags))
	    || (myLibrary->Columns() == 2 && myScreen->isActiveWindow(myLibrary->Albums));
}

void ToggleLibraryTagType::Run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Tag type? [" << NC::fmtBold << 'a' << NC::fmtBoldEnd << "rtist/album" << NC::fmtBold << 'A' << NC::fmtBoldEnd << "rtist/" << NC::fmtBold << 'y' << NC::fmtBoldEnd << "ear/" << NC::fmtBold << 'g' << NC::fmtBoldEnd << "enre/" << NC::fmtBold << 'c' << NC::fmtBoldEnd << "omposer/" << NC::fmtBold << 'p' << NC::fmtBoldEnd << "erformer] ";
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
		item_type = lowercase(item_type);
		if (myLibrary->Columns() == 2)
		{
			myLibrary->Songs.clear();
			myLibrary->Albums.reset();
			myLibrary->Albums.clear();
			myLibrary->Albums.setTitle(Config.titles_visibility ? "Albums (sorted by " + item_type + ")" : "");
			myLibrary->Albums.display();
		}
		else
		{
			myLibrary->Tags.clear();
			myLibrary->Tags.display();
		}
		Statusbar::msg("Switched to list of %s tag", item_type.c_str());
	}
}

bool RefetchLyrics::canBeRun() const
{
#	ifdef HAVE_CURL_CURL_H
	return myScreen == myLyrics;
#	else
	return false;
#	endif // HAVE_CURL_CURL_H
}

void RefetchLyrics::Run()
{
#	ifdef HAVE_CURL_CURL_H
	myLyrics->Refetch();
#	endif // HAVE_CURL_CURL_H
}

bool RefetchArtistInfo::canBeRun() const
{
#	ifdef HAVE_CURL_CURL_H
	return myScreen == myLastfm;
#	else
	return false;
#	endif // HAVE_CURL_CURL_H
}

void RefetchArtistInfo::Run()
{
#	ifdef HAVE_CURL_CURL_H
	myLastfm->Refetch();
#	endif // HAVE_CURL_CURL_H
}

bool SetSelectedItemsPriority::canBeRun() const
{
	if (Mpd.Version() < 17)
	{
		Statusbar::msg("Priorities are supported in MPD >= 0.17.0");
		return false;
	}
	return myScreen == myPlaylist && !myPlaylist->main().empty();
}

void SetSelectedItemsPriority::Run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Set priority [0-255]: ";
	std::string strprio = wFooter->getString();
	Statusbar::unlock();
	if (!isInteger(strprio.c_str(), true))
		return;
	int prio = stringToInt(strprio);
	if (prio < 0 || prio > 255)
	{
		Statusbar::msg("Number is out of range");
		return;
	}
	myPlaylist->SetSelectedItemsPriority(prio);
}

bool FilterPlaylistOnPriorities::canBeRun() const
{
	return myScreen == myPlaylist;
}

void FilterPlaylistOnPriorities::Run()
{
	using Global::wFooter;
	
	Statusbar::lock();
	Statusbar::put() << "Show songs with priority higher than: ";
	std::string strprio = wFooter->getString();
	Statusbar::unlock();
	if (!isInteger(strprio.c_str(), false))
		return;
	unsigned prio = stringToInt(strprio);
	myPlaylist->main().filter(myPlaylist->main().begin(), myPlaylist->main().end(),
		[prio](const NC::Menu<MPD::Song>::Item &s) {
			return s.value().getPrio() > prio;
	});
	Statusbar::msg("Playlist filtered (songs with priority higher than %u)", prio);
}

void ShowSongInfo::Run()
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

void ShowArtistInfo::Run()
{
#	ifdef HAVE_CURL_CURL_H
	if (myScreen == myLastfm || myLastfm->isDownloading())
	{
		myLastfm->switchTo();
		return;
	}
	
	std::string artist;
	if (myScreen->isActiveWindow(myLibrary->Tags))
	{
		assert(!myLibrary->Tags.empty());
		assert(Config.media_lib_primary_tag == MPD_TAG_ARTIST);
		artist = myLibrary->Tags.current().value();
	}
	else
	{
		auto s = currentSong(myScreen);
		assert(s);
		artist = s->getArtist();
	}
	
	if (!artist.empty() && myLastfm->SetArtistInfoArgs(artist, Config.lastfm_preferred_language))
		myLastfm->switchTo();
#	endif // HAVE_CURL_CURL_H
}

void ShowLyrics::Run()
{
	myLyrics->switchTo();
}

void Quit::Run()
{
	ExitMainLoop = true;
}

void NextScreen::Run()
{
	if (Config.screen_switcher_previous)
	{
		if (auto tababble = dynamic_cast<Tabbable *>(myScreen))
			tababble->switchToPreviousScreen();
	}
	else if (!Config.screens_seq.empty())
	{
		auto screen = std::find(Config.screens_seq.begin(), Config.screens_seq.end(), myScreen);
		if (++screen == Config.screens_seq.end())
			Config.screens_seq.front()->switchTo();
		else
			(*screen)->switchTo();
	}
}

void PreviousScreen::Run()
{
	if (Config.screen_switcher_previous)
	{
		if (auto tababble = dynamic_cast<Tabbable *>(myScreen))
			tababble->switchToPreviousScreen();
	}
	else if (!Config.screens_seq.empty())
	{
		auto screen = std::find(Config.screens_seq.begin(), Config.screens_seq.end(), myScreen);
		if (screen == Config.screens_seq.begin())
			Config.screens_seq.back()->switchTo();
		else
			(*--screen)->switchTo();
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

void ShowHelp::Run()
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

void ShowPlaylist::Run()
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

void ShowBrowser::Run()
{
	myBrowser->switchTo();
}

bool ChangeBrowseMode::canBeRun() const
{
	return myScreen == myBrowser;
}

void ChangeBrowseMode::Run()
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

void ShowSearchEngine::Run()
{
	mySearcher->switchTo();
}

bool ResetSearchEngine::canBeRun() const
{
	return myScreen == mySearcher;
}

void ResetSearchEngine::Run()
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

void ShowMediaLibrary::Run()
{
	myLibrary->switchTo();
}

bool ToggleMediaLibraryColumnsMode::canBeRun() const
{
	return myScreen == myLibrary;
}

void ToggleMediaLibraryColumnsMode::Run()
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

void ShowPlaylistEditor::Run()
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

void ShowTagEditor::Run()
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

void ShowOutputs::Run()
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

void ShowVisualizer::Run()
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

void ShowClock::Run()
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

void ShowServerInfo::Run()
{
	myServerInfo->switchTo();
}

namespace {//

void insertAction(Action *a)
{
	Actions[a->Type()] = a;
}

void populateActions()
{
	insertAction(new Dummy());
	insertAction(new MouseEvent());
	insertAction(new ScrollUp());
	insertAction(new ScrollDown());
	insertAction(new ScrollUpArtist());
	insertAction(new ScrollUpAlbum());
	insertAction(new ScrollDownArtist());
	insertAction(new ScrollDownAlbum());
	insertAction(new PageUp());
	insertAction(new PageDown());
	insertAction(new MoveHome());
	insertAction(new MoveEnd());
	insertAction(new ToggleInterface());
	insertAction(new JumpToParentDirectory());
	insertAction(new PressEnter());
	insertAction(new PressSpace());
	insertAction(new PreviousColumn());
	insertAction(new NextColumn());
	insertAction(new MasterScreen());
	insertAction(new SlaveScreen());
	insertAction(new VolumeUp());
	insertAction(new VolumeDown());
	insertAction(new Delete());
	insertAction(new ReplaySong());
	insertAction(new PreviousSong());
	insertAction(new NextSong());
	insertAction(new Pause());
	insertAction(new Stop());
	insertAction(new SavePlaylist());
	insertAction(new MoveSortOrderUp());
	insertAction(new MoveSortOrderDown());
	insertAction(new MoveSelectedItemsUp());
	insertAction(new MoveSelectedItemsDown());
	insertAction(new MoveSelectedItemsTo());
	insertAction(new Add());
	insertAction(new SeekForward());
	insertAction(new SeekBackward());
	insertAction(new ToggleDisplayMode());
	insertAction(new ToggleSeparatorsBetweenAlbums());
	insertAction(new ToggleLyricsFetcher());
	insertAction(new ToggleFetchingLyricsInBackground());
	insertAction(new TogglePlayingSongCentering());
	insertAction(new UpdateDatabase());
	insertAction(new JumpToPlayingSong());
	insertAction(new ToggleRepeat());
	insertAction(new Shuffle());
	insertAction(new ToggleRandom());
	insertAction(new StartSearching());
	insertAction(new SaveTagChanges());
	insertAction(new ToggleSingle());
	insertAction(new ToggleConsume());
	insertAction(new ToggleCrossfade());
	insertAction(new SetCrossfade());
	insertAction(new EditSong());
	insertAction(new EditLibraryTag());
	insertAction(new EditLibraryAlbum());
	insertAction(new EditDirectoryName());
	insertAction(new EditPlaylistName());
	insertAction(new EditLyrics());
	insertAction(new JumpToBrowser());
	insertAction(new JumpToMediaLibrary());
	insertAction(new JumpToPlaylistEditor());
	insertAction(new ToggleScreenLock());
	insertAction(new JumpToTagEditor());
	insertAction(new JumpToPositionInSong());
	insertAction(new ReverseSelection());
	insertAction(new RemoveSelection());
	insertAction(new SelectAlbum());
	insertAction(new AddSelectedItems());
	insertAction(new CropMainPlaylist());
	insertAction(new CropPlaylist());
	insertAction(new ClearMainPlaylist());
	insertAction(new ClearPlaylist());
	insertAction(new SortPlaylist());
	insertAction(new ReversePlaylist());
	insertAction(new ApplyFilter());
	insertAction(new Find());
	insertAction(new FindItemForward());
	insertAction(new FindItemBackward());
	insertAction(new NextFoundItem());
	insertAction(new PreviousFoundItem());
	insertAction(new ToggleFindMode());
	insertAction(new ToggleReplayGainMode());
	insertAction(new ToggleSpaceMode());
	insertAction(new ToggleAddMode());
	insertAction(new ToggleMouse());
	insertAction(new ToggleBitrateVisibility());
	insertAction(new AddRandomItems());
	insertAction(new ToggleBrowserSortMode());
	insertAction(new ToggleLibraryTagType());
	insertAction(new RefetchLyrics());
	insertAction(new RefetchArtistInfo());
	insertAction(new SetSelectedItemsPriority());
	insertAction(new FilterPlaylistOnPriorities());
	insertAction(new ShowSongInfo());
	insertAction(new ShowArtistInfo());
	insertAction(new ShowLyrics());
	insertAction(new Quit());
	insertAction(new NextScreen());
	insertAction(new PreviousScreen());
	insertAction(new ShowHelp());
	insertAction(new ShowPlaylist());
	insertAction(new ShowBrowser());
	insertAction(new ChangeBrowseMode());
	insertAction(new ShowSearchEngine());
	insertAction(new ResetSearchEngine());
	insertAction(new ShowMediaLibrary());
	insertAction(new ToggleMediaLibraryColumnsMode());
	insertAction(new ShowPlaylistEditor());
	insertAction(new ShowTagEditor());
	insertAction(new ShowOutputs());
	insertAction(new ShowVisualizer());
	insertAction(new ShowClock());
	insertAction(new ShowServerInfo());
}

}
