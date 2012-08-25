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
#include <iostream>

#include "actions.h"
#include "charset.h"
#include "config.h"
#include "display.h"
#include "global.h"
#include "mpdpp.h"

#include "browser.h"
#include "clock.h"
#include "help.h"
#include "media_library.h"
#include "lastfm.h"
#include "lyrics.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "sel_items_adder.h"
#include "server_info.h"
#include "song_info.h"
#include "outputs.h"
#include "tag_editor.h"
#include "tiny_tag_editor.h"
#include "visualizer.h"

using Global::myScreen;

bool Action::OriginalStatusbarVisibility;
bool Action::DesignChanged;
bool Action::OrderResize;
bool Action::ExitMainLoop = false;

size_t Action::HeaderHeight;
size_t Action::FooterHeight;
size_t Action::FooterStartY;

std::map<ActionType, Action *> Action::Actions;
Action::Key Action::NoOp = Action::Key(ERR, ctNCurses);

Action::Key Action::ReadKey(Window &w)
{
	std::string tmp;
	int input;
	while (true)
	{
		input = w.ReadKey();
		if (input == ERR)
			return NoOp;
		if (input > 255)
			return Key(input, ctNCurses);
		else
		{
			wchar_t wc;
			tmp += input;
			size_t conv_res = mbrtowc(&wc, tmp.c_str(), MB_CUR_MAX, 0);
			if (conv_res == size_t(-1)) // incomplete multibyte character
				continue;
			else if (conv_res == size_t(-2)) // garbage character sequence
				return NoOp;
			else // character complete
				return Key(wc, ctStandard);
		}
	}
	// not reachable
	assert(false);
}

void Action::ValidateScreenSize()
{
	using Global::MainHeight;
	
	if (COLS < 20 || MainHeight < 3)
	{
		DestroyScreen();
		std::cout << "Screen is too small!\n";
		exit(1);
	}
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

void Action::ResizeScreen()
{
	using Global::MainHeight;
	using Global::RedrawHeader;
	using Global::RedrawStatusbar;
	using Global::wHeader;
	using Global::wFooter;
	
	OrderResize = 0;
	
#	if defined(USE_PDCURSES)
	resize_term(0, 0);
#	else
	// update internal screen dimensions
	if (!DesignChanged)
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
	
	RedrawHeader = 1;
	MainHeight = LINES-(Config.new_design ? 7 : 4);
	
	ValidateScreenSize();
	
	if (!Config.header_visibility)
		MainHeight += 2;
	if (!Config.statusbar_visibility)
		MainHeight++;
	
	SetResizeFlags();
	
	ApplyToVisibleWindows(&BasicScreen::Resize);
	
	if (Config.header_visibility || Config.new_design)
		wHeader->Resize(COLS, HeaderHeight);
	
	FooterStartY = LINES-(Config.statusbar_visibility ? 2 : 1);
	wFooter->MoveTo(0, FooterStartY);
	wFooter->Resize(COLS, Config.statusbar_visibility ? 2 : 1);
	
	ApplyToVisibleWindows(&BasicScreen::Refresh);
	RedrawStatusbar = 1;
	MPD::StatusChanges changes;
	if (!Mpd.isPlaying() || DesignChanged)
	{
		changes.PlayerState = 1;
		if (DesignChanged)
			changes.Volume = 1;
	}
	// Note: routines for drawing separator if alternative user
	// interface is active and header is hidden are placed in
	// NcmpcppStatusChanges.StatusFlags
	changes.StatusFlags = 1; // force status update
	NcmpcppStatusChanged(&Mpd, changes, 0);
	if (DesignChanged)
	{
		RedrawStatusbar = 1;
		NcmpcppStatusChanged(&Mpd, MPD::StatusChanges(), 0);
		DesignChanged = 0;
		ShowMessage("User interface: %s", Config.new_design ? "Alternative" : "Classic");
	}
	wFooter->Refresh();
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
		MainHeight++;
	
	HeaderHeight = Config.new_design ? (Config.header_visibility ? 5 : 3) : 1;
	FooterStartY = LINES-(Config.statusbar_visibility ? 2 : 1);
	FooterHeight = Config.statusbar_visibility ? 2 : 1;
}

void Action::Seek()
{
	using Global::wHeader;
	using Global::wFooter;
	using Global::SeekingInProgress;
	
	if (!Mpd.GetTotalTime())
	{
		ShowMessage("Unknown item length");
		return;
	}
	
	LockProgressbar();
	LockStatusbar();
	
	int songpos = Mpd.GetElapsedTime();
	time_t t = time(0);
	
	int old_timeout = wFooter->GetTimeout();
	wFooter->SetTimeout(ncmpcpp_window_timeout);
	
	SeekingInProgress = true;
	while (true)
	{
		TraceMpdStatus();
		myPlaylist->UpdateTimer();
		
		int howmuch = Config.incremental_seeking ? (myPlaylist->Timer()-t)/2+Config.seek_time : Config.seek_time;
		
		Key input = ReadKey(*wFooter);
		KeyConfiguration::Binding k = Keys.Bindings.equal_range(input);
		// no action?
		if (k.first == k.second || !k.first->second.isSingle())
			break;
		Action *a = k.first->second.getAction();
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
		
		*wFooter << fmtBold;
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
			*wHeader << XY(0, 0) << tracklength << " ";
			wHeader->Refresh();
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
			*wFooter << XY(wFooter->GetWidth()-tracklength.length(), 1) << tracklength;
		}
		*wFooter << fmtBoldEnd;
		DrawProgressbar(songpos, Mpd.GetTotalTime());
		wFooter->Refresh();
	}
	SeekingInProgress = false;
	Mpd.Seek(songpos);
	
	wFooter->SetTimeout(old_timeout);
	
	UnlockProgressbar();
	UnlockStatusbar();
}

void Action::FindItem(const FindDirection fd)
{
	using Global::wFooter;
	
	List *mList = myScreen->GetList();
	if (!mList)
		return;
	
	LockStatusbar();
	Statusbar() << "Find " << (fd == fdForward ? "forward" : "backward") << ": ";
	std::string findme = wFooter->GetString();
	UnlockStatusbar();
	
	if (!findme.empty())
		ShowMessage("Searching...");
	
	bool success = mList->Search(findme, 0, REG_ICASE | Config.regex_type);
	
	if (findme.empty())
		return;
	
	if (success)
		ShowMessage("Searching finished");
	else
		ShowMessage("Unable to find \"%s\"", findme.c_str());
	
	if (fd == fdForward)
		mList->NextFound(Config.wrapped_search);
	else
		mList->PrevFound(Config.wrapped_search);
	
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
		if (myScreen->ActiveWindow() == myLibrary->Artists)
		{
			myLibrary->Albums->Clear();
			myLibrary->Songs->Clear();
		}
		else if (myScreen->ActiveWindow() == myLibrary->Albums)
		{
			myLibrary->Songs->Clear();
		}
		else if (myScreen->ActiveWindow() == myPlaylistEditor->Playlists)
		{
			myPlaylistEditor->Content->Clear();
		}
#		ifdef HAVE_TAGLIB_H
		else if (myScreen->ActiveWindow() == myTagEditor->LeftColumn)
		{
			myTagEditor->Tags->Clear();
		}
#		endif // HAVE_TAGLIB_H
	}
}

bool Action::AskYesNoQuestion(const std::string &question, void (*callback)())
{
	using Global::wFooter;
	
	LockStatusbar();
	Statusbar() << question << " [" << fmtBold << 'y' << fmtBoldEnd << '/' << fmtBold << 'n' << fmtBoldEnd << "]";
	wFooter->Refresh();
	int answer = 0;
	do
	{
		if (callback)
			callback();
		answer = wFooter->ReadKey();
	}
	while (answer != 'y' && answer != 'n');
	UnlockStatusbar();
	return answer == 'y';
}

bool Action::isMPDMusicDirSet()
{
	if (Config.mpd_music_dir.empty())
	{
		ShowMessage("Proper mpd_music_dir variable has to be set in configuration file");
		return false;
	}
	return true;
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
		if (!myPlaylist->isPlaying())
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
		myScreen->MouseButtonPressed(itsMouseEvent);
}

void ScrollUp::Run()
{
	myScreen->Scroll(wUp);
	ListsChangeFinisher();
}

void ScrollDown::Run()
{
	myScreen->Scroll(wDown);
	ListsChangeFinisher();
}

void ScrollUpArtist::Run()
{
	List *mList = myScreen->GetList();
	if (!mList || mList->Empty())
		return;
	size_t pos = mList->Choice();
	if (MPD::Song *s = myScreen->GetSong(pos))
	{
		std::string artist = s->GetArtist();
		while (pos > 0)
		{
			s = myScreen->GetSong(--pos);
			if (!s || s->GetArtist() != artist)
				break;
		}
		mList->Highlight(pos);
	}
}

void ScrollUpAlbum::Run()
{
	List *mList = myScreen->GetList();
	if (!mList || mList->Empty())
		return;
	size_t pos = mList->Choice();
	if (MPD::Song *s = myScreen->GetSong(pos))
	{
		std::string album = s->GetAlbum();
		while (pos > 0)
		{
			s = myScreen->GetSong(--pos);
			if (!s || s->GetAlbum() != album)
				break;
		}
		mList->Highlight(pos);
	}
}

void ScrollDownArtist::Run()
{
	List *mList = myScreen->GetList();
	if (!mList || mList->Empty())
		return;
	size_t pos = mList->Choice();
	if (MPD::Song *s = myScreen->GetSong(pos))
	{
		std::string artist = s->GetArtist();
		while (pos < mList->Size() - 1)
		{
			s = myScreen->GetSong(++pos);
			if (!s || s->GetArtist() != artist)
				break;
		}
		mList->Highlight(pos);
	}
}

void ScrollDownAlbum::Run()
{
	List *mList = myScreen->GetList();
	if (!mList || mList->Empty())
		return;
	size_t pos = mList->Choice();
	if (MPD::Song *s = myScreen->GetSong(pos))
	{
		std::string album = s->GetAlbum();
		while (pos < mList->Size() - 1)
		{
			s = myScreen->GetSong(++pos);
			if (!s || s->GetAlbum() != album)
				break;
		}
		mList->Highlight(pos);
	}
}

void PageUp::Run()
{
	myScreen->Scroll(wPageUp);
	ListsChangeFinisher();
}

void PageDown::Run()
{
	myScreen->Scroll(wPageDown);
	ListsChangeFinisher();
}

void MoveHome::Run()
{
	myScreen->Scroll(wHome);
	ListsChangeFinisher();
}

void MoveEnd::Run()
{
	myScreen->Scroll(wEnd);
	ListsChangeFinisher();
}

void ToggleInterface::Run()
{
	Config.new_design = !Config.new_design;
	Config.statusbar_visibility = Config.new_design ? 0 : OriginalStatusbarVisibility;
	SetWindowsDimensions();
	UnlockProgressbar();
	UnlockStatusbar();
	DesignChanged = true;
	ResizeScreen();
}

bool JumpToParentDir::canBeRun() const
{
	return myScreen == myBrowser
#	ifdef HAVE_TAGLIB_H
	    || myScreen == myTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void JumpToParentDir::Run()
{
	if (myScreen == myBrowser && myBrowser->CurrentDir() != "/")
	{
		myBrowser->Main()->Reset();
		myBrowser->EnterPressed();
	}
#	ifdef HAVE_TAGLIB_H
	else if (myScreen->ActiveWindow() == myTagEditor->Dirs && myTagEditor->CurrentDir() != "/")
	{
		myTagEditor->Dirs->Reset();
		myTagEditor->EnterPressed();
	}
#	endif // HAVE_TAGLIB_H
}

void PressEnter::Run()
{
	myScreen->EnterPressed();
}

void PressSpace::Run()
{
	myScreen->SpacePressed();
}

bool PreviousColumn::canBeRun() const
{
	return (myScreen == myLibrary && myLibrary->isPrevColumnAvailable())
	    || (myScreen == myPlaylistEditor && myPlaylistEditor->isPrevColumnAvailable())
#	ifdef HAVE_TAGLIB_H
	    || (myScreen == myTagEditor && myTagEditor->isPrevColumnAvailable())
#	endif // HAVE_TAGLIB_H
	;
}

void PreviousColumn::Run()
{
	if (myScreen == myLibrary)
		myLibrary->PrevColumn();
	else if (myScreen == myPlaylistEditor)
		myPlaylistEditor->PrevColumn();
#	ifdef HAVE_TAGLIB_H
	else if (myScreen == myTagEditor)
		myTagEditor->PrevColumn();
#	endif // HAVE_TAGLIB_H
}

bool NextColumn::canBeRun() const
{
	return (myScreen == myLibrary && myLibrary->isNextColumnAvailable())
	    || (myScreen == myPlaylistEditor && myPlaylistEditor->isNextColumnAvailable())
#	ifdef HAVE_TAGLIB_H
	    || (myScreen == myTagEditor && myTagEditor->isNextColumnAvailable())
#	endif // HAVE_TAGLIB_H
	;
}

void NextColumn::Run()
{
	if (myScreen == myLibrary)
		myLibrary->NextColumn();
	else if (myScreen == myPlaylistEditor)
		myPlaylistEditor->NextColumn();
#	ifdef HAVE_TAGLIB_H
	else if (myScreen == myTagEditor)
		myTagEditor->NextColumn();
#	endif // HAVE_TAGLIB_H
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
	using Global::RedrawHeader;
	
	myInactiveScreen = myScreen;
	myScreen = myLockedScreen;
	RedrawHeader = true;
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
	using Global::RedrawHeader;
	
	myScreen = myInactiveScreen;
	myInactiveScreen = myLockedScreen;
	RedrawHeader = true;
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
	using Global::wFooter;
	using MPD::itDirectory;
	using MPD::itSong;
	using MPD::itPlaylist;
	
	if (!myPlaylist->Items->Empty() && myScreen == myPlaylist)
	{
		if (myPlaylist->Items->hasSelected())
		{
			std::vector<size_t> list;
			myPlaylist->Items->GetSelected(list);
			Mpd.StartCommandsList();
			for (std::vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); ++it)
				Mpd.DeleteID((*myPlaylist->Items)[*it].GetID());
			if (Mpd.CommitCommandsList())
			{
				for (size_t i = 0; i < myPlaylist->Items->Size(); ++i)
					myPlaylist->Items->Select(i, 0);
				ShowMessage("Selected items deleted");
			}
		}
		else
			Mpd.DeleteID(myPlaylist->CurrentSong()->GetID());
	}
	else if (
		 (myScreen == myBrowser && !myBrowser->Main()->Empty() && myBrowser->CurrentDir() == "/" && myBrowser->Main()->Current().type == itPlaylist)
	||       (myScreen->ActiveWindow() == myPlaylistEditor->Playlists)
		)
	{
		std::string name;
		if (myScreen == myBrowser)
			name = myBrowser->Main()->Current().name;
		else
			name = myPlaylistEditor->Playlists->Current();
		bool yes = AskYesNoQuestion("Delete playlist \"" + Shorten(TO_WSTRING(name), COLS-28) + "\"?", TraceMpdStatus);
		if (yes)
		{
			if (Mpd.DeletePlaylist(locale_to_utf_cpy(name)))
			{
				const char msg[] = "Playlist \"%s\" deleted";
				ShowMessage(msg, Shorten(TO_WSTRING(name), COLS-static_strlen(msg)).c_str());
				if (myBrowser->Main() && !myBrowser->isLocal() && myBrowser->CurrentDir() == "/")
					myBrowser->GetDirectory("/");
			}
		}
		else
			ShowMessage("Aborted");
		if (myPlaylistEditor->Main()) // check if initialized
			myPlaylistEditor->Playlists->Clear(); // make playlists list update itself
	}
#	ifndef WIN32
	else if (myScreen == myBrowser && !myBrowser->Main()->Empty())
	{
		if (!myBrowser->isLocal() && !isMPDMusicDirSet())
			return;
		
		MPD::Item &item = myBrowser->Main()->Current();
		
		if (item.type == itSong && !Config.allow_physical_files_deletion)
		{
			ShowMessage("Deleting files is disabled by default, see man page for more details");
			return;
		}
		if (item.type == itDirectory && !Config.allow_physical_directories_deletion)
		{
			ShowMessage("Deleting directories is disabled by default, see man page for more details");
			return;
		}
		if (item.type == itDirectory && item.song) // parent dir
			return;
		
		std::string name = item.type == itSong ? item.song->GetName() : item.name;
		std::string question;
		if (myBrowser->Main()->hasSelected())
			question = "Delete selected items?";
		else
		{
			question = "Delete ";
			question += (item.type == itSong ? "file" : item.type == itDirectory ? "directory" : "playlist");
			question += " \"";
			question += Shorten(TO_WSTRING(name), COLS-30);
			question += "\"?";
		}
		bool yes = AskYesNoQuestion(question, TraceMpdStatus);
		if (yes)
		{
			std::vector<size_t> list;
			myBrowser->Main()->GetSelected(list);
			if (list.empty())
				list.push_back(myBrowser->Main()->Choice());
			bool success = 1;
			for (size_t i = 0; i < list.size(); ++i)
			{
				const MPD::Item &it = (*myBrowser->Main())[list[i]];
				name = it.type == itSong ? it.song->GetName() : it.name;
				if (myBrowser->DeleteItem(it))
				{
					const char msg[] = "\"%s\" deleted";
					ShowMessage(msg, Shorten(TO_WSTRING(name), COLS-static_strlen(msg)).c_str());
				}
				else
				{
					const char msg[] = "Couldn't delete \"%s\": %s";
					ShowMessage(msg, Shorten(TO_WSTRING(name), COLS-static_strlen(msg)-25).c_str(), strerror(errno));
					success = 0;
					break;
				}
			}
			if (success)
			{
				if (!myBrowser->isLocal())
					Mpd.UpdateDirectory(myBrowser->CurrentDir());
				else
					myBrowser->GetDirectory(myBrowser->CurrentDir());
			}
		}
		else
			ShowMessage("Aborted");
			
	}
#	endif // !WIN32
	else if (myScreen->ActiveWindow() == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
	{
		if (myPlaylistEditor->Content->hasSelected())
		{
			std::vector<size_t> list;
			myPlaylistEditor->Content->GetSelected(list);
			std::string playlist = locale_to_utf_cpy(myPlaylistEditor->Playlists->Current());
			ShowMessage("Deleting selected items...");
			Mpd.StartCommandsList();
			for (std::vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); ++it)
			{
				Mpd.Delete(playlist, *it);
				myPlaylistEditor->Content->DeleteOption(*it);
			}
			Mpd.CommitCommandsList();
			ShowMessage("Selected items deleted from playlist \"%s\"", myPlaylistEditor->Playlists->Current().c_str());
		}
		else
		{
			if (Mpd.Delete(myPlaylistEditor->Playlists->Current(), myPlaylistEditor->Content->Choice()))
				myPlaylistEditor->Content->DeleteOption(myPlaylistEditor->Content->Choice());
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
	
	LockStatusbar();
	Statusbar() << "Save playlist as: ";
	std::string playlist_name = wFooter->GetString();
	std::string real_playlist_name = locale_to_utf_cpy(playlist_name);
	UnlockStatusbar();
	if (playlist_name.find("/") != std::string::npos)
	{
		ShowMessage("Playlist name must not contain slashes");
		return;
	}
	if (!playlist_name.empty())
	{
		if (myPlaylist->Items->isFiltered())
		{
			Mpd.StartCommandsList();
			for (size_t i = 0; i < myPlaylist->Items->Size(); ++i)
				Mpd.AddToPlaylist(real_playlist_name, (*myPlaylist->Items)[i]);
			Mpd.CommitCommandsList();
			if (Mpd.GetErrorMessage().empty())
				ShowMessage("Filtered items added to playlist \"%s\"", playlist_name.c_str());
		}
		else
		{
			int result = Mpd.SavePlaylist(real_playlist_name);
			if (result == MPD_ERROR_SUCCESS)
			{
				ShowMessage("Playlist saved as \"%s\"", playlist_name.c_str());
				if (myPlaylistEditor->Main()) // check if initialized
					myPlaylistEditor->Playlists->Clear(); // make playlist's list update itself
			}
			else if (result == MPD_SERVER_ERROR_EXIST)
			{
				bool yes = AskYesNoQuestion("Playlist \"" + playlist_name + "\" already exists, overwrite?", TraceMpdStatus);
				if (yes)
				{
					Mpd.DeletePlaylist(real_playlist_name);
					if (Mpd.SavePlaylist(real_playlist_name) == MPD_ERROR_SUCCESS)
						ShowMessage("Playlist overwritten");
				}
				else
					ShowMessage("Aborted");
				if (myPlaylistEditor->Main()) // check if initialized
					myPlaylistEditor->Playlists->Clear(); // make playlist's list update itself
				if (myScreen == myPlaylist)
					myPlaylist->EnableHighlighting();
			}
		}
	}
	if (myBrowser->Main()
	&&  !myBrowser->isLocal()
	&&  myBrowser->CurrentDir() == "/"
	&&  !myBrowser->Main()->Empty())
		myBrowser->GetDirectory(myBrowser->CurrentDir());
}

void Stop::Run()
{
	Mpd.Stop();
}

bool MoveSortOrderUp::canBeRun() const
{
	return myScreen == myPlaylist
	    && myPlaylist->SortingInProgress();
}

void MoveSortOrderUp::Run()
{
	myPlaylist->AdjustSortOrder(Playlist::mUp);
}

bool MoveSortOrderDown::canBeRun() const
{
	return myScreen == myPlaylist
	    && myPlaylist->SortingInProgress();
}

void MoveSortOrderDown::Run()
{
	myPlaylist->AdjustSortOrder(Playlist::mDown);
}

bool MoveSelectedItemsUp::canBeRun() const
{
	return myScreen->ActiveWindow() == myPlaylist->Items
	    || myScreen->ActiveWindow() == myPlaylistEditor->Content;
}

void MoveSelectedItemsUp::Run()
{
	if (myScreen == myPlaylist)
		myPlaylist->MoveSelectedItems(Playlist::mUp);
	else if (myScreen == myPlaylistEditor)
		myPlaylistEditor->MoveSelectedItems(Playlist::mUp);
}

bool MoveSelectedItemsDown::canBeRun() const
{
	return myScreen->ActiveWindow() == myPlaylist->Items
	    || myScreen->ActiveWindow() == myPlaylistEditor->Content;
}

void MoveSelectedItemsDown::Run()
{
	if (myScreen == myPlaylist)
		myPlaylist->MoveSelectedItems(Playlist::mDown);
	else if (myScreen == myPlaylistEditor)
		myPlaylistEditor->MoveSelectedItems(Playlist::mDown);
}

void MoveSelectedItemsTo::Run()
{
	if (myPlaylist->isFiltered())
		return;
	if (!myPlaylist->Items->hasSelected())
	{
		ShowMessage("No selected items to move");
		return;
	}
	// remove search results as we may move them to different positions, but
	// search rememebers positions and may point to wrong ones after that.
	myPlaylist->Items->Search("");
	size_t pos = myPlaylist->Items->Choice();
	std::vector<size_t> list;
	myPlaylist->Items->GetSelected(list);
	if (pos >= list.front() && pos <= list.back()) // can't move to the middle of selected items
		return;
	++pos; // always move after currently highlighted item
	int diff = pos-list.front();
	Mpd.StartCommandsList();
	if (diff > 0)
	{
		pos -= list.size();
		size_t i = list.size()-1;
		std::vector<size_t>::reverse_iterator it = list.rbegin();
		for (; it != list.rend(); ++it, --i)
			Mpd.Move(*it, pos+i);
		if (Mpd.CommitCommandsList())
		{
			i = list.size()-1;
			for (it = list.rbegin(); it != list.rend(); ++it, --i)
			{
				myPlaylist->Items->Select(*it, false);
				myPlaylist->Items->Select(pos+i, true);
			}
		}
	}
	else if (diff < 0)
	{
		size_t i = 0;
		std::vector<size_t>::const_iterator it = list.begin();
		for (; it != list.end(); ++it, ++i)
			Mpd.Move(*it, pos+i);
		if (Mpd.CommitCommandsList())
		{
			i = 0;
			for (it = list.begin(); it != list.end(); ++it, ++i)
			{
				myPlaylist->Items->Select(*it, false);
				myPlaylist->Items->Select(pos+i, true);
			}
		}
	}
}

bool Add::canBeRun() const
{
	return myScreen != myPlaylistEditor
	   || !myPlaylistEditor->Playlists->Empty();
}

void Add::Run()
{
	using Global::wFooter;
	
	LockStatusbar();
	Statusbar() << (myScreen == myPlaylistEditor ? "Add to playlist: " : "Add: ");
	std::string path = wFooter->GetString();
	locale_to_utf(path);
	UnlockStatusbar();
	if (!path.empty())
	{
		Statusbar() << "Adding...";
		wFooter->Refresh();
		if (myScreen == myPlaylistEditor)
		{
			Mpd.AddToPlaylist(myPlaylistEditor->Playlists->Current(), path);
			myPlaylistEditor->Content->Clear(); // make it refetch content of playlist
		}
		else
		{
			const char lastfm_url[] = "lastfm://";
			if (path.compare(0, static_strlen(lastfm_url), lastfm_url) == 0
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
	return myPlaylist->NowPlayingSong();
}

void SeekForward::Run()
{
	Seek();
}

bool SeekBackward::canBeRun() const
{
	return myPlaylist->NowPlayingSong();
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
	    || myScreen->ActiveWindow() == myPlaylistEditor->Content;
}

void ToggleDisplayMode::Run()
{
	if (myScreen == myPlaylist)
	{
		Config.columns_in_playlist = !Config.columns_in_playlist;
		ShowMessage("Playlist display mode: %s", Config.columns_in_playlist ? "Columns" : "Classic");
		
		if (Config.columns_in_playlist)
		{
			myPlaylist->Items->SetItemDisplayer(Display::SongsInColumns);
			myPlaylist->Items->SetTitle(Config.titles_visibility ? Display::Columns(myPlaylist->Items->GetWidth()) : "");
			myPlaylist->Items->SetGetStringFunction(Playlist::SongInColumnsToString);
		}
		else
		{
			myPlaylist->Items->SetItemDisplayer(Display::Songs);
			myPlaylist->Items->SetTitle("");
			myPlaylist->Items->SetGetStringFunction(Playlist::SongToString);
		}
	}
	else if (myScreen == myBrowser)
	{
		Config.columns_in_browser = !Config.columns_in_browser;
		ShowMessage("Browser display mode: %s", Config.columns_in_browser ? "Columns" : "Classic");
		myBrowser->Main()->SetTitle(Config.columns_in_browser && Config.titles_visibility ? Display::Columns(myBrowser->Main()->GetWidth()) : "");
	}
	else if (myScreen == mySearcher)
	{
		Config.columns_in_search_engine = !Config.columns_in_search_engine;
		ShowMessage("Search engine display mode: %s", Config.columns_in_search_engine ? "Columns" : "Classic");
		if (mySearcher->Main()->Size() > SearchEngine::StaticOptions)
			mySearcher->Main()->SetTitle(Config.columns_in_search_engine && Config.titles_visibility ? Display::Columns(mySearcher->Main()->GetWidth()) : "");
	}
	else if (myScreen->ActiveWindow() == myPlaylistEditor->Content)
	{
		Config.columns_in_playlist_editor = !Config.columns_in_playlist_editor;
		ShowMessage("Playlist editor display mode: %s", Config.columns_in_playlist_editor ? "Columns" : "Classic");
		if (Config.columns_in_playlist_editor)
		{
			myPlaylistEditor->Content->SetItemDisplayer(Display::SongsInColumns);
			myPlaylistEditor->Content->SetGetStringFunction(Playlist::SongInColumnsToString);
		}
		else
		{
			myPlaylistEditor->Content->SetItemDisplayer(Display::Songs);
			myPlaylistEditor->Content->SetGetStringFunction(Playlist::SongToString);
		}
	}
}

bool ToggleSeparatorsInPlaylist::canBeRun() const
{
	return myScreen == myPlaylist;
}

void ToggleSeparatorsInPlaylist::Run()
{
	Config.playlist_separate_albums = !Config.playlist_separate_albums;
	ShowMessage("Separators between albums in playlist: %s", Config.playlist_separate_albums ? "On" : "Off");
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
	ShowMessage("Fetching lyrics for playing songs in background: %s", Config.fetch_lyrics_in_background ? "On" : "Off");
#	endif // HAVE_CURL_CURL_H
}

void ToggleAutoCenter::Run()
{
	Config.autocenter_mode = !Config.autocenter_mode;
	ShowMessage("Auto center mode: %s", Config.autocenter_mode ? "On" : "Off");
	if (Config.autocenter_mode && myPlaylist->isPlaying() && !myPlaylist->Items->isFiltered())
		myPlaylist->Items->Highlight(myPlaylist->NowPlaying);
}

void UpdateDatabase::Run()
{
	if (myScreen == myBrowser)
		Mpd.UpdateDirectory(locale_to_utf_cpy(myBrowser->CurrentDir()));
#	ifdef HAVE_TAGLIB_H
	else if (myScreen == myTagEditor && !Config.albums_in_tag_editor)
		Mpd.UpdateDirectory(myTagEditor->CurrentDir());
#	endif // HAVE_TAGLIB_H
	else
		Mpd.UpdateDirectory("/");
}

bool JumpToPlayingSong::canBeRun() const
{
	return (myScreen == myPlaylist || myScreen == myBrowser || myScreen == myLibrary)
	     && myPlaylist->isPlaying();
}

void JumpToPlayingSong::Run()
{
	using Global::RedrawHeader;
	
	if (myScreen == myPlaylist)
	{
		if (myPlaylist->isFiltered())
			return;
		assert(myPlaylist->isPlaying());
		myPlaylist->Items->Highlight(myPlaylist->NowPlaying);
	}
	else if (myScreen == myBrowser)
	{
		const MPD::Song *s = myPlaylist->NowPlayingSong();
		assert(s);
		myBrowser->LocateSong(*s);
		RedrawHeader = true;
	}
	else if (myScreen == myLibrary)
	{
		const MPD::Song *s = myPlaylist->NowPlayingSong();
		assert(s);
		myLibrary->LocateSong(*s);
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
	return myScreen == mySearcher;
}

void StartSearching::Run()
{
	if (mySearcher->Main()->isStatic(0))
		return;
	mySearcher->Main()->Highlight(SearchEngine::SearchButton);
	mySearcher->Main()->Highlighting(0);
	mySearcher->Main()->Refresh();
	mySearcher->Main()->Highlighting(1);
	mySearcher->EnterPressed();
}

bool SaveTagChanges::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen == myTinyTagEditor
	    || myScreen->ActiveWindow() == myTagEditor->TagTypes;
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void SaveTagChanges::Run()
{
#	ifdef HAVE_TAGLIB_H
	if (myScreen == myTinyTagEditor)
	{
		myTinyTagEditor->Main()->Highlight(myTinyTagEditor->Main()->Size()-2); // Save
		myTinyTagEditor->EnterPressed();
	}
	else if (myScreen->ActiveWindow() == myTagEditor->TagTypes)
	{
		myTagEditor->TagTypes->Highlight(myTagEditor->TagTypes->Size()-1); // Save
		myTagEditor->EnterPressed();
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
	
	LockStatusbar();
	Statusbar() << "Set crossfade to: ";
	std::string crossfade = wFooter->GetString(3);
	UnlockStatusbar();
	int cf = StrToInt(crossfade);
	if (cf > 0)
	{
		Config.crossfade_time = cf;
		Mpd.SetCrossfade(cf);
	}
}

bool EditSong::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen->CurrentSong();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditSong::Run()
{
#	ifdef HAVE_TAGLIB_H
	if (!isMPDMusicDirSet())
		return;
	const MPD::Song *s = myScreen->CurrentSong();
	assert(s);
	myTinyTagEditor->SetEdited(*s);
	myTinyTagEditor->SwitchTo();
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryTag::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen->ActiveWindow() == myLibrary->Artists
	   && !myLibrary->Artists->Empty();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryTag::Run()
{
#	ifdef HAVE_TAGLIB_H
	using Global::wFooter;
	
	if (!isMPDMusicDirSet())
		return;
	LockStatusbar();
	Statusbar() << fmtBold << IntoStr(Config.media_lib_primary_tag) << fmtBoldEnd << ": ";
	std::string new_tag = wFooter->GetString(myLibrary->Artists->Current());
	UnlockStatusbar();
	if (!new_tag.empty() && new_tag != myLibrary->Artists->Current())
	{
		bool success = 1;
		MPD::SongList list;
		ShowMessage("Updating tags...");
		Mpd.StartSearch(1);
		Mpd.AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(myLibrary->Artists->Current()));
		Mpd.CommitSearch(list);
		MPD::Song::SetFunction set = IntoSetFunction(Config.media_lib_primary_tag);
		assert(set);
		for (MPD::SongList::iterator it = list.begin(); it != list.end(); ++it)
		{
			(*it)->Localize();
			(*it)->SetTags(set, new_tag);
			ShowMessage("Updating tags in \"%s\"...", (*it)->GetName().c_str());
			std::string path = Config.mpd_music_dir + (*it)->GetFile();
			if (!TagEditor::WriteTags(**it))
			{
				const char msg[] = "Error while updating tags in \"%s\"";
				ShowMessage(msg, Shorten(TO_WSTRING((*it)->GetFile()), COLS-static_strlen(msg)).c_str());
				success = 0;
				break;
			}
		}
		if (success)
		{
			Mpd.UpdateDirectory(locale_to_utf_cpy(FindSharedDir(list)));
			ShowMessage("Tags updated successfully");
		}
		FreeSongList(list);
	}
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryAlbum::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen->ActiveWindow() == myLibrary->Albums
	    && !myLibrary->Albums->Empty();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryAlbum::Run()
{
#	ifdef HAVE_TAGLIB_H
	using Global::wFooter;
	
	if (!isMPDMusicDirSet())
		return;
	LockStatusbar();
	Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
	std::string new_album = wFooter->GetString(myLibrary->Albums->Current().Album);
	UnlockStatusbar();
	if (!new_album.empty() && new_album != myLibrary->Albums->Current().Album)
	{
		bool success = 1;
		ShowMessage("Updating tags...");
		for (size_t i = 0;  i < myLibrary->Songs->Size(); ++i)
		{
			(*myLibrary->Songs)[i].Localize();
			ShowMessage("Updating tags in \"%s\"...", (*myLibrary->Songs)[i].GetName().c_str());
			std::string path = Config.mpd_music_dir + (*myLibrary->Songs)[i].GetFile();
			TagLib::FileRef f(locale_to_utf_cpy(path).c_str());
			if (f.isNull())
			{
				const char msg[] = "Error while opening file \"%s\"";
				ShowMessage(msg, Shorten(TO_WSTRING((*myLibrary->Songs)[i].GetFile()), COLS-static_strlen(msg)).c_str());
				success = 0;
				break;
			}
			f.tag()->setAlbum(ToWString(new_album));
			if (!f.save())
			{
				const char msg[] = "Error while writing tags in \"%s\"";
				ShowMessage(msg, Shorten(TO_WSTRING((*myLibrary->Songs)[i].GetFile()), COLS-static_strlen(msg)).c_str());
				success = 0;
				break;
			}
		}
		if (success)
		{
			Mpd.UpdateDirectory(locale_to_utf_cpy(FindSharedDir(myLibrary->Songs)));
			ShowMessage("Tags updated successfully");
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditDirectoryName::canBeRun() const
{
	return   (myScreen == myBrowser
	      && !myBrowser->Main()->Empty()
	      && myBrowser->Main()->Current().type == MPD::itDirectory)
#	ifdef HAVE_TAGLIB_H
	    ||   (myScreen->ActiveWindow() == myTagEditor->Dirs
	      && !myTagEditor->Dirs->Empty()
	      && myTagEditor->Dirs->Choice() > 0)
#	endif // HAVE_TAGLIB_H
	;
}

void EditDirectoryName::Run()
{
	using Global::wFooter;
	
	if (!isMPDMusicDirSet())
		return;
	if (myScreen == myBrowser)
	{
		std::string old_dir = myBrowser->Main()->Current().name;
		LockStatusbar();
		Statusbar() << fmtBold << "Directory: " << fmtBoldEnd;
		std::string new_dir = wFooter->GetString(old_dir);
		UnlockStatusbar();
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir;
			if (!myBrowser->isLocal())
				full_old_dir += Config.mpd_music_dir;
			full_old_dir += locale_to_utf_cpy(old_dir);
			std::string full_new_dir;
			if (!myBrowser->isLocal())
				full_new_dir += Config.mpd_music_dir;
			full_new_dir += locale_to_utf_cpy(new_dir);
			int rename_result = rename(full_old_dir.c_str(), full_new_dir.c_str());
			if (rename_result == 0)
			{
				const char msg[] = "Directory renamed to \"%s\"";
				ShowMessage(msg, Shorten(TO_WSTRING(new_dir), COLS-static_strlen(msg)).c_str());
				if (!myBrowser->isLocal())
					Mpd.UpdateDirectory(locale_to_utf_cpy(FindSharedDir(old_dir, new_dir)));
				myBrowser->GetDirectory(myBrowser->CurrentDir());
			}
			else
			{
				const char msg[] = "Couldn't rename \"%s\": %s";
				ShowMessage(msg, Shorten(TO_WSTRING(old_dir), COLS-static_strlen(msg)-25).c_str(), strerror(errno));
			}
		}
	}
#	ifdef HAVE_TAGLIB_H
	else if (myScreen->ActiveWindow() == myTagEditor->Dirs)
	{
		std::string old_dir = myTagEditor->Dirs->Current().first;
		LockStatusbar();
		Statusbar() << fmtBold << "Directory: " << fmtBoldEnd;
		std::string new_dir = wFooter->GetString(old_dir);
		UnlockStatusbar();
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + locale_to_utf_cpy(old_dir);
			std::string full_new_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + locale_to_utf_cpy(new_dir);
			if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
			{
				const char msg[] = "Directory renamed to \"%s\"";
				ShowMessage(msg, Shorten(TO_WSTRING(new_dir), COLS-static_strlen(msg)).c_str());
				Mpd.UpdateDirectory(myTagEditor->CurrentDir());
			}
			else
			{
				const char msg[] = "Couldn't rename \"%s\": %s";
				ShowMessage(msg, Shorten(TO_WSTRING(old_dir), COLS-static_strlen(msg)-25).c_str(), strerror(errno));
			}
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditPlaylistName::canBeRun() const
{
	return   (myScreen->ActiveWindow() == myPlaylistEditor->Playlists
	      && !myPlaylistEditor->Playlists->Empty())
	    ||   (myScreen == myBrowser
	      && !myBrowser->Main()->Empty()
	      && myBrowser->Main()->Current().type == MPD::itPlaylist);
}

void EditPlaylistName::Run()
{
	using Global::wFooter;
	
	std::string old_name;
	if (myScreen->ActiveWindow() == myPlaylistEditor->Playlists)
		old_name = myPlaylistEditor->Playlists->Current();
	else
		old_name = myBrowser->Main()->Current().name;
	LockStatusbar();
	Statusbar() << fmtBold << "Playlist: " << fmtBoldEnd;
	std::string new_name = wFooter->GetString(old_name);
	UnlockStatusbar();
	if (!new_name.empty() && new_name != old_name)
	{
		if (Mpd.Rename(locale_to_utf_cpy(old_name), locale_to_utf_cpy(new_name)))
		{
			const char msg[] = "Playlist renamed to \"%s\"";
			ShowMessage(msg, Shorten(TO_WSTRING(new_name), COLS-static_strlen(msg)).c_str());
			if (myBrowser->Main() && !myBrowser->isLocal())
				myBrowser->GetDirectory("/");
			if (myPlaylistEditor->Main())
				myPlaylistEditor->Playlists->Clear();
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
	return myScreen->CurrentSong();
}

void JumpToBrowser::Run()
{
	MPD::Song *s = myScreen->CurrentSong();
	assert(s);
	myBrowser->LocateSong(*s);
}

bool JumpToMediaLibrary::canBeRun() const
{
	return myScreen->CurrentSong();
}

void JumpToMediaLibrary::Run()
{
	MPD::Song *s = myScreen->CurrentSong();
	assert(s);
	myLibrary->LocateSong(*s);
}

bool JumpToPlaylistEditor::canBeRun() const
{
	return myScreen == myBrowser
	    && myBrowser->Main()->Current().type == MPD::itPlaylist;
}

void JumpToPlaylistEditor::Run()
{
	myPlaylistEditor->Locate(myBrowser->Main()->Current().name);
}

void ToggleScreenLock::Run()
{
	using Global::wFooter;
	using Global::myLockedScreen;
	
	if (myLockedScreen != 0)
	{
		BasicScreen::Unlock();
		Action::SetResizeFlags();
		ShowMessage("Screen unlocked");
	}
	else
	{
		int part = Config.locked_screen_width_part*100;
		if (Config.ask_for_locked_screen_width_part)
		{
			LockStatusbar();
			Statusbar() << "% of the locked screen's width to be reserved (20-80): ";
			std::string str_part = wFooter->GetString(IntoStr(Config.locked_screen_width_part*100));
			UnlockStatusbar();
			if (str_part.empty())
				return;
			part = StrToInt(str_part);
		}
		if (part < 20 || part > 80)
		{
			ShowMessage("Number is out of range");
			return;
		}
		Config.locked_screen_width_part = part/100.0;
		if (myScreen->Lock())
			ShowMessage("Screen locked (with %d%% width)", part);
		else
			ShowMessage("Current screen can't be locked");
	}
}

bool JumpToTagEditor::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen->CurrentSong();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void JumpToTagEditor::Run()
{
#	ifdef HAVE_TAGLIB_H
	if (!isMPDMusicDirSet())
		return;
	MPD::Song *s = myScreen->CurrentSong();
	assert(s);
	myTagEditor->LocateSong(*s);
#	endif // HAVE_TAGLIB_H
}

bool JumpToPositionInSong::canBeRun() const
{
	return myPlaylist->NowPlayingSong();
}

void JumpToPositionInSong::Run()
{
	using Global::wFooter;
	
	if (!Mpd.GetTotalTime())
	{
		ShowMessage("Unknown item length");
		return;
	}
	
	const MPD::Song *s = myPlaylist->NowPlayingSong();
	assert(s);
	
	LockStatusbar();
	Statusbar() << "Position to go (in %/mm:ss/seconds(s)): ";
	std::string position = wFooter->GetString();
	UnlockStatusbar();
	
	if (position.empty())
		return;
	
	int newpos = 0;
	if (position.find(':') != std::string::npos) // probably time in mm:ss
	{
		newpos = StrToInt(position)*60 + StrToInt(position.substr(position.find(':')+1));
		if (newpos >= 0 && newpos <= Mpd.GetTotalTime())
			Mpd.Seek(newpos);
		else
			ShowMessage("Out of bounds, 0:00-%s possible for mm:ss, %s given", s->GetLength().c_str(), MPD::Song::ShowTime(newpos).c_str());
	}
	else if (position.find('s') != std::string::npos) // probably position in seconds
	{
		newpos = StrToInt(position);
		if (newpos >= 0 && newpos <= Mpd.GetTotalTime())
			Mpd.Seek(newpos);
		else
			ShowMessage("Out of bounds, 0-%d possible for seconds, %d given", s->GetTotalLength(), newpos);
	}
	else
	{
		newpos = StrToInt(position);
		if (newpos >= 0 && newpos <= 100)
			Mpd.Seek(Mpd.GetTotalTime()*newpos/100.0);
		else
			ShowMessage("Out of bounds, 0-100 possible for %%, %d given", newpos);
	}
}

bool ReverseSelection::canBeRun() const
{
	return myScreen->allowsSelection();
}

void ReverseSelection::Run()
{
	myScreen->ReverseSelection();
	ShowMessage("Selection reversed");
}

bool DeselectItems::canBeRun() const
{
	return myScreen->allowsSelection();
}

void DeselectItems::Run()
{
	List *mList = myScreen->GetList();
	if (!mList->hasSelected())
		return;
	for (size_t i = 0; i < mList->Size(); ++i)
		mList->Select(i, 0);
	ShowMessage("Items deselected");
}

bool SelectAlbum::canBeRun() const
{
	return myScreen->allowsSelection()
	    && myScreen->GetList();
}

void SelectAlbum::Run()
{
	List *mList = myScreen->GetList();
	assert(mList);
	size_t pos = mList->Choice();
	if (MPD::Song *s = myScreen->GetSong(pos))
	{
		std::string album = s->GetAlbum();
		
		// select song under cursor
		mList->Select(pos, 1);
		// go up
		while (pos > 0)
		{
			s = myScreen->GetSong(--pos);
			if (!s || s->GetAlbum() != album)
				break;
			else
				mList->Select(pos, 1);
		}
		// go down
		pos = mList->Choice();
		while (pos < mList->Size() - 1)
		{
			s = myScreen->GetSong(++pos);
			if (!s || s->GetAlbum() != album)
				break;
			else
				mList->Select(pos, 1);
		}
		ShowMessage("Album around cursor position selected");
	}
}

void AddSelectedItems::Run()
{
	mySelectedItemsAdder->SwitchTo();
}

void CropMainPlaylist::Run()
{
	if (myPlaylist->isFiltered())
		return;
	bool yes = true;
	if (Config.ask_before_clearing_main_playlist)
		yes = AskYesNoQuestion("Do you really want to crop main playlist?", TraceMpdStatus);
	if (yes)
	{
		bool delete_all_but_current = !myPlaylist->Items->hasSelected();
		Mpd.StartCommandsList();
		int current = myPlaylist->Items->Choice();
		for (int i = myPlaylist->Items->Size()-1; i >= 0; --i)
		{
			bool delete_i = (delete_all_but_current && i != current)
			             || (!delete_all_but_current && !myPlaylist->Items->isSelected(i));
			if (delete_i && i != myPlaylist->NowPlaying)
				Mpd.Delete(i);
		}
		// if mpd deletes now playing song deletion will be sluggishly slow
		// then so we have to assure it will be deleted at the very end.
		bool delete_np = (delete_all_but_current && current != myPlaylist->NowPlaying)
		              || (!delete_all_but_current && !myPlaylist->Items->isSelected(myPlaylist->NowPlaying));
		if (myPlaylist->isPlaying() && delete_np)
			Mpd.DeleteID(myPlaylist->NowPlayingSong()->GetID());
		ShowMessage("Cropping playlist...");
		if (Mpd.CommitCommandsList())
			ShowMessage("Playlist cropped");
	}
}

bool CropPlaylist::canBeRun() const
{
	return myScreen == myPlaylistEditor;
}

void CropPlaylist::Run()
{
	if (myPlaylistEditor->Playlists->Empty() || myPlaylistEditor->isContentFiltered())
		return;
	bool yes = true;
	if (Config.ask_before_clearing_main_playlist)
		yes = AskYesNoQuestion("Do you really want to crop playlist \"" + myPlaylistEditor->Playlists->Current() + "\"?", TraceMpdStatus);
	if (yes)
	{
		bool delete_all_but_current = !myPlaylistEditor->Content->hasSelected();
		Mpd.StartCommandsList();
		int current = myPlaylistEditor->Content->Choice();
		std::string playlist = locale_to_utf_cpy(myPlaylistEditor->Playlists->Current());
		for (int i = myPlaylistEditor->Content->Size()-1; i >= 0; --i)
		{
			bool delete_i = (delete_all_but_current && i != current)
			             || (!delete_all_but_current && !myPlaylistEditor->Content->isSelected(i));
			if (delete_i)
				Mpd.Delete(playlist, i);
		}
		ShowMessage("Cropping playlist \"%s\"...", myPlaylistEditor->Playlists->Current().c_str());
		if (Mpd.CommitCommandsList())
		{
			ShowMessage("Playlist \"%s\" cropped", myPlaylistEditor->Playlists->Current().c_str());
			// enforce content update
			myPlaylistEditor->Content->Clear();
		}
	}
}

void ClearMainPlaylist::Run()
{
	bool yes = true;
	if (Config.ask_before_clearing_main_playlist)
		yes = AskYesNoQuestion("Do you really want to clear main playlist?", TraceMpdStatus);
	if (yes)
	{
		if (myPlaylist->Items->isFiltered())
		{
			ShowMessage("Deleting filtered items...");
			Mpd.StartCommandsList();
			for (int i = myPlaylist->Items->Size()-1; i >= 0; --i)
				Mpd.Delete((*myPlaylist->Items)[i].GetPosition());
			if (Mpd.CommitCommandsList())
				ShowMessage("Filtered items deleted");
		}
		else
		{
			ShowMessage("Clearing playlist...");
			if (Mpd.ClearPlaylist())
				ShowMessage("Playlist cleared");
		}
	}
}

bool ClearPlaylist::canBeRun() const
{
	return myScreen == myPlaylistEditor;
}

void ClearPlaylist::Run()
{
	if (myPlaylistEditor->Playlists->Empty() || myPlaylistEditor->isContentFiltered())
		return;
	bool yes = true;
	if (Config.ask_before_clearing_main_playlist)
		yes = AskYesNoQuestion("Do you really want to clear playlist \"" + myPlaylistEditor->Playlists->Current() + "\"?", TraceMpdStatus);
	if (yes)
	{
		
		ShowMessage("Clearing playlist \"%s\"...", myPlaylistEditor->Playlists->Current().c_str());
		if (Mpd.ClearPlaylist(locale_to_utf_cpy(myPlaylistEditor->Playlists->Current())))
			ShowMessage("Playlist \"%s\" cleared", myPlaylistEditor->Playlists->Current().c_str());
	}
}

bool SortPlaylist::canBeRun() const
{
	return myScreen == myPlaylist;
}

void SortPlaylist::Run()
{
	myPlaylist->Sort();
}

bool ReversePlaylist::canBeRun() const
{
	return myScreen == myPlaylist;
}

void ReversePlaylist::Run()
{
	myPlaylist->Reverse();
}

void ApplyFilter::Run()
{
	using Global::RedrawHeader;
	using Global::wFooter;
	
	List *mList = myScreen->GetList();
	if (!mList)
		return;
	
	LockStatusbar();
	Statusbar() << fmtBold << "Apply filter: " << fmtBoldEnd;
	wFooter->SetGetStringHelper(StatusbarApplyFilterImmediately);
	wFooter->GetString(mList->GetFilter());
	wFooter->SetGetStringHelper(StatusbarGetStringHelper);
	UnlockStatusbar();
	
	if (mList->isFiltered())
		ShowMessage("Using filter \"%s\"", mList->GetFilter().c_str());
	else
		ShowMessage("Filtering disabled");
	
	if (myScreen == myPlaylist)
	{
		myPlaylist->EnableHighlighting();
		Playlist::ReloadTotalLength = true;
		RedrawHeader = true;
	}
	ListsChangeFinisher();
}

void DisableFilter::Run()
{
	using Global::RedrawHeader;
	using Global::wFooter;
	
	List *mList = myScreen->GetList();
	if (!mList)
		return;
	
	mList->ApplyFilter("");
	
	ShowMessage("Filtering disabled");
	
	if (myScreen == myPlaylist)
	{
		myPlaylist->EnableHighlighting();
		Playlist::ReloadTotalLength = true;
		RedrawHeader = true;
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
	
	LockStatusbar();
	Statusbar() << "Find: ";
	std::string findme = wFooter->GetString();
	UnlockStatusbar();
	
	ShowMessage("Searching...");
	Screen<Scrollpad> *s = static_cast<Screen<Scrollpad> *>(myScreen);
	s->Main()->RemoveFormatting();
	ShowMessage("%s", findme.empty() || s->Main()->SetFormatting(fmtReverse, TO_WSTRING(findme), fmtReverseEnd, 0) ? "Done!" : "No matching patterns found");
	s->Main()->Flush();
}

void FindItemForward::Run()
{
	FindItem(fdForward);
	ListsChangeFinisher();
}

void FindItemBackward::Run()
{
	FindItem(fdBackward);
	ListsChangeFinisher();
}

void NextFoundItem::Run()
{
	List *mList = myScreen->GetList();
	if (!mList)
		return;
	mList->NextFound(Config.wrapped_search);
	ListsChangeFinisher();
}

void PreviousFoundItem::Run()
{
	List *mList = myScreen->GetList();
	if (!mList)
		return;
	mList->PrevFound(Config.wrapped_search);
	ListsChangeFinisher();
}

void ToggleFindMode::Run()
{
	Config.wrapped_search = !Config.wrapped_search;
	ShowMessage("Search mode: %s", Config.wrapped_search ? "Wrapped" : "Normal");
}

void ToggleReplayGainMode::Run()
{
	using Global::wFooter;
	
	if (Mpd.Version() < 16)
	{
		ShowMessage("Replay gain mode control is supported in MPD >= 0.16.0");
		return;
	}
	
	LockStatusbar();
	Statusbar() << "Replay gain mode? [" << fmtBold << 'o' << fmtBoldEnd << "ff/" << fmtBold << 't' << fmtBoldEnd << "rack/" << fmtBold << 'a' << fmtBoldEnd << "lbum]";
	wFooter->Refresh();
	int answer = 0;
	do
	{
		TraceMpdStatus();
		answer = wFooter->ReadKey();
	}
	while (answer != 'o' && answer != 't' && answer != 'a');
	UnlockStatusbar();
	Mpd.SetReplayGainMode(answer == 't' ? MPD::rgmTrack : (answer == 'a' ? MPD::rgmAlbum : MPD::rgmOff));
	ShowMessage("Replay gain mode: %s", Mpd.GetReplayGainMode().c_str());
}

void ToggleSpaceMode::Run()
{
	Config.space_selects = !Config.space_selects;
	ShowMessage("Space mode: %s item", Config.space_selects ? "Select" : "Add");
}

void ToggleAddMode::Run()
{
	Config.ncmpc_like_songs_adding = !Config.ncmpc_like_songs_adding;
	ShowMessage("Add mode: %s", Config.ncmpc_like_songs_adding ? "Add item to playlist, remove if already added" : "Always add item to playlist");
}

void ToggleMouse::Run()
{
	Config.mouse_support = !Config.mouse_support;
	mousemask(Config.mouse_support ? ALL_MOUSE_EVENTS : 0, 0);
	ShowMessage("Mouse support %s", Config.mouse_support ? "enabled" : "disabled");
}

void ToggleBitrateVisibility::Run()
{
	Config.display_bitrate = !Config.display_bitrate;
	ShowMessage("Bitrate visibility %s", Config.display_bitrate ? "enabled" : "disabled");
}

void AddRandomItems::Run()
{
	using Global::wFooter;
	
	LockStatusbar();
	Statusbar() << "Add random? [" << fmtBold << 's' << fmtBoldEnd << "ongs/" << fmtBold << 'a' << fmtBoldEnd << "rtists/al" << fmtBold << 'b' << fmtBoldEnd << "ums] ";
	wFooter->Refresh();
	int answer = 0;
	do
	{
		TraceMpdStatus();
		answer = wFooter->ReadKey();
	}
	while (answer != 's' && answer != 'a' && answer != 'b');
	UnlockStatusbar();
	
	mpd_tag_type tag_type = IntoTagItem(answer);
	std::string tag_type_str = answer == 's' ? "song" : IntoStr(tag_type);
	ToLower(tag_type_str);
	
	LockStatusbar();
	Statusbar() << "Number of random " << tag_type_str << "s: ";
	size_t number = StrToLong(wFooter->GetString());
	UnlockStatusbar();
	if (number && (answer == 's' ? Mpd.AddRandomSongs(number) : Mpd.AddRandomTag(tag_type, number)))
		ShowMessage("%zu random %s%s added to playlist", number, tag_type_str.c_str(), number == 1 ? "" : "s");
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
				ShowMessage("Sort songs by: Modification time");
				break;
			}
		// local browser doesn't support sorting by mtime, so we just skip it.
		case smMTime:
			Config.browser_sort_mode = smCustomFormat;
			ShowMessage("Sort songs by: Custom format");
			break;
		case smCustomFormat:
			Config.browser_sort_mode = smName;
			ShowMessage("Sort songs by: Name");
			break;
	}
	myBrowser->Main()->Sort<CaseInsensitiveSorting>(myBrowser->CurrentDir() != "/");
}

bool ToggleLibraryTagType::canBeRun() const
{
	return (myScreen->ActiveWindow() == myLibrary->Artists)
	    || (myLibrary->Columns() == 2 && myScreen->ActiveWindow() == myLibrary->Albums);
}

void ToggleLibraryTagType::Run()
{
	using Global::wFooter;
	
	LockStatusbar();
	Statusbar() << "Tag type? [" << fmtBold << 'a' << fmtBoldEnd << "rtist/album" << fmtBold << 'A' << fmtBoldEnd << "rtist/" << fmtBold << 'y' << fmtBoldEnd << "ear/" << fmtBold << 'g' << fmtBoldEnd << "enre/" << fmtBold << 'c' << fmtBoldEnd << "omposer/" << fmtBold << 'p' << fmtBoldEnd << "erformer] ";
	wFooter->Refresh();
	int answer = 0;
	do
	{
		TraceMpdStatus();
		answer = wFooter->ReadKey();
	}
	while (answer != 'a' && answer != 'A' && answer != 'y' && answer != 'g' && answer != 'c' && answer != 'p');
	UnlockStatusbar();
	mpd_tag_type new_tagitem = IntoTagItem(answer);
	if (new_tagitem != Config.media_lib_primary_tag)
	{
		Config.media_lib_primary_tag = new_tagitem;
		std::string item_type = IntoStr(Config.media_lib_primary_tag);
		myLibrary->Artists->SetTitle(Config.titles_visibility ? item_type + "s" : "");
		myLibrary->Artists->Reset();
		ToLower(item_type);
		if (myLibrary->Columns() == 2)
		{
			myLibrary->Songs->Clear();
			myLibrary->Albums->Reset();
			myLibrary->Albums->Clear();
			myLibrary->Albums->SetTitle(Config.titles_visibility ? "Albums (sorted by " + item_type + ")" : "");
			myLibrary->Albums->Display();
		}
		else
		{
			myLibrary->Artists->Clear();
			myLibrary->Artists->Display();
		}
		ShowMessage("Switched to list of %s tag", item_type.c_str());
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
	return myScreen->ActiveWindow() == myPlaylist->Items;
}

void SetSelectedItemsPriority::Run()
{
	using Global::wFooter;
	
	assert(myScreen->ActiveWindow() == myPlaylist->Items);
	if (myPlaylist->Items->Empty())
		return;
	
	if (Mpd.Version() < 17)
	{
		ShowMessage("Priorities are supported in MPD >= 0.17.0");
		return;
	}
	
	LockStatusbar();
	Statusbar() << "Set priority [0-255]: ";
	std::string strprio = wFooter->GetString();
	UnlockStatusbar();
	if (!isInteger(strprio.c_str()))
		return;
	int prio = atoi(strprio.c_str());
	if (prio < 0 || prio > 255)
	{
		ShowMessage("Number is out of range");
		return;
	}
	myPlaylist->SetSelectedItemsPriority(prio);
}

void ShowSongInfo::Run()
{
	mySongInfo->SwitchTo();
}

#ifndef HAVE_CURL_CURL_H
bool ShowArtistInfo::canBeRun() const
{
	return false;
}
#endif // NOT HAVE_CURL_CURL_H

void ShowArtistInfo::Run()
{
#	ifdef HAVE_CURL_CURL_H
	if (myScreen == myLastfm || myLastfm->isDownloading())
	{
		myLastfm->SwitchTo();
		return;
	}
	
	std::string artist;
	MPD::Song *s = myScreen->CurrentSong();
	
	if (s)
		artist = s->GetArtist();
	else if (myScreen == myLibrary && myLibrary->Main() == myLibrary->Artists && !myLibrary->Artists->Empty())
		artist = myLibrary->Artists->Current();
	
	if (!artist.empty() && myLastfm->SetArtistInfoArgs(artist, Config.lastfm_preferred_language))
		myLastfm->SwitchTo();
#	endif // HAVE_CURL_CURL_H
}

void ShowLyrics::Run()
{
	myLyrics->SwitchTo();
}

void Quit::Run()
{
	ExitMainLoop = true;
}

void NextScreen::Run()
{
	using Global::myOldScreen;
	using Global::myPrevScreen;
	
	if (Config.screen_switcher_previous)
	{
		if (myScreen->isTabbable())
			myPrevScreen->SwitchTo();
		else
			myOldScreen->SwitchTo();
	}
	else if (!Config.screens_seq.empty())
	{
		std::list<BasicScreen *>::const_iterator screen = std::find(Config.screens_seq.begin(), Config.screens_seq.end(), myScreen);
		if (++screen == Config.screens_seq.end())
			Config.screens_seq.front()->SwitchTo();
		else
			(*screen)->SwitchTo();
	}
}

void PreviousScreen::Run()
{
	using Global::myOldScreen;
	using Global::myPrevScreen;
	
	if (Config.screen_switcher_previous)
	{
		if (myScreen->isTabbable())
			myPrevScreen->SwitchTo();
		else
			myOldScreen->SwitchTo();
	}
	else if (!Config.screens_seq.empty())
	{
		std::list<BasicScreen *>::const_iterator screen = std::find(Config.screens_seq.begin(), Config.screens_seq.end(), myScreen);
		if (screen == Config.screens_seq.begin())
			Config.screens_seq.back()->SwitchTo();
		else
			(*--screen)->SwitchTo();
	}
}

#ifdef HAVE_TAGLIB_H
bool ShowHelp::canBeRun() const
{
	return myScreen != myTinyTagEditor;
}
#endif // HAVE_TAGLIB_H

void ShowHelp::Run()
{
	myHelp->SwitchTo();
}

#ifdef HAVE_TAGLIB_H
bool ShowPlaylist::canBeRun() const
{
	return myScreen != myTinyTagEditor;
}
#endif // HAVE_TAGLIB_H

void ShowPlaylist::Run()
{
	myPlaylist->SwitchTo();
}

#ifdef HAVE_TAGLIB_H
bool ShowBrowser::canBeRun() const
{
	return myScreen != myTinyTagEditor;
}
#endif // HAVE_TAGLIB_H

void ShowBrowser::Run()
{
	myBrowser->SwitchTo();
}

#ifdef HAVE_TAGLIB_H
bool ShowSearchEngine::canBeRun() const
{
	return myScreen != myTinyTagEditor;
}
#endif // HAVE_TAGLIB_H

void ShowSearchEngine::Run()
{
	mySearcher->SwitchTo();
}

#ifdef HAVE_TAGLIB_H
bool ShowMediaLibrary::canBeRun() const
{
	return myScreen != myTinyTagEditor;
}
#endif // HAVE_TAGLIB_H

void ShowMediaLibrary::Run()
{
	myLibrary->SwitchTo();
}

#ifdef HAVE_TAGLIB_H
bool ShowPlaylistEditor::canBeRun() const
{
	return myScreen != myTinyTagEditor;
}
#endif // HAVE_TAGLIB_H

void ShowPlaylistEditor::Run()
{
	myPlaylistEditor->SwitchTo();
}

bool ShowTagEditor::canBeRun() const
{
#	ifdef HAVE_TAGLIB_H
	return myScreen != myTinyTagEditor;
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void ShowTagEditor::Run()
{
#	ifdef HAVE_TAGLIB_H
	if (isMPDMusicDirSet())
		myTagEditor->SwitchTo();
#	endif // HAVE_TAGLIB_H
}

bool ShowOutputs::canBeRun() const
{
#	ifdef ENABLE_OUTPUTS
#	ifdef HAVE_TAGLIB_H
	return myScreen != myTinyTagEditor;
#	else
	return true;
#	endif // HAVE_TAGLIB_H
#	else
	return false;
#	endif // ENABLE_OUTPUTS
}

void ShowOutputs::Run()
{
#	ifdef ENABLE_OUTPUTS
	myOutputs->SwitchTo();
#	endif // ENABLE_OUTPUTS
}

bool ShowVisualizer::canBeRun() const
{
#	ifdef ENABLE_OUTPUTS
#	ifdef HAVE_TAGLIB_H
	return myScreen != myTinyTagEditor;
#	else
	return true;
#	endif // HAVE_TAGLIB_H
#	else
	return false;
#	endif // ENABLE_OUTPUTS
}

void ShowVisualizer::Run()
{
#	ifdef ENABLE_VISUALIZER
	myVisualizer->SwitchTo();
#	endif // ENABLE_VISUALIZER
}

bool ShowClock::canBeRun() const
{
#	ifdef ENABLE_CLOCK
#	ifdef HAVE_TAGLIB_H
	return myScreen != myTinyTagEditor;
#	else
	return true;
#	endif // HAVE_TAGLIB_H
#	else
	return false;
#	endif // ENABLE_CLOCK
}

void ShowClock::Run()
{
#	ifdef ENABLE_CLOCK
	myClock->SwitchTo();
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
	myServerInfo->SwitchTo();
}

Action *Action::Get(ActionType at)
{
	if (Actions.empty())
	{
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
		insertAction(new JumpToParentDir());
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
		insertAction(new ToggleSeparatorsInPlaylist());
		insertAction(new ToggleLyricsFetcher());
		insertAction(new ToggleFetchingLyricsInBackground());
		insertAction(new ToggleAutoCenter());
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
		insertAction(new DeselectItems());
		insertAction(new SelectAlbum());
		insertAction(new AddSelectedItems());
		insertAction(new CropMainPlaylist());
		insertAction(new CropPlaylist());
		insertAction(new ClearMainPlaylist());
		insertAction(new ClearPlaylist());
		insertAction(new SortPlaylist());
		insertAction(new ReversePlaylist());
		insertAction(new ApplyFilter());
		insertAction(new DisableFilter());
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
		insertAction(new ShowSongInfo());
		insertAction(new ShowArtistInfo());
		insertAction(new ShowLyrics());
		insertAction(new Quit());
		insertAction(new NextScreen());
		insertAction(new PreviousScreen());
		insertAction(new ShowHelp());
		insertAction(new ShowPlaylist());
		insertAction(new ShowBrowser());
		insertAction(new ShowSearchEngine());
		insertAction(new ShowMediaLibrary());
		insertAction(new ShowPlaylistEditor());
		insertAction(new ShowTagEditor());
		insertAction(new ShowOutputs());
		insertAction(new ShowVisualizer());
		insertAction(new ShowClock());
		insertAction(new ShowServerInfo());
	}
	return Actions[at];
}
