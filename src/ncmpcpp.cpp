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

#include <cerrno>
#include <clocale>
#include <csignal>
#include <cstring>
#include <sys/time.h>

#include <iostream>
#include <fstream>
#include <stdexcept>

#include "mpdpp.h"
#include "ncmpcpp.h"

#include "browser.h"
#include "charset.h"
#include "clock.h"
#include "display.h"
#include "global.h"
#include "help.h"
#include "helpers.h"
#include "media_library.h"
#include "sel_items_adder.h"
#include "server_info.h"
#include "lyrics.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "settings.h"
#include "song.h"
#include "song_info.h"
#include "lastfm.h"
#include "outputs.h"
#include "status.h"
#include "tag_editor.h"
#include "tiny_tag_editor.h"
#include "visualizer.h"

#define CHECK_PLAYLIST_FOR_FILTERING									\
			if (myPlaylist->Items->isFiltered())						\
			{										\
				ShowMessage("%s", MPD::Message::FunctionDisabledFilteringEnabled);	\
				continue;								\
			}

#define CHECK_MPD_MUSIC_DIR										\
			if (Config.mpd_music_dir.empty())						\
			{										\
				ShowMessage("configuration variable mpd_music_dir is not set!");	\
				continue;								\
			}

using namespace Global;
using namespace MPD;

BasicScreen *Global::myScreen;
BasicScreen *Global::myOldScreen;
BasicScreen *Global::myPrevScreen;
BasicScreen *Global::myLockedScreen;
BasicScreen *Global::myInactiveScreen;

Window *Global::wHeader;
Window *Global::wFooter;

size_t Global::MainStartY;
size_t Global::MainHeight;

bool Global::BlockItemListUpdate = 0;

bool Global::MessagesAllowed = 0;
bool Global::SeekingInProgress = 0;
bool Global::RedrawHeader = 1;

namespace
{
	std::ofstream errorlog;
	std::streambuf *cerr_buffer;
	
	bool design_changed = 0;
	bool order_resize = 0;
	size_t header_height, footer_start_y, footer_height;
	
	void check_screen_min_size()
	{
		if (COLS < 20 || MainHeight < 3)
		{
			DestroyScreen();
			std::cout << "Screen is too small!\n";
			exit(1);
		}
	}
	
	void set_resize_flags()
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
		
#		ifdef HAVE_CURL_CURL_H
		myLastfm->hasToBeResized = 1;
#		endif // HAVE_CURL_CURL_H
		
#		ifdef HAVE_TAGLIB_H
		myTinyTagEditor->hasToBeResized = 1;
		myTagEditor->hasToBeResized = 1;
#		endif // HAVE_TAGLIB_H
		
#		ifdef ENABLE_VISUALIZER
		myVisualizer->hasToBeResized = 1;
#		endif // ENABLE_VISUALIZER
		
#		ifdef ENABLE_OUTPUTS
		myOutputs->hasToBeResized = 1;
#		endif // ENABLE_OUTPUTS
		
#		ifdef ENABLE_CLOCK
		myClock->hasToBeResized = 1;
#		endif // ENABLE_CLOCK
	}
	
	void resize_screen()
	{
		order_resize = 0;
		
#		if defined(USE_PDCURSES)
		resize_term(0, 0);
#		else
		// update internal screen dimensions
		if (!design_changed)
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
#		endif
		
		RedrawHeader = 1;
		MainHeight = LINES-(Config.new_design ? 7 : 4);
		
		check_screen_min_size();
		
		if (!Config.header_visibility)
			MainHeight += 2;
		if (!Config.statusbar_visibility)
			MainHeight++;
		
		set_resize_flags();
		
		ApplyToVisibleWindows(&BasicScreen::Resize);
		
		if (Config.header_visibility || Config.new_design)
			wHeader->Resize(COLS, header_height);
		
		footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
		wFooter->MoveTo(0, footer_start_y);
		wFooter->Resize(COLS, Config.statusbar_visibility ? 2 : 1);
		
		ApplyToVisibleWindows(&BasicScreen::Refresh);
		RedrawStatusbar = 1;
		StatusChanges changes;
		if (!Mpd.isPlaying() || design_changed)
		{
			changes.PlayerState = 1;
			if (design_changed)
				changes.Volume = 1;
		}
		// Note: routines for drawing separator if alternative user
		// interface is active and header is hidden are placed in
		// NcmpcppStatusChanges.StatusFlags
		changes.StatusFlags = 1; // force status update
		NcmpcppStatusChanged(&Mpd, changes, 0);
		if (design_changed)
		{
			RedrawStatusbar = 1;
			NcmpcppStatusChanged(&Mpd, StatusChanges(), 0);
			design_changed = 0;
			ShowMessage("User interface: %s", Config.new_design ? "Alternative" : "Classic");
		}
		wFooter->Refresh();
		refresh();
	}
	
#	if !defined(WIN32)
	void sighandler(int signal)
	{
		if (signal == SIGPIPE)
		{
			ShowMessage("Broken pipe signal caught!");
		}
		else if (signal == SIGWINCH)
		{
			order_resize = 1;
		}
	}
#	endif // !WIN32
	
	void do_at_exit()
	{
		// restore old cerr buffer
		std::cerr.rdbuf(cerr_buffer);
		errorlog.close();
		Mpd.Disconnect();
#		ifndef USE_PDCURSES // destroying screen somehow crashes pdcurses
		DestroyScreen();
#		endif // USE_PDCURSES
		WindowTitle("");
	}
}

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	
	Config.CheckForCommandLineConfigFilePath(argv, argc);
	
	Config.SetDefaults();
	Key.SetDefaults();
	Config.Read();
	Key.Read();
	
	if (getenv("MPD_HOST"))
		Mpd.SetHostname(getenv("MPD_HOST"));
	if (getenv("MPD_PORT"))
		Mpd.SetPort(atoi(getenv("MPD_PORT")));
	
	if (Config.mpd_host != "localhost")
		Mpd.SetHostname(Config.mpd_host);
	if (Config.mpd_port != 6600)
		Mpd.SetPort(Config.mpd_port);
	
	Mpd.SetTimeout(Config.mpd_connection_timeout);
	Mpd.SetIdleEnabled(Config.enable_idle_notifications);
	
	if (argc > 1)
		ParseArgv(argc, argv);
	
	if (!ConnectToMPD())
		exit(1);
	
	CreateDir(Config.ncmpcpp_directory);
	
	// always execute these commands, even if ncmpcpp use exit function
	atexit(do_at_exit);
	
	// redirect std::cerr output to ~/.ncmpcpp/error.log file
	errorlog.open((Config.ncmpcpp_directory + "error.log").c_str(), std::ios::app);
	cerr_buffer = std::cerr.rdbuf();
	std::cerr.rdbuf(errorlog.rdbuf());
	
	InitScreen("ncmpcpp ver. "VERSION, Config.colors_enabled);
	
	bool real_statusbar_visibility = Config.statusbar_visibility;
	
	if (!Config.titles_visibility)
		wattron(stdscr, COLOR_PAIR(Config.main_color));
	
	if (Config.new_design)
		Config.statusbar_visibility = 0;
	
	SetWindowsDimensions(header_height, footer_start_y, footer_height);
	check_screen_min_size();
	
	wHeader = new Window(0, 0, COLS, header_height, "", Config.header_color, brNone);
	if (Config.header_visibility || Config.new_design)
		wHeader->Display();
	
	wFooter = new Window(0, footer_start_y, COLS, footer_height, "", Config.statusbar_color, brNone);
	wFooter->SetTimeout(ncmpcpp_window_timeout);
	wFooter->SetGetStringHelper(StatusbarGetStringHelper);
	if (Mpd.SupportsIdle())
		wFooter->AddFDCallback(Mpd.GetFD(), StatusbarMPDCallback);
	wFooter->CreateHistory();
	
	// initialize screens to browser as default previous screen
	myScreen = myBrowser;
	myPrevScreen = myBrowser;
	myOldScreen = myBrowser;

	// go to playlist
	myPlaylist->SwitchTo();
	myPlaylist->UpdateTimer();
	
	// go to startup screen
	if (Config.startup_screen != myScreen)
		Config.startup_screen->SwitchTo();
	
	Mpd.SetStatusUpdater(NcmpcppStatusChanged, 0);
	Mpd.SetErrorHandler(NcmpcppErrorCallback, 0);
	
	// local variables
	int input = 0;
	
	bool main_exit = 0;
	bool title_allowed = !Config.display_screens_numbers_on_start;
	
	std::string screen_title;
	
	timeval past = { 0, 0 };
	// local variables end
	
#	ifndef WIN32
	signal(SIGPIPE, sighandler);
	signal(SIGWINCH, sighandler);
#	endif // !WIN32
	
	mouseinterval(0);
	if (Config.mouse_support)
		mousemask(ALL_MOUSE_EVENTS, 0);
	
	Mpd.OrderDataFetching();
	if (Config.jump_to_now_playing_song_at_start)
	{
		TraceMpdStatus();
		if (myPlaylist->isPlaying())
			myPlaylist->Items->Highlight(myPlaylist->NowPlaying);
	}
	
	while (!main_exit)
	{
		if (!Mpd.Connected())
		{
			if (!wFooter->FDCallbacksListEmpty())
				wFooter->ClearFDCallbacksList();
			ShowMessage("Attempting to reconnect...");
			if (Mpd.Connect())
			{
				ShowMessage("Connected to %s!", Mpd.GetHostname().c_str());
				if (Mpd.SupportsIdle())
				{
					wFooter->AddFDCallback(Mpd.GetFD(), StatusbarMPDCallback);
					Mpd.OrderDataFetching(); // we need info about new connection
				}
				// we want everything updated after reconnect, no blocking.
				Playlist::BlockUpdate = 0;
				Global::BlockItemListUpdate = 0;
				MessagesAllowed = 0;
				UpdateStatusImmediately = 1;
#				ifdef ENABLE_VISUALIZER
				myVisualizer->ResetFD();
				if (myScreen == myVisualizer)
					myVisualizer->SetFD();
				myVisualizer->FindOutputID();
#				endif // ENABLE_VISUALIZER
			}
		}
		
		TraceMpdStatus();
		
		MessagesAllowed = 1;
		
		if (order_resize)
			resize_screen();
		
		// header stuff
		if (((Timer.tv_sec == past.tv_sec && Timer.tv_usec >= past.tv_usec+500000) || Timer.tv_sec > past.tv_sec)
		&&   (myScreen == myPlaylist || myScreen == myBrowser || myScreen == myLyrics)
		   )
		{
			RedrawHeader = 1;
			gettimeofday(&past, 0);
		}
		if (Config.header_visibility && RedrawHeader)
		{
			if (title_allowed)
			{
				if (Config.new_design)
				{
					std::basic_string<my_char_t> title = myScreen->Title();
					*wHeader << XY(0, 3) << wclrtoeol;
					*wHeader << fmtBold << Config.alternative_ui_separator_color;
					mvwhline(wHeader->Raw(), 2, 0, 0, COLS);
					mvwhline(wHeader->Raw(), 4, 0, 0, COLS);
					*wHeader << XY((COLS-Window::Length(title))/2, 3);
					*wHeader << Config.header_color << title << clEnd;
					*wHeader << clEnd << fmtBoldEnd;
				}
				else
					*wHeader << XY(0, 0) << wclrtoeol << fmtBold << myScreen->Title() << fmtBoldEnd;
			}
			else
			{
				*wHeader << XY(0, Config.new_design ? 3 : 0)
				<< fmtBold << char(Key.Help[0]) << fmtBoldEnd << ":Help  "
				<< fmtBold << char(Key.Playlist[0]) << fmtBoldEnd << ":Playlist  "
				<< fmtBold << char(Key.Browser[0]) << fmtBoldEnd << ":Browse  "
				<< fmtBold << char(Key.SearchEngine[0]) << fmtBoldEnd << ":Search  "
				<< fmtBold << char(Key.MediaLibrary[0]) << fmtBoldEnd << ":Library  "
				<< fmtBold << char(Key.PlaylistEditor[0]) << fmtBoldEnd << ":Playlist editor";
#				ifdef HAVE_TAGLIB_H
				*wHeader << "  " << fmtBold << char(Key.TagEditor[0]) << fmtBoldEnd << ":Tag editor";
#				endif // HAVE_TAGLIB_H
#				ifdef ENABLE_VISUALIZER
				*wHeader << "  " << fmtBold << char(Key.Visualizer[0]) << fmtBoldEnd << ":Music visualizer";
#				endif // ENABLE_VISUALIZER
#				ifdef ENABLE_OUTPUTS
				*wHeader << "  " << fmtBold << char(Key.Outputs[0]) << fmtBoldEnd << ":Outputs";
#				endif // ENABLE_OUTPUTS
#				ifdef ENABLE_CLOCK
				*wHeader << "  " << fmtBold << char(Key.Clock[0]) << fmtBoldEnd << ":Clock";
#				endif // ENABLE_CLOCK
				if (Config.new_design)
				{
					*wHeader << fmtBold << Config.alternative_ui_separator_color;
					mvwhline(wHeader->Raw(), 2, 0, 0, COLS);
					mvwhline(wHeader->Raw(), 4, 0, 0, COLS);
					*wHeader << clEnd << fmtBoldEnd;
				}
			}
			if (!Config.new_design)
			{
				*wHeader << Config.volume_color;
				*wHeader << XY(wHeader->GetWidth()-VolumeState.length(), 0) << VolumeState;
				*wHeader << clEnd;
			}
			wHeader->Refresh();
			RedrawHeader = 0;
		}
		// header stuff end
		
		if (input != ERR)
			myScreen->RefreshWindow();
		wFooter->ReadKey(input);
		
		if (input == ERR)
			continue;
		
		if (!title_allowed)
			RedrawHeader = 1;
		title_allowed = 1;
		
		// key mapping beginning
		
		if (Keypressed(input, Key.Up))
		{
			myScreen->Scroll(wUp, Key.Up);
		}
		else if (Keypressed(input, Key.Down))
		{
			myScreen->Scroll(wDown, Key.Down);
		}
		else if (Keypressed(input, Key.UpAlbum)  || Keypressed(input, Key.DownAlbum)
		     ||  Keypressed(input, Key.UpArtist) || Keypressed(input, Key.DownArtist))
		{
			Menu<MPD::Song> *songs = NULL;
			if (myScreen == myPlaylist && !myPlaylist->Items->Empty())
				songs = myPlaylist->Items;
			else if (myScreen->ActiveWindow() == myPlaylistEditor->Content)
				songs = myPlaylistEditor->Content;
			else if (myScreen == mySearcher)
				mySearcher->Scroll(input);

			if (songs && !songs->Empty())
			{
				size_t pos = songs->Choice();
				if (Keypressed(input, Key.UpAlbum))
				{
					std::string album = songs->at(pos).GetAlbum();
					while (pos > 0)
						if (songs->at(--pos).GetAlbum() != album)
							break;
				}
				else if (Keypressed(input, Key.DownAlbum))
				{
					std::string album = songs->at(pos).GetAlbum();
					while (pos < songs->Size() - 1)
						if (songs->at(++pos).GetAlbum() != album)
							break;
				}
				else if (Keypressed(input, Key.UpArtist))
				{
					std::string artist = songs->at(pos).GetArtist();
					while (pos > 0)
						if (songs->at(--pos).GetArtist() != artist)
							break;
				}
				else if (Keypressed(input, Key.DownArtist))
				{
					std::string artist = songs->at(pos).GetArtist();
					while (pos < songs->Size() - 1)
						if (songs->at(++pos).GetArtist() != artist)
							break;
				}
				songs->Highlight(pos);
			}
		}
		else if (Keypressed(input, Key.PageUp))
		{
			myScreen->Scroll(wPageUp, Key.PageUp);
		}
		else if (Keypressed(input, Key.PageDown))
		{
			myScreen->Scroll(wPageDown, Key.PageDown);
		}
		else if (Keypressed(input, Key.Home))
		{
			myScreen->Scroll(wHome);
		}
		else if (Keypressed(input, Key.End))
		{
			myScreen->Scroll(wEnd);
		}
		else if (Config.mouse_support && input == KEY_MOUSE)
		{
			static MEVENT mouse_event, old_mouse_event;
			old_mouse_event = mouse_event;
			getmouse(&mouse_event);
			// workaround shitty ncurses behavior introduced in >=5.8, when we mysteriously get
			// a few times after ncmpcpp startup 2^27 code instead of BUTTON{1,3}_RELEASED. since that
			// 2^27 thing shows constantly instead of BUTTON2_PRESSED, it was redefined to be recognized
			// as BUTTON2_PRESSED. but clearly we don't want to trigger behavior bound to BUTTON2
			// after BUTTON{1,3} was pressed. so, here is the workaround: if last event was BUTTON{1,3}_PRESSED,
			// we MUST get BUTTON{1,3}_RELEASED afterwards. if we get BUTTON2_PRESSED, erroneus behavior
			// is about to occur and we need to prevent that.
			if (old_mouse_event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED) && mouse_event.bstate & BUTTON2_PRESSED)
				continue;
			if (mouse_event.bstate & BUTTON1_PRESSED
			&&  mouse_event.y == LINES-(Config.statusbar_visibility ? 2 : 1)
			   ) // progressbar
			{
				if (!myPlaylist->isPlaying())
					continue;
				Mpd.Seek(Mpd.GetTotalTime()*mouse_event.x/double(COLS));
				UpdateStatusImmediately = 1;
			}
			else if (mouse_event.bstate & BUTTON1_PRESSED
			     &&	 (Config.statusbar_visibility || Config.new_design)
			     &&	 Mpd.isPlaying()
			     &&	 mouse_event.y == (Config.new_design ? 1 : LINES-1) && mouse_event.x < 9
				) // playing/paused
			{
				Mpd.Toggle();
				UpdateStatusImmediately = 1;
			}
			else if ((mouse_event.bstate & BUTTON2_PRESSED || mouse_event.bstate & BUTTON4_PRESSED)
			     &&	 Config.header_visibility
			     &&	 mouse_event.y == 0 && size_t(mouse_event.x) > COLS-VolumeState.length()
				) // volume
			{
				if (mouse_event.bstate & BUTTON2_PRESSED)
					Mpd.SetVolume(Mpd.GetVolume()-2);
				else
					Mpd.SetVolume(Mpd.GetVolume()+2);
			}
			else if (mouse_event.bstate & (BUTTON1_PRESSED | BUTTON2_PRESSED | BUTTON3_PRESSED | BUTTON4_PRESSED))
				myScreen->MouseButtonPressed(mouse_event);
		}
		else if (Keypressed(input, Key.ToggleInterface))
		{
			Config.new_design = !Config.new_design;
			Config.statusbar_visibility = Config.new_design ? 0 : real_statusbar_visibility;
			SetWindowsDimensions(header_height, footer_start_y, footer_height);
			UnlockProgressbar();
			UnlockStatusbar();
			design_changed = 1;
			resize_screen();
		}
		else if (Keypressed(input, Key.GoToParentDir)
		     &&  (myScreen == myBrowser
#			ifdef HAVE_TAGLIB_H
		        || myScreen == myTagEditor
#			endif // HAVE_TAGLIB_H
			 )
			)
		{
			if (myScreen == myBrowser && myBrowser->CurrentDir() != "/")
			{
				myBrowser->Main()->Reset();
				myBrowser->EnterPressed();
			}
#			ifdef HAVE_TAGLIB_H
			else if (myScreen->ActiveWindow() == myTagEditor->Dirs && myTagEditor->CurrentDir() != "/")
			{
				myTagEditor->Dirs->Reset();
				myTagEditor->EnterPressed();
			}
#			endif // HAVE_TAGLIB_H
		}
		else if (Keypressed(input, Key.Enter))
		{
			myScreen->EnterPressed();
		}
		else if (Keypressed(input, Key.Space))
		{
			myScreen->SpacePressed();
		}
		else if (Keypressed(input, Key.PrevColumn) && SwitchToPrevColumn(myScreen)) { }
		else if (Keypressed(input, Key.NextColumn) && SwitchToNextColumn(myScreen)) { }
		else if (Keypressed(input, Key.PrevColumn) && myLockedScreen && myInactiveScreen
		     &&  myLockedScreen != myScreen && myScreen->isMergable())
		{
			if (myScreen != myLockedScreen)
			{
				myInactiveScreen = myScreen;
				myScreen = myLockedScreen;
				RedrawHeader = 1;
			}
		}
		else if (Keypressed(input, Key.NextColumn) && myLockedScreen && myInactiveScreen
		     &&  myLockedScreen == myScreen && myScreen->isMergable())
		{
			if (myScreen == myLockedScreen)
			{
				myScreen = myInactiveScreen;
				myInactiveScreen = myLockedScreen;
				RedrawHeader = 1;
			}
		}
		else if (Keypressed(input, Key.VolumeUp))
		{
			Mpd.SetVolume(Mpd.GetVolume()+1);
		}
		else if (Keypressed(input, Key.VolumeDown))
		{
			Mpd.SetVolume(Mpd.GetVolume()-1);
		}
		else if (Keypressed(input, Key.Delete))
		{
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
						myPlaylist->FixPositions(list.front());
						ShowMessage("Selected items deleted!");
					}
				}
				else
				{
					wFooter->SetTimeout(50);
					bool adjust_np = myPlaylist->NowPlaying > int(myPlaylist->CurrentSong()->GetPosition());
					while (!myPlaylist->Items->Empty() && Keypressed(input, Key.Delete))
					{
						size_t id = myPlaylist->Items->Choice();
						TraceMpdStatus();
						Playlist::BlockUpdate = 1;
						myPlaylist->UpdateTimer();
						// needed for keeping proper position of now playing song.
						if (adjust_np)
							--myPlaylist->NowPlaying;
						if (Mpd.DeleteID(myPlaylist->CurrentSong()->GetID()))
						{
							myPlaylist->Items->DeleteOption(id);
							myPlaylist->Items->Refresh();
							wFooter->ReadKey(input);
						}
						else
							break;
					}
					myPlaylist->FixPositions(myPlaylist->Items->Choice());
					wFooter->SetTimeout(ncmpcpp_window_timeout);
				}
			}
			else if (
				 (myScreen == myBrowser && !myBrowser->Main()->Empty() && myBrowser->CurrentDir() == "/" && myBrowser->Main()->Current().type == itPlaylist)
			||       (myScreen->ActiveWindow() == myPlaylistEditor->Playlists)
				)
			{
				std::string name = myScreen == myBrowser ? myBrowser->Main()->Current().name : myPlaylistEditor->Playlists->Current();
				LockStatusbar();
				Statusbar() << "Delete playlist \"" << Shorten(TO_WSTRING(name), COLS-28) << "\" ? [" << fmtBold << 'y' << fmtBoldEnd << '/' << fmtBold << 'n' << fmtBoldEnd << "]";
				wFooter->Refresh();
				int answer = 0;
				do
				{
					TraceMpdStatus();
					wFooter->ReadKey(answer);
				}
				while (answer != 'y' && answer != 'n');
				UnlockStatusbar();
				if (answer == 'y')
				{
					if (Mpd.DeletePlaylist(locale_to_utf_cpy(name)))
					{
						static const char msg[] = "Playlist \"%s\" deleted!";
						ShowMessage(msg, Shorten(TO_WSTRING(name), COLS-static_strlen(msg)).c_str());
						if (myBrowser->Main() && !myBrowser->isLocal() && myBrowser->CurrentDir() == "/")
							myBrowser->GetDirectory("/");
					}
				}
				else
					ShowMessage("Aborted!");
				if (myPlaylistEditor->Main()) // check if initialized
					myPlaylistEditor->Playlists->Clear(); // make playlists list update itself
			}
#			ifndef WIN32
			else if (myScreen == myBrowser && !myBrowser->Main()->Empty())
			{
				if (!myBrowser->isLocal())
					CHECK_MPD_MUSIC_DIR;
				
				MPD::Item &item = myBrowser->Main()->Current();
				
				if (item.type == itSong && !Config.allow_physical_files_deletion)
				{
					ShowMessage("Deleting files is disabled by default, see man page for more details");
					continue;
				}
				if (item.type == itDirectory && !Config.allow_physical_directories_deletion)
				{
					ShowMessage("Deleting directories is disabled by default, see man page for more details");
					continue;
				}
				if (item.type == itDirectory && item.song) // parent dir
					continue;
				
				std::string name = item.type == itSong ? item.song->GetName() : item.name;
				LockStatusbar();
				if (myBrowser->Main()->hasSelected())
					Statusbar() << "Delete selected items ? [" << fmtBold << 'y' << fmtBoldEnd << '/' << fmtBold << 'n' << fmtBoldEnd << "] ";
				else
					Statusbar() << "Delete " << (item.type == itSong ? "file" : item.type == itDirectory ? "directory" : "playlist") << " \"" << Shorten(TO_WSTRING(name), COLS-30) << "\" ? [" << fmtBold << 'y' << fmtBoldEnd << '/' << fmtBold << 'n' << fmtBoldEnd << "] ";
				wFooter->Refresh();
				int answer = 0;
				do
				{
					TraceMpdStatus();
					wFooter->ReadKey(answer);
				}
				while (answer != 'y' && answer != 'n');
				UnlockStatusbar();
				if (answer == 'y')
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
							static const char msg[] = "\"%s\" deleted!";
							ShowMessage(msg, Shorten(TO_WSTRING(name), COLS-static_strlen(msg)).c_str());
						}
						else
						{
							static const char msg[] = "Couldn't remove \"%s\": %s";
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
					ShowMessage("Aborted!");
				
			}
#			endif // !WIN32
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
					ShowMessage("Selected items deleted from playlist \"%s\"!", myPlaylistEditor->Playlists->Current().c_str());
				}
				else
				{
					wFooter->SetTimeout(50);
					locale_to_utf(myPlaylistEditor->Playlists->Current());
					while (!myPlaylistEditor->Content->Empty() && Keypressed(input, Key.Delete))
					{
						TraceMpdStatus();
						myPlaylist->UpdateTimer();
						Mpd.Delete(myPlaylistEditor->Playlists->Current(), myPlaylistEditor->Content->Choice());
						myPlaylistEditor->Content->DeleteOption(myPlaylistEditor->Content->Choice());
						myPlaylistEditor->Content->Refresh();
						wFooter->ReadKey(input);
					}
					utf_to_locale(myPlaylistEditor->Playlists->Current());
					wFooter->SetTimeout(ncmpcpp_window_timeout);
				}
			}
		}
		else if (Keypressed(input, Key.Replay))
		{
			if (Mpd.isPlaying())
			{
				Mpd.Seek(0);
				UpdateStatusImmediately = 1;
			}
		}
		else if (Keypressed(input, Key.Prev))
		{
			Mpd.Prev();
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.Next))
		{
			Mpd.Next();
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.Pause))
		{
			Mpd.Toggle();
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.SavePlaylist))
		{
			LockStatusbar();
			Statusbar() << "Save playlist as: ";
			std::string playlist_name = wFooter->GetString();
			std::string real_playlist_name = locale_to_utf_cpy(playlist_name);
			UnlockStatusbar();
			if (playlist_name.find("/") != std::string::npos)
			{
				ShowMessage("Playlist name cannot contain slashes!");
				continue;
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
						ShowMessage("Playlist saved as: %s", playlist_name.c_str());
						if (myPlaylistEditor->Main()) // check if initialized
							myPlaylistEditor->Playlists->Clear(); // make playlist's list update itself
					}
					else if (result == MPD_SERVER_ERROR_EXIST)
					{
						LockStatusbar();
						Statusbar() << "Playlist already exists, overwrite: " << playlist_name << " ? [" << fmtBold << 'y' << fmtBoldEnd << '/' << fmtBold << 'n' << fmtBoldEnd << "] ";
						wFooter->Refresh();
						int answer = 0;
						while (answer != 'y' && answer != 'n')
						{
							TraceMpdStatus();
							wFooter->ReadKey(answer);
						}
						UnlockStatusbar();
						
						if (answer == 'y')
						{
							Mpd.DeletePlaylist(real_playlist_name);
							if (Mpd.SavePlaylist(real_playlist_name) == MPD_ERROR_SUCCESS)
								ShowMessage("Playlist overwritten!");
						}
						else
							ShowMessage("Aborted!");
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
		else if (Keypressed(input, Key.Stop))
		{
			Mpd.Stop();
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.MvSongUp))
		{
			if (myScreen == myPlaylist && myPlaylist->SortingInProgress())
				myPlaylist->AdjustSortOrder(input);
			else if (myScreen == myPlaylist && !myPlaylist->Items->Empty())
			{
				CHECK_PLAYLIST_FOR_FILTERING;
				// remove search results as we may move them to different positions, but
				// search rememebers positions and may point to wrong ones after that.
				myPlaylist->Items->Search("");
				wFooter->SetTimeout(50);
				if (myPlaylist->Items->hasSelected())
				{
					std::vector<size_t> list;
					myPlaylist->Items->GetSelected(list);
					std::vector<size_t> origs(list);
					
					// NOTICE: since ncmpcpp only pretends to move the songs until the key is
					// released, mpd doesn't know about the change while the songs are moved
					// so wee need to block playlist update for this time and also if one of
					// the songs being moved is currently playing, now playing update to prevent
					// mpd from 'updating' and thus showing wrong position
					
					bool modify_now_playing = 0;
					for (std::vector<size_t>::iterator it = list.begin(); it != list.end(); ++it)
					{
						if (*it == size_t(myPlaylist->NowPlaying) && list.front() > 0)
						{
							modify_now_playing = 1;
							Playlist::BlockNowPlayingUpdate = 1;
							break;
						}
					}
					
					while (Keypressed(input, Key.MvSongUp) && list.front() > 0)
					{
						TraceMpdStatus();
						Playlist::BlockUpdate = 1;
						myPlaylist->UpdateTimer();
						if (modify_now_playing)
							--myPlaylist->NowPlaying;
						for (std::vector<size_t>::iterator it = list.begin(); it != list.end(); ++it)
						{
							--*it;
							myPlaylist->Items->at((*it)+1).SetPosition(*it);
							myPlaylist->Items->at(*it).SetPosition((*it)+1);
							myPlaylist->Items->Swap(*it, (*it)+1);
						}
						myPlaylist->Items->Highlight(list[(list.size()-1)/2]);
						myPlaylist->Items->Refresh();
						wFooter->ReadKey(input);
					}
					Playlist::BlockNowPlayingUpdate = 0;
					Mpd.StartCommandsList();
					for (size_t i = 0; i < list.size(); ++i)
						Mpd.Move(origs[i], list[i]);
					Mpd.CommitCommandsList();
				}
				else
				{
					size_t from, to;
					from = to = myPlaylist->Items->Choice();
					bool modify_now_playing = from == size_t(myPlaylist->NowPlaying);
					if (modify_now_playing)
						Playlist::BlockNowPlayingUpdate = 1;
					while (Keypressed(input, Key.MvSongUp) && to > 0)
					{
						TraceMpdStatus();
						Playlist::BlockUpdate = 1;
						myPlaylist->UpdateTimer();
						if (modify_now_playing)
							--myPlaylist->NowPlaying;
						--to;
						myPlaylist->Items->at(to+1).SetPosition(to);
						myPlaylist->Items->at(to).SetPosition(to+1);
						myPlaylist->Items->Swap(to, to+1);
						myPlaylist->Items->Scroll(wUp);
						myPlaylist->Items->Refresh();
						wFooter->ReadKey(input);
					}
					Mpd.Move(from, to);
					Playlist::BlockNowPlayingUpdate = 0;
					UpdateStatusImmediately = 1;
				}
				wFooter->SetTimeout(ncmpcpp_window_timeout);
			}
			else if (myScreen->ActiveWindow() == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
			{
				// remove search results as we may move them to different positions, but
				// search rememebers positions and may point to wrong ones after that.
				myPlaylistEditor->Content->Search("");
				wFooter->SetTimeout(50);
				if (myPlaylistEditor->Content->hasSelected())
				{
					std::vector<size_t> list;
					myPlaylistEditor->Content->GetSelected(list);
					
					std::vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongUp) && list.front() > 0)
					{
						TraceMpdStatus();
						myPlaylist->UpdateTimer();
						for (std::vector<size_t>::iterator it = list.begin(); it != list.end(); ++it)
						{
							--*it;
							myPlaylistEditor->Content->Swap(*it, (*it)+1);
						}
						myPlaylistEditor->Content->Highlight(list[(list.size()-1)/2]);
						myPlaylistEditor->Content->Refresh();
						wFooter->ReadKey(input);
					}
					Mpd.StartCommandsList();
					for (size_t i = 0; i < list.size(); ++i)
						if (origs[i] != list[i])
							Mpd.Move(myPlaylistEditor->Playlists->Current(), origs[i], list[i]);
					Mpd.CommitCommandsList();
				}
				else
				{
					size_t from, to;
					from = to = myPlaylistEditor->Content->Choice();
					while (Keypressed(input, Key.MvSongUp) && to > 0)
					{
						TraceMpdStatus();
						myPlaylist->UpdateTimer();
						--to;
						myPlaylistEditor->Content->Swap(to, to+1);
						myPlaylistEditor->Content->Scroll(wUp);
						myPlaylistEditor->Content->Refresh();
						wFooter->ReadKey(input);
					}
					if (from != to)
						Mpd.Move(myPlaylistEditor->Playlists->Current(), from, to);
				}
				wFooter->SetTimeout(ncmpcpp_window_timeout);
			}
		}
		else if (Keypressed(input, Key.MvSongDown))
		{
			if (myScreen == myPlaylist && myPlaylist->SortingInProgress())
				myPlaylist->AdjustSortOrder(input);
			else if (myScreen == myPlaylist && !myPlaylist->Items->Empty())
			{
				CHECK_PLAYLIST_FOR_FILTERING;
				// remove search results as we may move them to different positions, but
				// search rememebers positions and may point to wrong ones after that.
				myPlaylist->Items->Search("");
				wFooter->SetTimeout(50);
				if (myPlaylist->Items->hasSelected())
				{
					std::vector<size_t> list;
					myPlaylist->Items->GetSelected(list);
					std::vector<size_t> origs(list);
					
					bool modify_now_playing = 0;
					for (std::vector<size_t>::iterator it = list.begin(); it != list.end(); ++it)
					{
						if (*it == size_t(myPlaylist->NowPlaying) && list.back() < myPlaylist->Items->Size()-1)
						{
							modify_now_playing = 1;
							Playlist::BlockNowPlayingUpdate = 1;
							break;
						}
					}
					
					while (Keypressed(input, Key.MvSongDown) && list.back() < myPlaylist->Items->Size()-1)
					{
						TraceMpdStatus();
						Playlist::BlockUpdate = 1;
						myPlaylist->UpdateTimer();
						if (modify_now_playing)
							++myPlaylist->NowPlaying;
						for (std::vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); ++it)
						{
							++*it;
							myPlaylist->Items->at((*it)-1).SetPosition(*it);
							myPlaylist->Items->at(*it).SetPosition((*it)-1);
							myPlaylist->Items->Swap(*it, (*it)-1);
						}
						myPlaylist->Items->Highlight(list[(list.size()-1)/2]);
						myPlaylist->Items->Refresh();
						wFooter->ReadKey(input);
					}
					Playlist::BlockNowPlayingUpdate = 0;
					Mpd.StartCommandsList();
					for (int i = list.size()-1; i >= 0; --i)
						Mpd.Move(origs[i], list[i]);
					Mpd.CommitCommandsList();
				}
				else
				{
					size_t from, to;
					from = to = myPlaylist->Items->Choice();
					bool modify_now_playing = from == size_t(myPlaylist->NowPlaying);
					if (modify_now_playing)
						Playlist::BlockNowPlayingUpdate = 1;
					while (Keypressed(input, Key.MvSongDown) && to < myPlaylist->Items->Size()-1)
					{
						TraceMpdStatus();
						Playlist::BlockUpdate = 1;
						myPlaylist->UpdateTimer();
						if (modify_now_playing)
							++myPlaylist->NowPlaying;
						++to;
						myPlaylist->Items->at(to-1).SetPosition(to);
						myPlaylist->Items->at(to).SetPosition(to-1);
						myPlaylist->Items->Swap(to, to-1);
						myPlaylist->Items->Scroll(wDown);
						myPlaylist->Items->Refresh();
						wFooter->ReadKey(input);
					}
					Mpd.Move(from, to);
					Playlist::BlockNowPlayingUpdate = 0;
					UpdateStatusImmediately = 1;
				}
				wFooter->SetTimeout(ncmpcpp_window_timeout);
				
			}
			else if (myScreen->ActiveWindow() == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
			{
				// remove search results as we may move them to different positions, but
				// search rememebers positions and may point to wrong ones after that.
				myPlaylistEditor->Content->Search("");
				wFooter->SetTimeout(50);
				if (myPlaylistEditor->Content->hasSelected())
				{
					std::vector<size_t> list;
					myPlaylistEditor->Content->GetSelected(list);
					
					std::vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongDown) && list.back() < myPlaylistEditor->Content->Size()-1)
					{
						TraceMpdStatus();
						myPlaylist->UpdateTimer();
						for (std::vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); ++it)
						{
							++*it;
							myPlaylistEditor->Content->Swap(*it, (*it)-1);
						}
						myPlaylistEditor->Content->Highlight(list[(list.size()-1)/2]);
						myPlaylistEditor->Content->Refresh();
						wFooter->ReadKey(input);
					}
					Mpd.StartCommandsList();
					for (int i = list.size()-1; i >= 0; --i)
						if (origs[i] != list[i])
							Mpd.Move(myPlaylistEditor->Playlists->Current(), origs[i], list[i]);
					Mpd.CommitCommandsList();
				}
				else
				{
					size_t from, to;
					from = to = myPlaylistEditor->Content->Choice();
					while (Keypressed(input, Key.MvSongDown) && to < myPlaylistEditor->Content->Size()-1)
					{
						TraceMpdStatus();
						myPlaylist->UpdateTimer();
						++to;
						myPlaylistEditor->Content->Swap(to, to-1);
						myPlaylistEditor->Content->Scroll(wDown);
						myPlaylistEditor->Content->Refresh();
						wFooter->ReadKey(input);
					}
					if (from != to)
						Mpd.Move(myPlaylistEditor->Playlists->Current(), from, to);
				}
				wFooter->SetTimeout(ncmpcpp_window_timeout);
			}
		}
		else if ((Keypressed(input, Key.MoveTo) || Keypressed(input, Key.MoveBefore)
					|| Keypressed(input, Key.MoveAfter)) && myScreen == myPlaylist)
		{
			CHECK_PLAYLIST_FOR_FILTERING;
			if (!myPlaylist->Items->hasSelected())
			{
				ShowMessage("No selected items to move!");
				continue;
			}
			// remove search results as we may move them to different positions, but
			// search rememebers positions and may point to wrong ones after that.
			myPlaylist->Items->Search("");
			Playlist::BlockUpdate = 1;
			size_t pos = myPlaylist->Items->Choice();
			std::vector<size_t> list;
			myPlaylist->Items->GetSelected(list);
			if (pos >= list.front() && pos <= list.back())
				continue;
			if (Keypressed(input, Key.MoveTo))
			{
				 // if cursor is at the last item, break convention and move at the end of playlist
				if (pos == myPlaylist->Items->Size()-1)
					pos++;
			}
			else if (Keypressed(input, Key.MoveAfter))
				pos++;
			int diff = pos-list.front();
			Mpd.StartCommandsList();
			if (diff > 0)
			{
				pos -= list.size();
				size_t i = list.size()-1;
				for (std::vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); ++it, --i)
				{
					Mpd.Move(*it, pos+i);
					myPlaylist->Items->Move(*it, pos+i);
				}
			}
			else if (diff < 0)
			{
				size_t i = 0;
				for (std::vector<size_t>::const_iterator it = list.begin(); it != list.end(); ++it, ++i)
				{
					Mpd.Move(*it, pos+i);
					myPlaylist->Items->Move(*it, pos+i);
				}
			}
			Mpd.CommitCommandsList();
			myPlaylist->Items->Highlight(pos);
			myPlaylist->FixPositions();
		}
		else if (Keypressed(input, Key.Add))
		{
			if (myScreen == myPlaylistEditor && myPlaylistEditor->Playlists->Empty())
				continue;
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
					static const char lastfm_url[] = "lastfm://";
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
				UpdateStatusImmediately = 1;
			}
		}
		else if (Keypressed(input, Key.SeekForward) || Keypressed(input, Key.SeekBackward))
		{
			if (!Mpd.GetTotalTime())
			{
				ShowMessage("Unknown item length!");
				continue;
			}
			
			const Song *s = myPlaylist->NowPlayingSong();
			if (!s)
				continue;
			
			LockProgressbar();
			LockStatusbar();
			
			int songpos;
			time_t t = time(0);
			
			songpos = Mpd.GetElapsedTime();
			
			int old_timeout = wFooter->GetTimeout();
			wFooter->SetTimeout(ncmpcpp_window_timeout);
			
			SeekingInProgress = 1;
			*wFooter << fmtBold;
			while (Keypressed(input, Key.SeekForward) || Keypressed(input, Key.SeekBackward))
			{
				TraceMpdStatus();
				myPlaylist->UpdateTimer();
				wFooter->ReadKey(input);
				
				int howmuch = Config.incremental_seeking ? (myPlaylist->Timer()-t)/2+Config.seek_time : Config.seek_time;
				
				if (Keypressed(input, Key.SeekForward) && songpos < Mpd.GetTotalTime())
				{
					songpos += howmuch;
					if (songpos > Mpd.GetTotalTime())
						songpos = Mpd.GetTotalTime();
				}
				else if (Keypressed(input, Key.SeekBackward) && songpos > 0)
				{
					songpos -= howmuch;
					if (songpos < 0)
						songpos = 0;
				}
				
				std::string tracklength;
				if (Config.new_design)
				{
					if (Config.display_remaining_time)
					{
						tracklength = "-";
						tracklength += Song::ShowTime(Mpd.GetTotalTime()-songpos);
					}
					else
						tracklength = Song::ShowTime(songpos);
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
						tracklength += Song::ShowTime(Mpd.GetTotalTime()-songpos);
					}
					else
						tracklength += Song::ShowTime(songpos);
					tracklength += "/";
					tracklength += MPD::Song::ShowTime(Mpd.GetTotalTime());
					tracklength += "]";
					*wFooter << XY(wFooter->GetWidth()-tracklength.length(), 1) << tracklength;
				}
				DrawProgressbar(songpos, Mpd.GetTotalTime());
				wFooter->Refresh();
			}
			*wFooter << fmtBoldEnd;
			SeekingInProgress = 0;
			Mpd.Seek(songpos);
			UpdateStatusImmediately = 1;
			
			wFooter->SetTimeout(old_timeout);
			
			UnlockProgressbar();
			UnlockStatusbar();
		}
		else if (Keypressed(input, Key.ToggleDisplayMode))
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
		else if (Keypressed(input, Key.ToggleSeparatorsInPlaylist))
		{
			Config.playlist_separate_albums = !Config.playlist_separate_albums;
			ShowMessage("Separators between albums in playlist: %s", Config.playlist_separate_albums ? "On" : "Off");
		}
#		ifdef HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ToggleLyricsDB))
		{
			myLyrics->ToggleFetcher();
		}
		else if (Keypressed(input, Key.ToggleFetchingLyricsInBackground))
		{
			Config.fetch_lyrics_in_background = !Config.fetch_lyrics_in_background;
			ShowMessage("Fetching lyrics for currently playing song in background: %s", Config.fetch_lyrics_in_background ? "On" : "Off");
		}
#		endif // HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ToggleAutoCenter))
		{
			Config.autocenter_mode = !Config.autocenter_mode;
			ShowMessage("Auto center mode: %s", Config.autocenter_mode ? "On" : "Off");
			if (Config.autocenter_mode && myPlaylist->isPlaying() && !myPlaylist->Items->isFiltered())
				myPlaylist->Items->Highlight(myPlaylist->NowPlaying);
		}
		else if (Keypressed(input, Key.UpdateDB))
		{
			if (myScreen == myBrowser)
				Mpd.UpdateDirectory(locale_to_utf_cpy(myBrowser->CurrentDir()));
#			ifdef HAVE_TAGLIB_H
			else if (myScreen == myTagEditor && !Config.albums_in_tag_editor)
				Mpd.UpdateDirectory(myTagEditor->CurrentDir());
#			endif // HAVE_TAGLIB_H
			else
				Mpd.UpdateDirectory("/");
		}
		else if (Keypressed(input, Key.GoToNowPlaying))
		{
			if (myScreen == myPlaylist && myPlaylist->isPlaying())
			{
				CHECK_PLAYLIST_FOR_FILTERING;
				myPlaylist->Items->Highlight(myPlaylist->NowPlaying);
			}
			else if (myScreen == myBrowser)
			{
				if (const Song *s = myPlaylist->NowPlayingSong())
				{
					myBrowser->LocateSong(*s);
					RedrawHeader = 1;
				}
			}
		}
		else if (Keypressed(input, Key.ToggleRepeat))
		{
			Mpd.SetRepeat(!Mpd.GetRepeat());
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.Shuffle))
		{
			Mpd.Shuffle();
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.ToggleRandom))
		{
			Mpd.SetRandom(!Mpd.GetRandom());
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.ToggleSingle))
		{
			if (myScreen == mySearcher && !mySearcher->Main()->isStatic(0))
			{
				mySearcher->Main()->Highlight(SearchEngine::SearchButton);
				mySearcher->Main()->Highlighting(0);
				mySearcher->Main()->Refresh();
				mySearcher->Main()->Highlighting(1);
				mySearcher->EnterPressed();
			}
#			ifdef HAVE_TAGLIB_H
			else if (myScreen == myTinyTagEditor)
			{
				myTinyTagEditor->Main()->Highlight(myTinyTagEditor->Main()->Size()-2); // Save
				myTinyTagEditor->EnterPressed();
			}
#			endif // HAVE_TAGLIB_H
			else
			{
				Mpd.SetSingle(!Mpd.GetSingle());
				UpdateStatusImmediately = 1;
			}
		}
		else if (Keypressed(input, Key.ToggleConsume))
		{
			Mpd.SetConsume(!Mpd.GetConsume());
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.ToggleCrossfade))
		{
			Mpd.SetCrossfade(Mpd.GetCrossfade() ? 0 : Config.crossfade_time);
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.SetCrossfade))
		{
			LockStatusbar();
			Statusbar() << "Set crossfade to: ";
			std::string crossfade = wFooter->GetString(3);
			UnlockStatusbar();
			int cf = StrToInt(crossfade);
			if (cf > 0)
			{
				Config.crossfade_time = cf;
				Mpd.SetCrossfade(cf);
				UpdateStatusImmediately = 1;
			}
		}
		else if (Keypressed(input, Key.EditTags))
		{
			if ((myScreen != myBrowser || !myBrowser->isLocal())
			&&  myScreen != myLyrics)
				CHECK_MPD_MUSIC_DIR;
#			ifdef HAVE_TAGLIB_H
			if (myTinyTagEditor->SetEdited(myScreen->CurrentSong()))
			{
				myTinyTagEditor->SwitchTo();
			}
			else if (myScreen->ActiveWindow() == myLibrary->Artists && !myLibrary->Artists->Empty())
			{
				LockStatusbar();
				Statusbar() << fmtBold << IntoStr(Config.media_lib_primary_tag) << fmtBoldEnd << ": ";
				std::string new_tag = wFooter->GetString(myLibrary->Artists->Current());
				UnlockStatusbar();
				if (!new_tag.empty() && new_tag != myLibrary->Artists->Current())
				{
					bool success = 1;
					SongList list;
					ShowMessage("Updating tags...");
					Mpd.StartSearch(1);
					Mpd.AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(myLibrary->Artists->Current()));
					Mpd.CommitSearch(list);
					Song::SetFunction set = IntoSetFunction(Config.media_lib_primary_tag);
					if (!set)
						continue;
					for (SongList::iterator it = list.begin(); it != list.end(); ++it)
					{
						(*it)->Localize();
						(*it)->SetTags(set, new_tag);
						ShowMessage("Updating tags in \"%s\"...", (*it)->GetName().c_str());
						std::string path = Config.mpd_music_dir + (*it)->GetFile();
						if (!TagEditor::WriteTags(**it))
						{
							static const char msg[] = "Error while updating tags in \"%s\"!";
							ShowMessage(msg, Shorten(TO_WSTRING((*it)->GetFile()), COLS-static_strlen(msg)).c_str());
							success = 0;
							break;
						}
					}
					if (success)
					{
						Mpd.UpdateDirectory(locale_to_utf_cpy(FindSharedDir(list)));
						ShowMessage("Tags updated successfully!");
					}
					FreeSongList(list);
				}
			}
			else if (myScreen->ActiveWindow() == myLibrary->Albums && !myLibrary->Albums->Empty())
			{
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
							static const char msg[] = "Error while opening file \"%s\"!";
							ShowMessage(msg, Shorten(TO_WSTRING((*myLibrary->Songs)[i].GetFile()), COLS-static_strlen(msg)).c_str());
							success = 0;
							break;
						}
						f.tag()->setAlbum(ToWString(new_album));
						if (!f.save())
						{
							static const char msg[] = "Error while writing tags in \"%s\"!";
							ShowMessage(msg, Shorten(TO_WSTRING((*myLibrary->Songs)[i].GetFile()), COLS-static_strlen(msg)).c_str());
							success = 0;
							break;
						}
					}
					if (success)
					{
						Mpd.UpdateDirectory(locale_to_utf_cpy(FindSharedDir(myLibrary->Songs)));
						ShowMessage("Tags updated successfully!");
					}
				}
			}
			else if (myScreen->ActiveWindow() == myTagEditor->Dirs
			     &&  !myTagEditor->Dirs->Empty()
			     &&  myTagEditor->Dirs->Choice() > 0)
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
						static const char msg[] = "Directory renamed to \"%s\"";
						ShowMessage(msg, Shorten(TO_WSTRING(new_dir), COLS-static_strlen(msg)).c_str());
						Mpd.UpdateDirectory(myTagEditor->CurrentDir());
					}
					else
					{
						static const char msg[] = "Couldn't rename \"%s\": %s";
						ShowMessage(msg, Shorten(TO_WSTRING(old_dir), COLS-static_strlen(msg)-25).c_str(), strerror(errno));
					}
				}
			}
			else
#			endif // HAVE_TAGLIB_H
			if (myScreen == myLyrics)
			{
				myLyrics->Edit();
			}
			if (myScreen == myBrowser && !myBrowser->Main()->Empty() && myBrowser->Main()->Current().type == itDirectory)
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
						static const char msg[] = "Directory renamed to \"%s\"";
						ShowMessage(msg, Shorten(TO_WSTRING(new_dir), COLS-static_strlen(msg)).c_str());
						if (!myBrowser->isLocal())
							Mpd.UpdateDirectory(locale_to_utf_cpy(FindSharedDir(old_dir, new_dir)));
						myBrowser->GetDirectory(myBrowser->CurrentDir());
					}
					else
					{
						static const char msg[] = "Couldn't rename \"%s\": %s";
						ShowMessage(msg, Shorten(TO_WSTRING(old_dir), COLS-static_strlen(msg)-25).c_str(), strerror(errno));
					}
				}
			}
			else if (
				 (myScreen->ActiveWindow() == myPlaylistEditor->Playlists
			       && !myPlaylistEditor->Playlists->Empty()
				 )
			     ||  (myScreen == myBrowser
			       && !myBrowser->Main()->Empty()
			       && myBrowser->Main()->Current().type == itPlaylist
				 )
				)
			{
				std::string old_name = myScreen->ActiveWindow() == myPlaylistEditor->Playlists ? myPlaylistEditor->Playlists->Current() : myBrowser->Main()->Current().name;
				LockStatusbar();
				Statusbar() << fmtBold << "Playlist: " << fmtBoldEnd;
				std::string new_name = wFooter->GetString(old_name);
				UnlockStatusbar();
				if (!new_name.empty() && new_name != old_name)
				{
					if (Mpd.Rename(locale_to_utf_cpy(old_name), locale_to_utf_cpy(new_name)))
					{
						static const char msg[] = "Playlist renamed to \"%s\"";
						ShowMessage(msg, Shorten(TO_WSTRING(new_name), COLS-static_strlen(msg)).c_str());
						if (myBrowser->Main() && !myBrowser->isLocal())
							myBrowser->GetDirectory("/");
						if (myPlaylistEditor->Main())
							myPlaylistEditor->Playlists->Clear();
					}
				}
			}
		}
		else if (Keypressed(input, Key.GoToContainingDir))
		{
			if (myScreen == myBrowser && myBrowser->Main()->Current().type == itPlaylist)
			{
				const std::string pl_name = myBrowser->Main()->Current().name;
				myPlaylistEditor->JumpTo(pl_name);
			}
			else
			{
				Song *s = myScreen->CurrentSong();
				if (s)
					myBrowser->LocateSong(*s);
			}
		}
		else if (Keypressed(input, Key.GoToMediaLibrary))
		{
			Song *s = myScreen->CurrentSong();
			if (s)
				myLibrary->LocateSong(*s);
		}
		else if (Keypressed(input, Key.ToggleScreenLock))
		{
			if (myLockedScreen != 0)
			{
				BasicScreen::Unlock();
				set_resize_flags();
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
						continue;
					part = StrToInt(str_part);
				}
				if (part < 20 || part > 80)
				{
					ShowMessage("Invalid number (%d)!", part);
					continue;
				}
				Config.locked_screen_width_part = part/100.0;
				if (myScreen->Lock())
					ShowMessage("Screen locked (with %d%% width)", part);
				else
					ShowMessage("Screen cannot be locked");
			}
		}
#		ifdef HAVE_TAGLIB_H
		else if (Keypressed(input, Key.GoToTagEditor))
		{
			CHECK_MPD_MUSIC_DIR;
			if (Song *s = myScreen->CurrentSong())
				myTagEditor->LocateSong(*s);
		}
#		endif // HAVE_TAGLIB_H
		else if (Keypressed(input, Key.GoToPosition))
		{
			if (!Mpd.GetTotalTime())
			{
				ShowMessage("Unknown item length!");
				continue;
			}
			
			const Song *s = myPlaylist->NowPlayingSong();
			if (!s)
				continue;
			
			LockStatusbar();
			Statusbar() << "Position to go (in %/mm:ss/seconds(s)): ";
			std::string position = wFooter->GetString();
			UnlockStatusbar();
			
			if (position.empty())
				continue;
			
			int newpos = 0;
			if (position.find(':') != std::string::npos) // probably time in mm:ss
			{
				newpos = StrToInt(position)*60 + StrToInt(position.substr(position.find(':')+1));
				if (newpos >= 0 && newpos <= Mpd.GetTotalTime())
					Mpd.Seek(newpos);
				else
					ShowMessage("Out of bounds, 0:00-%s possible for mm:ss, %s given.", s->GetLength().c_str(), MPD::Song::ShowTime(newpos).c_str());
			}
			else if (position.find('s') != std::string::npos) // probably position in seconds
			{
				newpos = StrToInt(position);
				if (newpos >= 0 && newpos <= Mpd.GetTotalTime())
					Mpd.Seek(newpos);
				else
					ShowMessage("Out of bounds, 0-%d possible for seconds, %d given.", s->GetTotalLength(), newpos);
			}
			else
			{
				newpos = StrToInt(position);
				if (newpos >= 0 && newpos <= 100)
					Mpd.Seek(Mpd.GetTotalTime()*newpos/100.0);
				else
					ShowMessage("Out of bounds, 0-100 possible for %%, %d given.", newpos);
			}
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.ReverseSelection))
		{
			if (myScreen->allowsSelection())
			{
				myScreen->ReverseSelection();
				ShowMessage("Selection reversed!");
			}
		}
		else if (Keypressed(input, Key.DeselectAll))
		{
			if (myScreen->allowsSelection())
			{
				List *mList = myScreen->GetList();
				if (!mList->hasSelected())
					continue;
				for (size_t i = 0; i < mList->Size(); ++i)
					mList->Select(i, 0);
				ShowMessage("Items deselected!");
			}
		}
		else if (Keypressed(input, Key.SelectAlbum))
		{
			if (myScreen->allowsSelection())
			{
				if (List *mList = myScreen->GetList())
				{
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
						ShowMessage("Album around cursor position selected.");
					}
				}
			}
		}
		else if (Keypressed(input, Key.AddSelected))
		{
			mySelectedItemsAdder->SwitchTo();
		}
		else if (Keypressed(input, Key.Crop))
		{
			CHECK_PLAYLIST_FOR_FILTERING;
			if (Config.ask_before_clearing_main_playlist)
			{
				LockStatusbar();
				Statusbar() << "Do you really want to crop playlist";
				if (myScreen->ActiveWindow() == myPlaylistEditor->Content)
					*wFooter << " \"" << myPlaylistEditor->Playlists->Current() << "\"";
				*wFooter << " ? [" << fmtBold << 'y' << fmtBoldEnd << '/' << fmtBold << 'n' << fmtBoldEnd << "] ";
				wFooter->Refresh();
				int answer = 0;
				do
				{
					TraceMpdStatus();
					wFooter->ReadKey(answer);
				}
				while (answer != 'y' && answer != 'n');
				UnlockStatusbar();
				if (answer != 'y')
				{
					ShowMessage("Aborted!");
					continue;
				}
			}
			if (myPlaylist->Items->hasSelected())
			{
				Mpd.StartCommandsList();
				for (int i = myPlaylist->Items->Size()-1; i >= 0; --i)
				{
					if (!myPlaylist->Items->isSelected(i) && i != myPlaylist->NowPlaying)
						Mpd.Delete(i);
				}
				// if mpd deletes now playing song deletion will be sluggishly slow
				// then so we have to assure it will be deleted at the very end.
				if (myPlaylist->isPlaying() && !myPlaylist->Items->isSelected(myPlaylist->NowPlaying))
					Mpd.DeleteID(myPlaylist->NowPlayingSong()->GetID());
				
				ShowMessage("Deleting all items but selected...");
				Mpd.CommitCommandsList();
				ShowMessage("Items deleted!");
			}
			else
			{
				if (!myPlaylist->isPlaying())
				{
					ShowMessage("Nothing is playing now!");
					continue;
				}
				Mpd.StartCommandsList();
				for (int i = myPlaylist->Items->Size()-1; i >= 0; --i)
					if (i != myPlaylist->NowPlaying)
						Mpd.Delete(i);
				ShowMessage("Deleting all items except now playing one...");
				Mpd.CommitCommandsList();
				ShowMessage("Items deleted!");
			}
		}
		else if (Keypressed(input, Key.Clear))
		{
			if (myScreen == myPlaylistEditor && myPlaylistEditor->Playlists->Empty())
				continue;
			
			if (myScreen->ActiveWindow() == myPlaylistEditor->Content
			||  Config.ask_before_clearing_main_playlist)
			{
				LockStatusbar();
				Statusbar() << "Do you really want to clear playlist";
				if (myScreen->ActiveWindow() == myPlaylistEditor->Content)
					*wFooter << " \"" << myPlaylistEditor->Playlists->Current() << "\"";
				*wFooter << " ? [" << fmtBold << 'y' << fmtBoldEnd << '/' << fmtBold << 'n' << fmtBoldEnd << "] ";
				wFooter->Refresh();
				int answer = 0;
				do
				{
					TraceMpdStatus();
					wFooter->ReadKey(answer);
				}
				while (answer != 'y' && answer != 'n');
				UnlockStatusbar();
				if (answer != 'y')
				{
					ShowMessage("Aborted!");
					continue;
				}
			}
			
			if (myPlaylist->Items->isFiltered())
			{
				ShowMessage("Deleting filtered items...");
				Mpd.StartCommandsList();
				for (int i = myPlaylist->Items->Size()-1; i >= 0; --i)
					Mpd.Delete((*myPlaylist->Items)[i].GetPosition());
				Mpd.CommitCommandsList();
				ShowMessage("Filtered items deleted!");
			}
			else
			{
				if (myScreen->ActiveWindow() == myPlaylistEditor->Content)
				{
					Mpd.ClearPlaylist(locale_to_utf_cpy(myPlaylistEditor->Playlists->Current()));
					myPlaylistEditor->Content->Clear();
				}
				else
				{
					ShowMessage("Clearing playlist...");
					Mpd.ClearPlaylist();
				}
			}
			// if playlist is cleared, items list have to be updated, but this
			// can be blocked if new song was added to playlist less than one
			// second ago, so we need to assume it's unlocked.
			BlockItemListUpdate = 0;
			UpdateStatusImmediately = 1;
		}
		else if (Keypressed(input, Key.SortPlaylist) && myScreen == myPlaylist)
		{
			CHECK_PLAYLIST_FOR_FILTERING;
			myPlaylist->Sort();
		}
		else if (Keypressed(input, Key.ApplyFilter))
		{
			List *mList = myScreen->GetList();
			
			if (!mList)
				continue;
			
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
				Playlist::ReloadTotalLength = 1;
				RedrawHeader = 1;
			}
		}
		else if (Keypressed(input, Key.FindForward) || Keypressed(input, Key.FindBackward))
		{
			List *mList = myScreen->GetList();
			
			if (mList)
			{
				LockStatusbar();
				Statusbar() << "Find " << (Keypressed(input, Key.FindForward) ? "forward" : "backward") << ": ";
				std::string findme = wFooter->GetString();
				UnlockStatusbar();
				myPlaylist->UpdateTimer();
				
				if (!findme.empty())
					ShowMessage("Searching...");
				
				bool success = mList->Search(findme, myScreen == mySearcher ? SearchEngine::StaticOptions : 0, REG_ICASE | Config.regex_type);
				
				if (findme.empty())
					continue;
				
				success ? ShowMessage("Searching finished!") : ShowMessage("Unable to find \"%s\"", findme.c_str());
				
				if (Keypressed(input, Key.FindForward))
					mList->NextFound(Config.wrapped_search);
				else
					mList->PrevFound(Config.wrapped_search);
				
				if (myScreen == myPlaylist)
					myPlaylist->EnableHighlighting();
			}
			else if (myScreen == myHelp || myScreen == myLyrics
#			ifdef HAVE_CURL_CURL_H
			     || myScreen == myLastfm
#			endif // HAVE_CURL_CURL_H
				)
			{
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
		}
		else if (Keypressed(input, Key.NextFoundPosition) || Keypressed(input, Key.PrevFoundPosition))
		{
			List *mList = myScreen->GetList();
			
			if (!mList)
				continue;
			
			if (Keypressed(input, Key.NextFoundPosition))
				mList->NextFound(Config.wrapped_search);
			else
				mList->PrevFound(Config.wrapped_search);
		}
		else if (Keypressed(input, Key.ToggleFindMode))
		{
			Config.wrapped_search = !Config.wrapped_search;
			ShowMessage("Search mode: %s", Config.wrapped_search ? "Wrapped" : "Normal");
		}
		else if (Keypressed(input, Key.ToggleReplayGainMode) && Mpd.Version() >= 16)
		{
			LockStatusbar();
			Statusbar() << "Replay gain mode ? [" << fmtBold << 'o' << fmtBoldEnd << "ff/" << fmtBold << 't' << fmtBoldEnd << "rack/" << fmtBold << 'a' << fmtBoldEnd << "lbum]";
			wFooter->Refresh();
			int answer = 0;
			do
			{
				TraceMpdStatus();
				wFooter->ReadKey(answer);
			}
			while (answer != 'o' && answer != 't' && answer != 'a');
			UnlockStatusbar();
			Mpd.SetReplayGainMode(answer == 't' ? rgmTrack : (answer == 'a' ? rgmAlbum : rgmOff));
			ShowMessage("Replay gain mode: %s", Mpd.GetReplayGainMode().c_str());
		}
		else if (Keypressed(input, Key.ToggleSpaceMode))
		{
			Config.space_selects = !Config.space_selects;
			ShowMessage("Space mode: %s item", Config.space_selects ? "Select/deselect" : "Add");
		}
		else if (Keypressed(input, Key.ToggleAddMode))
		{
			Config.ncmpc_like_songs_adding = !Config.ncmpc_like_songs_adding;
			ShowMessage("Add mode: %s", Config.ncmpc_like_songs_adding ? "Add item to playlist, remove if already added" : "Always add item to playlist");
		}
		else if (Keypressed(input, Key.ToggleMouse))
		{
			Config.mouse_support = !Config.mouse_support;
			mousemask(Config.mouse_support ? ALL_MOUSE_EVENTS : 0, 0);
			ShowMessage("Mouse support %s", Config.mouse_support ? "enabled" : "disabled");
		}
		else if (Keypressed(input, Key.ToggleBitrateVisibility))
		{
			Config.display_bitrate = !Config.display_bitrate;
			ShowMessage("Bitrate visibility %s", Config.display_bitrate ? "enabled" : "disabled");
		}
		else if (Keypressed(input, Key.SwitchTagTypeList))
		{
			if (myScreen == myPlaylist)
			{
				LockStatusbar();
				Statusbar() << "Add random ? [" << fmtBold << 's' << fmtBoldEnd << "ongs/" << fmtBold << 'a' << fmtBoldEnd << "rtists/al" << fmtBold << 'b' << fmtBoldEnd << "ums] ";
				wFooter->Refresh();
				int answer = 0;
				do
				{
					TraceMpdStatus();
					wFooter->ReadKey(answer);
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
					ShowMessage("%zu random %s%s added to playlist!", number, tag_type_str.c_str(), number == 1 ? "" : "s");
			}
			else if (myScreen == myBrowser && !myBrowser->isLocal())
			{
				Config.browser_sort_by_mtime = !Config.browser_sort_by_mtime;
				myBrowser->Main()->Sort<CaseInsensitiveSorting>(myBrowser->CurrentDir() != "/");
				ShowMessage("Sort songs by: %s", Config.browser_sort_by_mtime ? "Modification time" : "Name");
			}
			else if (myScreen->ActiveWindow() == myLibrary->Artists
			||	 (myLibrary->Columns() == 2 && myScreen->ActiveWindow() == myLibrary->Albums))
			{
				LockStatusbar();
				Statusbar() << "Tag type ? [" << fmtBold << 'a' << fmtBoldEnd << "rtist/album" << fmtBold << 'A' << fmtBoldEnd << "rtist/" << fmtBold << 'y' << fmtBoldEnd << "ear/" << fmtBold << 'g' << fmtBoldEnd << "enre/" << fmtBold << 'c' << fmtBoldEnd << "omposer/" << fmtBold << 'p' << fmtBoldEnd << "erformer] ";
				wFooter->Refresh();
				int answer = 0;
				do
				{
					TraceMpdStatus();
					wFooter->ReadKey(answer);
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
			else if (myScreen == myLyrics)
			{
				myLyrics->Refetch();
			}
#			ifdef HAVE_CURL_CURL_H
			else if (myScreen == myLastfm)
			{
				myLastfm->Refetch();
			}
#			endif // HAVE_CURL_CURL_H
		}
		else if (Keypressed(input, Key.SongInfo))
		{
			mySongInfo->SwitchTo();
		}
#		ifdef HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ArtistInfo))
		{
			if (myScreen == myLastfm || myLastfm->isDownloading())
			{
				myLastfm->SwitchTo();
				continue;
			}
			
			std::string artist;
			MPD::Song *s = myScreen->CurrentSong();
			
			if (s)
				artist = s->GetArtist();
			else if (myScreen == myLibrary && myLibrary->Main() == myLibrary->Artists && !myLibrary->Artists->Empty())
				artist = myLibrary->Artists->Current();
			
			if (!artist.empty() && myLastfm->SetArtistInfoArgs(artist, Config.lastfm_preferred_language))
				myLastfm->SwitchTo();
		}
#		endif // HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.Lyrics))
		{
			myLyrics->SwitchTo();
		}
		else if (Keypressed(input, Key.Quit))
		{
			main_exit = 1;
		}
#		ifdef HAVE_TAGLIB_H
		else if (myScreen == myTinyTagEditor)
		{
			continue;
		}
#		endif // HAVE_TAGLIB_H
		else if (Keypressed(input, Key.Help))
		{
			myHelp->SwitchTo();
		}
		else if (Keypressed(input, Key.ScreenSwitcher))
		{
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
					(*Config.screens_seq.begin())->SwitchTo();
				else
					(*screen)->SwitchTo();
			}
		}
		else if (!Config.screen_switcher_previous && Keypressed(input, Key.BackwardScreenSwitcher))
		{
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
					(*Config.screens_seq.rbegin())->SwitchTo();
				else
					(*--screen)->SwitchTo();
			}
		}
		else if (Keypressed(input, Key.Playlist))
		{
			myPlaylist->SwitchTo();
		}
		else if (Keypressed(input, Key.Browser))
		{
			myBrowser->SwitchTo();
		}
		else if (Keypressed(input, Key.SearchEngine))
		{
			mySearcher->SwitchTo();
		}
		else if (Keypressed(input, Key.MediaLibrary))
		{
			myLibrary->SwitchTo();
		}
		else if (Keypressed(input, Key.PlaylistEditor))
		{
			myPlaylistEditor->SwitchTo();
		}
#		ifdef HAVE_TAGLIB_H
		else if (Keypressed(input, Key.TagEditor))
		{
			CHECK_MPD_MUSIC_DIR;
			myTagEditor->SwitchTo();
		}
#		endif // HAVE_TAGLIB_H
#		ifdef ENABLE_OUTPUTS
		else if (Keypressed(input, Key.Outputs))
		{
			myOutputs->SwitchTo();
		}
#		endif // ENABLE_OUTPUTS
#		ifdef ENABLE_VISUALIZER
		else if (Keypressed(input, Key.Visualizer))
		{
			myVisualizer->SwitchTo();
		}
#		endif // ENABLE_VISUALIZER
#		ifdef ENABLE_CLOCK
		else if (Keypressed(input, Key.Clock))
		{
			myClock->SwitchTo();
		}
#		endif // ENABLE_CLOCK
		else if (Keypressed(input, Key.ServerInfo))
		{
			myServerInfo->SwitchTo();
		}
		// key mapping end
		
		if (myScreen == myLibrary
		||  myScreen == myPlaylistEditor
#		ifdef HAVE_TAGLIB_H
		||  myScreen == myTagEditor
#		endif // HAVE_TAGLIB_H
		   )
		{
			if (Keypressed(input, Key.Up)
			||  Keypressed(input, Key.Down)
			||  Keypressed(input, Key.PageUp)
			||  Keypressed(input, Key.PageDown)
			||  Keypressed(input, Key.Home)
			||  Keypressed(input, Key.End)
			||  Keypressed(input, Key.ApplyFilter)
			||  Keypressed(input, Key.FindForward)
			||  Keypressed(input, Key.FindBackward)
			||  Keypressed(input, Key.NextFoundPosition)
			||  Keypressed(input, Key.PrevFoundPosition))
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
#				ifdef HAVE_TAGLIB_H
				else if (myScreen->ActiveWindow() == myTagEditor->LeftColumn)
				{
					myTagEditor->Tags->Clear();
				}
#				endif // HAVE_TAGLIB_H
			}
		}
		
		if (myScreen == myPlaylist)
			myPlaylist->EnableHighlighting();
		
#		ifdef ENABLE_VISUALIZER
		// visualizer sets timmeout to 40ms, but since only it needs such small
		// value, we should restore defalt one after switching to another screen.
		if (wFooter->GetTimeout() < ncmpcpp_window_timeout
		&&  !(myScreen == myVisualizer || myLockedScreen == myVisualizer || myInactiveScreen == myVisualizer)
		   )
			wFooter->SetTimeout(ncmpcpp_window_timeout);
#		endif // ENABLE_VISUALIZER
	}
	return 0;
}

