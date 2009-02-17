/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include <clocale>
#include <csignal>

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
#include "lyrics.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "settings.h"
#include "song.h"
#include "info.h"
#include "status.h"
#include "tag_editor.h"

#define CHECK_MPD_MUSIC_DIR \
			if (Config.mpd_music_dir.empty()) \
			{ \
				ShowMessage("configuration variable mpd_music_dir is not set!"); \
				continue; \
			}

using namespace Global;
using namespace MPD;

using std::make_pair;
using std::string;
using std::vector;

BasicScreen *Global::myScreen;
BasicScreen *Global::myOldScreen;

Window *Global::wHeader;
Window *Global::wFooter;

Connection *Global::Mpd;

size_t Global::main_start_y;
size_t Global::main_height;

time_t Global::timer;

bool Global::block_progressbar_update = 0;
bool Global::block_item_list_update = 0;

bool Global::messages_allowed = 0;
bool Global::redraw_header = 1;

vector<int> Global::vFoundPositions;
int Global::found_pos = 0;

int main(int argc, char *argv[])
{
	CreateConfigDir();
	DefaultConfiguration(Config);
	DefaultKeys(Key);
	ReadConfiguration(Config);
	ReadKeys(Key);
	
	Mpd = new Connection;
	
	if (getenv("MPD_HOST"))
		Mpd->SetHostname(getenv("MPD_HOST"));
	if (getenv("MPD_PORT"))
		Mpd->SetPort(atoi(getenv("MPD_PORT")));
	
	if (Config.mpd_host != "localhost")
		Mpd->SetHostname(Config.mpd_host);
	if (Config.mpd_port != 6600)
		Mpd->SetPort(Config.mpd_port);
	
	Mpd->SetTimeout(Config.mpd_connection_timeout);
	
	if (argc > 1)
	{
		ParseArgv(argc, argv);
	}
	
	if (!ConnectToMPD())
		return -1;
	
	// redirect std::cerr output to ~/.ncmpcpp/error.log file
	std::ofstream errorlog((config_dir + "error.log").c_str(), std::ios::app);
	std::streambuf *cerr_buffer = std::cerr.rdbuf();
	std::cerr.rdbuf(errorlog.rdbuf());
	
	InitScreen(Config.colors_enabled);
	init_current_locale();
	
	main_start_y = 2;
	main_height = LINES-4;
	
	if (!Config.header_visibility)
	{
		main_start_y -= 2;
		main_height += 2;
	}
	if (!Config.statusbar_visibility)
		main_height++;
	
	myPlaylist->Init();
	myBrowser->Init();
	mySearcher->Init();
	myLibrary->Init();
	myPlaylistEditor->Init();
	
#	ifdef HAVE_TAGLIB_H
	myTinyTagEditor->Init();
	myTagEditor->Init();
#	endif // HAVE_TAGLIB_H
	
#	ifdef ENABLE_CLOCK
	myClock->Init();
#	endif // ENABLE_CLOCK
	
	myHelp->Init();
	myInfo->Init();
	myLyrics->Init();
	
	if (Config.header_visibility)
	{
		wHeader = new Window(0, 0, COLS, 1, "", Config.header_color, brNone);
		wHeader->Display();
	}
	
	size_t footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
	size_t footer_height = Config.statusbar_visibility ? 2 : 1;
	
	wFooter = new Window(0, footer_start_y, COLS, footer_height, "", Config.statusbar_color, brNone);
	wFooter->SetTimeout(ncmpcpp_window_timeout);
	wFooter->SetGetStringHelper(StatusbarGetStringHelper);
	wFooter->Display();
	
	myScreen = myPlaylist;
	
	timer = time(NULL);
	
	Mpd->SetStatusUpdater(NcmpcppStatusChanged, NULL);
	Mpd->SetErrorHandler(NcmpcppErrorCallback, NULL);
	
	// local variables
	int input;
	
	bool main_exit = 0;
	bool title_allowed = !Config.display_screens_numbers_on_start;
	
	string screen_title;
	
	timeval now, past;
	
	// local variables end
	
	signal(SIGPIPE, SIG_IGN);
	
	gettimeofday(&now, 0);
	
	while (!main_exit)
	{
		if (!Mpd->Connected())
		{
			ShowMessage("Attempting to reconnect...");
			if (Mpd->Connect())
				ShowMessage("Connected!");
			messages_allowed = 0;
		}
		
		TraceMpdStatus();
		
		block_item_list_update = 0;
		Playlist::BlockUpdate = 0;
		messages_allowed = 1;
		
		// header stuff
		gettimeofday(&past, 0);
		if (((past.tv_sec == now.tv_sec && past.tv_usec >= now.tv_usec+500000) || past.tv_sec > now.tv_sec)
		&&   (myScreen == myBrowser || myScreen == myLyrics)
		   )
		{
			redraw_header = 1;
			gettimeofday(&now, 0);
		}
		if (Config.header_visibility && redraw_header)
		{
			if (title_allowed)
			{
				*wHeader << XY(0, 0) << wclrtoeol << fmtBold << myScreen->Title() << fmtBoldEnd;
			}
			else
			{
				*wHeader << XY(0, 0)
				<< fmtBold << (char)Key.Help[0] << fmtBoldEnd << ":Help  "
				<< fmtBold << (char)Key.Playlist[0] << fmtBoldEnd << ":Playlist  "
				<< fmtBold << (char)Key.Browser[0] << fmtBoldEnd << ":Browse  "
				<< fmtBold << (char)Key.SearchEngine[0] << fmtBoldEnd << ":Search  "
				<< fmtBold << (char)Key.MediaLibrary[0] << fmtBoldEnd << ":Library  "
				<< fmtBold << (char)Key.PlaylistEditor[0] << fmtBoldEnd << ":Playlist editor";
#				ifdef HAVE_TAGLIB_H
				*wHeader << "  " << fmtBold << (char)Key.TagEditor[0] << fmtBoldEnd << ":Tag editor";
#				endif // HAVE_TAGLIB_H
#				ifdef ENABLE_CLOCK
				*wHeader << "  " << fmtBold << (char)Key.Clock[0] << fmtBoldEnd << ":Clock";
#				endif // ENABLE_CLOCK
			}
			
			wHeader->SetColor(Config.volume_color);
			*wHeader << XY(wHeader->GetWidth()-volume_state.length(), 0) << volume_state;
			wHeader->SetColor(Config.header_color);
			wHeader->Refresh();
			redraw_header = 0;
		}
		// header stuff end
		
		myScreen->Update();
		myScreen->RefreshWindow();
		myScreen->ReadKey(input);
		
		if (input == ERR)
			continue;
		
		if (!title_allowed)
			redraw_header = 1;
		title_allowed = 1;
		timer = time(NULL);
		
		if (myScreen == myPlaylist)
		{
			myPlaylist->Main()->Highlighting(1);
		}
		else if (
		   myScreen == myLibrary
		|| myScreen == myPlaylistEditor
#		ifdef HAVE_TAGLIB_H
		|| myScreen == myTagEditor
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
				if (myScreen->Cmp() == myLibrary->Artists)
				{
					myLibrary->Albums->Clear(0);
					myLibrary->Songs->Clear(0);
				}
				else if (myScreen->Cmp() == myLibrary->Albums)
				{
					myLibrary->Songs->Clear(0);
				}
				else if (myScreen->Cmp() == myPlaylistEditor->Playlists)
				{
					myPlaylistEditor->Content->Clear(0);
				}
#				ifdef HAVE_TAGLIB_H
				else if (myScreen->Cmp() == myTagEditor->LeftColumn)
				{
					myTagEditor->Tags->Clear(0);
					myTagEditor->TagTypes->Refresh();
				}
#				endif // HAVE_TAGLIB_H
			}
		}
		
		// key mapping beginning
		
		if (Keypressed(input, Key.Up))
		{
			myScreen->Scroll(wUp, Key.Up);
		}
		else if (Keypressed(input, Key.Down))
		{
			myScreen->Scroll(wDown, Key.Down);
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
		else if (input == KEY_RESIZE)
		{
			redraw_header = 1;
			
			if (COLS < 20 || LINES < 5)
			{
				endwin();
				std::cout << "Screen too small!\n";
				return 1;
			}
			
			main_height = LINES-4;
			
			if (!Config.header_visibility)
				main_height += 2;
			if (!Config.statusbar_visibility)
				main_height++;
			
			myHelp->hasToBeResized = 1;
			myPlaylist->hasToBeResized = 1;
			myBrowser->hasToBeResized = 1;
			mySearcher->hasToBeResized = 1;
			myLibrary->hasToBeResized = 1;
			myPlaylistEditor->hasToBeResized = 1;
			myInfo->hasToBeResized = 1;
			myLyrics->hasToBeResized = 1;
			
#			ifdef HAVE_TAGLIB_H
			myTinyTagEditor->hasToBeResized = 1;
			myTagEditor->hasToBeResized = 1;
#			endif // HAVE_TAGLIB_H
			
#			ifdef ENABLE_CLOCK
			myClock->hasToBeResized = 1;
#			endif // ENABLE_CLOCK
			
			myScreen->Resize();
			
			if (Config.header_visibility)
				wHeader->Resize(COLS, wHeader->GetHeight());
			
			footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
			wFooter->MoveTo(0, footer_start_y);
			wFooter->Resize(COLS, wFooter->GetHeight());
			
			myScreen->Refresh();
			PlayerState mpd_state = Mpd->GetState();
			StatusChanges changes;
			changes.StatusFlags = 1; // force status update
			if (mpd_state == psPlay || mpd_state == psPause)
				changes.ElapsedTime = 1; // restore status
			else
				changes.PlayerState = 1;
			
			NcmpcppStatusChanged(Mpd, changes, NULL);
		}
		else if (Keypressed(input, Key.GoToParentDir))
		{
			if (myScreen == myBrowser && myBrowser->CurrentDir() != "/")
			{
				myBrowser->Main()->Reset();
				myBrowser->EnterPressed();
			}
		}
		else if (Keypressed(input, Key.Enter))
		{
			myScreen->EnterPressed();
		}
		else if (Keypressed(input, Key.Space))
		{
			myScreen->SpacePressed();
		}
		else if (Keypressed(input, Key.VolumeUp))
		{
			if (myScreen == myLibrary && input == Key.VolumeUp[0])
			{
				myLibrary->NextColumn();
			}
			else if (myScreen == myPlaylistEditor && input == Key.VolumeUp[0])
			{
				myPlaylistEditor->NextColumn();
			}
#			ifdef HAVE_TAGLIB_H
			else if (myScreen == myTagEditor && input == Key.VolumeUp[0])
			{
				myTagEditor->NextColumn();
			}
#			endif // HAVE_TAGLIB_H
			else
				Mpd->SetVolume(Mpd->GetVolume()+1);
		}
		else if (Keypressed(input, Key.VolumeDown))
		{
			if (myScreen == myLibrary && input == Key.VolumeDown[0])
			{
				myLibrary->PrevColumn();
			}
			else if (myScreen == myPlaylistEditor && input == Key.VolumeDown[0])
			{
				myPlaylistEditor->PrevColumn();
			}
#			ifdef HAVE_TAGLIB_H
			else if (myScreen == myTagEditor && input == Key.VolumeDown[0])
			{
				myTagEditor->PrevColumn();
			}
#			endif // HAVE_TAGLIB_H
			else
				Mpd->SetVolume(Mpd->GetVolume()-1);
		}
		else if (Keypressed(input, Key.Delete))
		{
			if (!myPlaylist->Main()->Empty() && myScreen == myPlaylist)
			{
				Playlist::BlockUpdate = 1;
				if (myPlaylist->Main()->hasSelected())
				{
					vector<size_t> list;
					myPlaylist->Main()->GetSelected(list);
					for (vector<size_t>::const_reverse_iterator it = list.rbegin(); it != ((const vector<size_t> &)list).rend(); it++)
					{
						Mpd->QueueDeleteSongId((*myPlaylist->Main())[*it].GetID());
						myPlaylist->Main()->DeleteOption(*it);
					}
					ShowMessage("Selected items deleted!");
				}
				else
				{
					Playlist::BlockNowPlayingUpdate = 1;
					myPlaylist->Main()->SetTimeout(50);
					while (!myPlaylist->Main()->Empty() && Keypressed(input, Key.Delete))
					{
						size_t id = myPlaylist->Main()->Choice();
						TraceMpdStatus();
						timer = time(NULL);
						if (myPlaylist->NowPlaying > myPlaylist->CurrentSong()->GetPosition())  // needed for keeping proper
							myPlaylist->NowPlaying--; // position of now playing song.
						Mpd->QueueDeleteSongId(myPlaylist->CurrentSong()->GetID());
						myPlaylist->Main()->DeleteOption(id);
						myPlaylist->Main()->Refresh();
						myPlaylist->Main()->ReadKey(input);
					}
					myPlaylist->Main()->SetTimeout(ncmpcpp_window_timeout);
					Playlist::BlockNowPlayingUpdate = 0;
				}
				Mpd->CommitQueue();
			}
			else if (myScreen == myBrowser || myScreen->Cmp() == myPlaylistEditor->Playlists)
			{
				LockStatusbar();
				string name = myScreen == myBrowser ? myBrowser->Main()->Current().name : myPlaylistEditor->Playlists->Current();
				if (myScreen != myBrowser || myBrowser->Main()->Current().type == itPlaylist)
				{
					Statusbar() << "Delete playlist " << name << " ? [y/n] ";
					curs_set(1);
					int in = 0;
					do
					{
						TraceMpdStatus();
						wFooter->ReadKey(in);
					}
					while (in != 'y' && in != 'n');
					if (in == 'y')
					{
						Mpd->DeletePlaylist(locale_to_utf_cpy(name));
						ShowMessage("Playlist %s deleted!", name.c_str());
						if (!Config.local_browser)
							myBrowser->GetDirectory("/");
					}
					else
						ShowMessage("Aborted!");
					curs_set(0);
					myPlaylistEditor->Playlists->Clear(0); // make playlists list update itself
				}
				UnlockStatusbar();
			}
			else if (myScreen->Cmp() == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
			{
				if (myPlaylistEditor->Content->hasSelected())
				{
					vector<size_t> list;
					myPlaylistEditor->Content->GetSelected(list);
					locale_to_utf(myPlaylistEditor->Playlists->Current());
					for (vector<size_t>::const_reverse_iterator it = list.rbegin(); it != ((const vector<size_t> &)list).rend(); it++)
					{
						Mpd->QueueDeleteFromPlaylist(myPlaylistEditor->Playlists->Current(), *it);
						myPlaylistEditor->Content->DeleteOption(*it);
					}
					utf_to_locale(myPlaylistEditor->Playlists->Current());
					ShowMessage("Selected items deleted from playlist '%s'!", myPlaylistEditor->Playlists->Current().c_str());
				}
				else
				{
					myPlaylistEditor->Content->SetTimeout(50);
					locale_to_utf(myPlaylistEditor->Playlists->Current());
					while (!myPlaylistEditor->Content->Empty() && Keypressed(input, Key.Delete))
					{
						TraceMpdStatus();
						timer = time(NULL);
						Mpd->QueueDeleteFromPlaylist(myPlaylistEditor->Playlists->Current(), myPlaylistEditor->Content->Choice());
						myPlaylistEditor->Content->DeleteOption(myPlaylistEditor->Content->Choice());
						myPlaylistEditor->Content->Refresh();
						myPlaylistEditor->Content->ReadKey(input);
					}
					utf_to_locale(myPlaylistEditor->Playlists->Current());
					myPlaylistEditor->Content->SetTimeout(ncmpcpp_window_timeout);
				}
				Mpd->CommitQueue();
			}
		}
		else if (Keypressed(input, Key.Prev))
		{
			Mpd->Prev();
		}
		else if (Keypressed(input, Key.Next))
		{
			Mpd->Next();
		}
		else if (Keypressed(input, Key.Pause))
		{
			Mpd->Pause();
		}
		else if (Keypressed(input, Key.SavePlaylist))
		{
			LockStatusbar();
			Statusbar() << "Save playlist as: ";
			string playlist_name = wFooter->GetString();
			string real_playlist_name = playlist_name;
			locale_to_utf(real_playlist_name);
			UnlockStatusbar();
			if (playlist_name.find("/") != string::npos)
			{
				ShowMessage("Playlist name cannot contain slashes!");
				continue;
			}
			if (!playlist_name.empty())
			{
				if (Mpd->SavePlaylist(real_playlist_name))
				{
					ShowMessage("Playlist saved as: %s", playlist_name.c_str());
					myPlaylistEditor->Playlists->Clear(0); // make playlist's list update itself
				}
				else
				{
					LockStatusbar();
					Statusbar() << "Playlist already exists, overwrite: " << playlist_name << " ? [y/n] ";
					curs_set(1);
					int in = 0;
					messages_allowed = 0;
					while (in != 'y' && in != 'n')
					{
						Mpd->UpdateStatus();
						wFooter->ReadKey(in);
					}
					messages_allowed = 1;
					
					if (in == 'y')
					{
						Mpd->DeletePlaylist(real_playlist_name);
						if (Mpd->SavePlaylist(real_playlist_name))
							ShowMessage("Playlist overwritten!");
					}
					else
						ShowMessage("Aborted!");
					curs_set(0);
					myPlaylistEditor->Playlists->Clear(0); // make playlist's list update itself
					UnlockStatusbar();
				}
			}
			if (!Config.local_browser && myBrowser->CurrentDir() == "/" && !myBrowser->Main()->Empty())
				myBrowser->GetDirectory(myBrowser->CurrentDir());
		}
		else if (Keypressed(input, Key.Stop))
		{
			Mpd->Stop();
		}
		else if (Keypressed(input, Key.MvSongUp))
		{
			if (myScreen == myPlaylist && !myPlaylist->Main()->Empty())
			{
				if (myPlaylist->Main()->isFiltered())
				{
					ShowMessage("%s", MPD::Message::FunctionDisabledFilteringEnabled);
					continue;
				}
				
				Playlist::BlockUpdate = 1;
				myPlaylist->Main()->SetTimeout(50);
				if (myPlaylist->Main()->hasSelected())
				{
					vector<size_t> list;
					myPlaylist->Main()->GetSelected(list);
					
					for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						if (*it == size_t(myPlaylist->NowPlaying) && list.front() > 0)
							myPlaylist->Main()->BoldOption(myPlaylist->NowPlaying, 0);
					
					vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongUp) && list.front() > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						{
							(*it)--;
							myPlaylist->Main()->Swap(*it, (*it)+1);
						}
						myPlaylist->Main()->Highlight(list[(list.size()-1)/2]);
						myPlaylist->Main()->Refresh();
						myPlaylist->Main()->ReadKey(input);
					}
					for (size_t i = 0; i < list.size(); i++)
						Mpd->QueueMove(origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					size_t from, to;
					from = to = myPlaylist->Main()->Choice();
					// unbold now playing as if song changes during move, this won't be unbolded.
					if (to == size_t(myPlaylist->NowPlaying) && to > 0)
						myPlaylist->Main()->BoldOption(myPlaylist->NowPlaying, 0);
					while (Keypressed(input, Key.MvSongUp) && to > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						to--;
						myPlaylist->Main()->Swap(to, to+1);
						myPlaylist->Main()->Scroll(wUp);
						myPlaylist->Main()->Refresh();
						myPlaylist->Main()->ReadKey(input);
					}
					Mpd->Move(from, to);
				}
				myPlaylist->Main()->SetTimeout(ncmpcpp_window_timeout);
			}
			else if (myScreen->Cmp() == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
			{
				myPlaylistEditor->Content->SetTimeout(50);
				if (myPlaylistEditor->Content->hasSelected())
				{
					vector<size_t> list;
					myPlaylistEditor->Content->GetSelected(list);
					
					vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongUp) && list.front() > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						{
							(*it)--;
							myPlaylistEditor->Content->Swap(*it, (*it)+1);
						}
						myPlaylistEditor->Content->Highlight(list[(list.size()-1)/2]);
						myPlaylistEditor->Content->Refresh();
						myPlaylistEditor->Content->ReadKey(input);
					}
					for (size_t i = 0; i < list.size(); i++)
						if (origs[i] != list[i])
							Mpd->QueueMove(myPlaylistEditor->Playlists->Current(), origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					size_t from, to;
					from = to = myPlaylistEditor->Content->Choice();
					while (Keypressed(input, Key.MvSongUp) && to > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						to--;
						myPlaylistEditor->Content->Swap(to, to+1);
						myPlaylistEditor->Content->Scroll(wUp);
						myPlaylistEditor->Content->Refresh();
						myPlaylistEditor->Content->ReadKey(input);
					}
					if (from != to)
						Mpd->Move(myPlaylistEditor->Playlists->Current(), from, to);
				}
				myPlaylistEditor->Content->SetTimeout(ncmpcpp_window_timeout);
			}
		}
		else if (Keypressed(input, Key.MvSongDown))
		{
			if (myScreen == myPlaylist && !myPlaylist->Main()->Empty())
			{
				if (myPlaylist->Main()->isFiltered())
				{
					ShowMessage("%s", MPD::Message::FunctionDisabledFilteringEnabled);
					continue;
				}
				
				Playlist::BlockUpdate = 1;
				myPlaylist->Main()->SetTimeout(50);
				if (myPlaylist->Main()->hasSelected())
				{
					vector<size_t> list;
					myPlaylist->Main()->GetSelected(list);
					
					for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						if (*it == size_t(myPlaylist->NowPlaying) && list.back() < myPlaylist->Main()->Size()-1)
							myPlaylist->Main()->BoldOption(myPlaylist->NowPlaying, 0);
					
					vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongDown) && list.back() < myPlaylist->Main()->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); it++)
						{
							(*it)++;
							myPlaylist->Main()->Swap(*it, (*it)-1);
						}
						myPlaylist->Main()->Highlight(list[(list.size()-1)/2]);
						myPlaylist->Main()->Refresh();
						myPlaylist->Main()->ReadKey(input);
					}
					for (int i = list.size()-1; i >= 0; i--)
						Mpd->QueueMove(origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					size_t from, to;
					from = to = myPlaylist->Main()->Choice();
					// unbold now playing as if song changes during move, this won't be unbolded.
					if (to == size_t(myPlaylist->NowPlaying) && to < myPlaylist->Main()->Size()-1)
						myPlaylist->Main()->BoldOption(myPlaylist->NowPlaying, 0);
					while (Keypressed(input, Key.MvSongDown) && to < myPlaylist->Main()->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						to++;
						myPlaylist->Main()->Swap(to, to-1);
						myPlaylist->Main()->Scroll(wDown);
						myPlaylist->Main()->Refresh();
						myPlaylist->Main()->ReadKey(input);
					}
					Mpd->Move(from, to);
				}
				myPlaylist->Main()->SetTimeout(ncmpcpp_window_timeout);
				
			}
			else if (myScreen->Cmp() == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
			{
				myPlaylistEditor->Content->SetTimeout(50);
				if (myPlaylistEditor->Content->hasSelected())
				{
					vector<size_t> list;
					myPlaylistEditor->Content->GetSelected(list);
					
					vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongDown) && list.back() < myPlaylistEditor->Content->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); it++)
						{
							(*it)++;
							myPlaylistEditor->Content->Swap(*it, (*it)-1);
						}
						myPlaylistEditor->Content->Highlight(list[(list.size()-1)/2]);
						myPlaylistEditor->Content->Refresh();
						myPlaylistEditor->Content->ReadKey(input);
					}
					for (int i = list.size()-1; i >= 0; i--)
						if (origs[i] != list[i])
							Mpd->QueueMove(myPlaylistEditor->Playlists->Current(), origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					size_t from, to;
					from = to = myPlaylistEditor->Content->Choice();
					while (Keypressed(input, Key.MvSongDown) && to < myPlaylistEditor->Content->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						to++;
						myPlaylistEditor->Content->Swap(to, to-1);
						myPlaylistEditor->Content->Scroll(wDown);
						myPlaylistEditor->Content->Refresh();
						myPlaylistEditor->Content->ReadKey(input);
					}
					if (from != to)
						Mpd->Move(myPlaylistEditor->Playlists->Current(), from, to);
				}
				myPlaylistEditor->Content->SetTimeout(ncmpcpp_window_timeout);
			}
		}
		else if (Keypressed(input, Key.Add))
		{
			LockStatusbar();
			Statusbar() << "Add: ";
			string path = wFooter->GetString();
			UnlockStatusbar();
			if (!path.empty())
			{
				SongList list;
				Mpd->GetDirectoryRecursive(path, list);
				if (!list.empty())
				{
					for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
						Mpd->QueueAddSong(**it);
					if (Mpd->CommitQueue())
					{
						Song &s = myPlaylist->Main()->at(myPlaylist->Main()->Size()-list.size());
						if (s.GetHash() != list[0]->GetHash())
							ShowMessage("%s", MPD::Message::PartOfSongsAdded);
					}
				}
				else
					Mpd->AddSong(path);
				FreeSongList(list);
			}
		}
		else if (Keypressed(input, Key.SeekForward) || Keypressed(input, Key.SeekBackward))
		{
			if (!myPlaylist->isPlaying())
				continue;
			
			const Song &s = Mpd->GetCurrentSong();
			
			if (!s.GetTotalLength())
			{
				ShowMessage("Unknown item length!");
				continue;
			}
			block_progressbar_update = 1;
			LockStatusbar();
			
			int songpos;
			time_t t = time(NULL);
			
			songpos = Mpd->GetElapsedTime();
			
			while (Keypressed(input, Key.SeekForward) || Keypressed(input, Key.SeekBackward))
			{
				TraceMpdStatus();
				timer = time(NULL);
				myPlaylist->Main()->ReadKey(input);
				
				int howmuch = Config.incremental_seeking ? (timer-t)/2+Config.seek_time : Config.seek_time;
				
				if (songpos < s.GetTotalLength() && Keypressed(input, Key.SeekForward))
				{
					songpos += howmuch;
					if (songpos > s.GetTotalLength())
						songpos = s.GetTotalLength();
				}
				if (songpos < s.GetTotalLength() && songpos > 0 && Keypressed(input, Key.SeekBackward))
				{
					songpos -= howmuch;
					if (songpos < 0)
						songpos = 0;
				}
				
				wFooter->Bold(1);
				string tracklength = "[" + Song::ShowTime(songpos) + "/" + s.GetLength() + "]";
				*wFooter << XY(wFooter->GetWidth()-tracklength.length(), 1) << tracklength;
				double progressbar_size = (double)songpos/(s.GetTotalLength());
				int howlong = wFooter->GetWidth()*progressbar_size;
				
				mvwhline(wFooter->Raw(), 0, 0, 0, wFooter->GetWidth());
				mvwhline(wFooter->Raw(), 0, 0, '=',howlong);
				mvwaddch(wFooter->Raw(), 0, howlong, '>');
				wFooter->Bold(0);
				wFooter->Refresh();
			}
			Mpd->Seek(songpos);
			
			block_progressbar_update = 0;
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
					myPlaylist->Main()->SetItemDisplayer(Display::SongsInColumns);
					myPlaylist->Main()->SetItemDisplayerUserData(&Config.song_columns_list_format);
					myPlaylist->Main()->SetTitle(Display::Columns(Config.song_columns_list_format));
					myPlaylist->Main()->SetGetStringFunction(Playlist::SongInColumnsToString);
					myPlaylist->Main()->SetGetStringFunctionUserData(&Config.song_columns_list_format);
				}
				else
				{
					myPlaylist->Main()->SetItemDisplayer(Display::Songs);
					myPlaylist->Main()->SetItemDisplayerUserData(&Config.song_list_format);
					myPlaylist->Main()->SetTitle("");
					myPlaylist->Main()->SetGetStringFunction(Playlist::SongToString);
					myPlaylist->Main()->SetGetStringFunctionUserData(&Config.song_list_format);
				}
			}
			else if (myScreen == myBrowser)
			{
				Config.columns_in_browser = !Config.columns_in_browser;
				ShowMessage("Browser display mode: %s", Config.columns_in_browser ? "Columns" : "Classic");
				myBrowser->Main()->SetTitle(Config.columns_in_browser ? Display::Columns(Config.song_columns_list_format) : "");
				
			}
			else if (myScreen == mySearcher)
			{
				Config.columns_in_search_engine = !Config.columns_in_search_engine;
				ShowMessage("Search engine display mode: %s", Config.columns_in_search_engine ? "Columns" : "Classic");
				if (mySearcher->Main()->Size() > SearchEngine::StaticOptions)
					mySearcher->Main()->SetTitle(Config.columns_in_search_engine ? Display::Columns(Config.song_columns_list_format) : "");
			}
		}
#		ifdef HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ToggleLyricsDB))
		{
			const char *current_lyrics_plugin = Lyrics::GetPluginName(++Config.lyrics_db);
			if (!current_lyrics_plugin)
			{
				current_lyrics_plugin = Lyrics::GetPluginName(Config.lyrics_db = 0);
			}
			ShowMessage("Using lyrics database: %s", current_lyrics_plugin);
		}
#		endif // HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ToggleAutoCenter))
		{
			Config.autocenter_mode = !Config.autocenter_mode;
			ShowMessage("Auto center mode: %s", Config.autocenter_mode ? "On" : "Off");
			if (Config.autocenter_mode && myPlaylist->isPlaying() && !myPlaylist->Main()->isFiltered())
				myPlaylist->Main()->Highlight(myPlaylist->NowPlaying);
		}
		else if (Keypressed(input, Key.UpdateDB))
		{
			if (myScreen == myBrowser)
				Mpd->UpdateDirectory(myBrowser->CurrentDir());
#			ifdef HAVE_TAGLIB_H
			else if (myScreen == myTagEditor && !Config.albums_in_tag_editor)
				Mpd->UpdateDirectory(myTagEditor->CurrentDir());
#			endif // HAVE_TAGLIB_H
			else
				Mpd->UpdateDirectory("/");
		}
		else if (Keypressed(input, Key.GoToNowPlaying))
		{
			if (myScreen != myPlaylist || !myPlaylist->isPlaying())
				continue;
			if (myPlaylist->Main()->isFiltered())
			{
				ShowMessage("%s", MPD::Message::FunctionDisabledFilteringEnabled);
				continue;
			}
			myPlaylist->Main()->Highlight(myPlaylist->NowPlaying);
		}
		else if (Keypressed(input, Key.ToggleRepeat))
		{
			Mpd->SetRepeat(!Mpd->GetRepeat());
		}
		else if (Keypressed(input, Key.ToggleRepeatOne))
		{
			Config.repeat_one_mode = !Config.repeat_one_mode;
			ShowMessage("'Repeat one' mode: %s", Config.repeat_one_mode ? "On" : "Off");
		}
		else if (Keypressed(input, Key.Shuffle))
		{
			Mpd->Shuffle();
		}
		else if (Keypressed(input, Key.ToggleRandom))
		{
			Mpd->SetRandom(!Mpd->GetRandom());
		}
		else if (Keypressed(input, Key.ToggleCrossfade))
		{
			Mpd->SetCrossfade(Mpd->GetCrossfade() ? 0 : Config.crossfade_time);
		}
		else if (Keypressed(input, Key.SetCrossfade))
		{
			LockStatusbar();
			Statusbar() << "Set crossfade to: ";
			string crossfade = wFooter->GetString(3);
			UnlockStatusbar();
			int cf = StrToInt(crossfade);
			if (cf > 0)
			{
				Config.crossfade_time = cf;
				Mpd->SetCrossfade(cf);
			}
		}
		else if (Keypressed(input, Key.EditTags))
		{
			CHECK_MPD_MUSIC_DIR;
#			ifdef HAVE_TAGLIB_H
			if (myTinyTagEditor->SetEdited(myScreen->CurrentSong()))
			{
				myTinyTagEditor->SwitchTo();
			}
			else if (myScreen->Cmp() == myLibrary->Artists)
			{
				LockStatusbar();
				Statusbar() << fmtBold << IntoStr(Config.media_lib_primary_tag) << fmtBoldEnd << ": ";
				string new_tag = wFooter->GetString(myLibrary->Artists->Current());
				UnlockStatusbar();
				if (!new_tag.empty() && new_tag != myLibrary->Artists->Current())
				{
					bool success = 1;
					SongList list;
					ShowMessage("Updating tags...");
					Mpd->StartSearch(1);
					Mpd->AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(myLibrary->Artists->Current()));
					Mpd->CommitSearch(list);
					SongSetFunction set = IntoSetFunction(Config.media_lib_primary_tag);
					if (!set)
						continue;
					for (SongList::iterator it = list.begin(); it != list.end(); it++)
					{
						(*it)->Localize();
						((*it)->*set)(new_tag);
						ShowMessage("Updating tags in '%s'...", (*it)->GetName().c_str());
						string path = Config.mpd_music_dir + (*it)->GetFile();
						if (!TagEditor::WriteTags(**it))
						{
							ShowMessage("Error updating tags in '%s'!", (*it)->GetFile().c_str());
							success = 0;
							break;
						}
					}
					if (success)
					{
						Mpd->UpdateDirectory(FindSharedDir(list));
						ShowMessage("Tags updated succesfully!");
					}
					FreeSongList(list);
				}
			}
			else if (myScreen->Cmp() == myLibrary->Albums)
			{
				LockStatusbar();
				Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
				string new_album = wFooter->GetString(myLibrary->Albums->Current().second);
				UnlockStatusbar();
				if (!new_album.empty() && new_album != myLibrary->Albums->Current().second)
				{
					bool success = 1;
					ShowMessage("Updating tags...");
					for (size_t i = 0;  i < myLibrary->Songs->Size(); i++)
					{
						(*myLibrary->Songs)[i].Localize();
						ShowMessage("Updating tags in '%s'...", (*myLibrary->Songs)[i].GetName().c_str());
						string path = Config.mpd_music_dir + (*myLibrary->Songs)[i].GetFile();
						TagLib::FileRef f(path.c_str());
						if (f.isNull())
						{
							ShowMessage("Error opening file '%s'!", (*myLibrary->Songs)[i].GetFile().c_str());
							success = 0;
							break;
						}
						f.tag()->setAlbum(ToWString(new_album));
						if (!f.save())
						{
							ShowMessage("Error writing tags in '%s'!", (*myLibrary->Songs)[i].GetFile().c_str());
							success = 0;
							break;
						}
					}
					if (success)
					{
						Mpd->UpdateDirectory(FindSharedDir(myLibrary->Songs));
						ShowMessage("Tags updated succesfully!");
					}
				}
			}
			else if (myScreen->Cmp() == myTagEditor->Dirs)
			{
				string old_dir = myTagEditor->Dirs->Current().first;
				LockStatusbar();
				Statusbar() << fmtBold << "Directory: " << fmtBoldEnd;
				string new_dir = wFooter->GetString(old_dir);
				UnlockStatusbar();
				if (!new_dir.empty() && new_dir != old_dir)
				{
					string full_old_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + locale_to_utf_cpy(old_dir);
					string full_new_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + locale_to_utf_cpy(new_dir);
					if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
					{
						Mpd->UpdateDirectory(myTagEditor->CurrentDir());
					}
					else
					{
						ShowMessage("Cannot rename '%s' to '%s'!", old_dir.c_str(), new_dir.c_str());
					}
				}
			}
			else
#			endif // HAVE_TAGLIB_H
			if (myScreen == myBrowser && myBrowser->Main()->Current().type == itDirectory)
			{
				string old_dir = myBrowser->Main()->Current().name;
				LockStatusbar();
				Statusbar() << fmtBold << "Directory: " << fmtBoldEnd;
				string new_dir = wFooter->GetString(old_dir);
				UnlockStatusbar();
				if (!new_dir.empty() && new_dir != old_dir)
				{
					string full_old_dir;
					if (!Config.local_browser)
						full_old_dir += Config.mpd_music_dir;
					full_old_dir += locale_to_utf_cpy(old_dir);
					string full_new_dir;
					if (!Config.local_browser)
						full_new_dir += Config.mpd_music_dir;
					full_new_dir += locale_to_utf_cpy(new_dir);
					int rename_result = rename(full_old_dir.c_str(), full_new_dir.c_str());
					if (rename_result == 0)
					{
						ShowMessage("'%s' renamed to '%s'", old_dir.c_str(), new_dir.c_str());
						if (!Config.local_browser)
							Mpd->UpdateDirectory(FindSharedDir(old_dir, new_dir));
						myBrowser->GetDirectory(myBrowser->CurrentDir());
					}
					else
						ShowMessage("Cannot rename '%s' to '%s'!", old_dir.c_str(), new_dir.c_str());
				}
			}
			else if (myScreen->Cmp() == myPlaylistEditor->Playlists || (myScreen == myBrowser && myBrowser->Main()->Current().type == itPlaylist))
			{
				string old_name = myScreen->Cmp() == myPlaylistEditor->Playlists ? myPlaylistEditor->Playlists->Current() : myBrowser->Main()->Current().name;
				LockStatusbar();
				Statusbar() << fmtBold << "Playlist: " << fmtBoldEnd;
				string new_name = wFooter->GetString(old_name);
				UnlockStatusbar();
				if (!new_name.empty() && new_name != old_name)
				{
					Mpd->Rename(locale_to_utf_cpy(old_name), locale_to_utf_cpy(new_name));
					ShowMessage("Playlist '%s' renamed to '%s'", old_name.c_str(), new_name.c_str());
					if (!Config.local_browser)
						myBrowser->GetDirectory("/");
					myPlaylistEditor->Playlists->Clear(0);
				}
			}
		}
		else if (Keypressed(input, Key.GoToContainingDir))
		{
			Song *s = myScreen->CurrentSong();
			
			if (!s || s->GetDirectory().empty())
				continue;
			
			Config.local_browser = !s->IsFromDB();
			
			string option = s->toString(Config.song_status_format);
			locale_to_utf(option);
			myBrowser->GetDirectory(s->GetDirectory());
			for (size_t i = 0; i < myBrowser->Main()->Size(); i++)
			{
				if (myBrowser->Main()->at(i).type == itSong && option == myBrowser->Main()->at(i).song->toString(Config.song_status_format))
				{
					myBrowser->Main()->Highlight(i);
					break;
				}
			}
			myBrowser->SwitchTo();
		}
		else if (Keypressed(input, Key.StartSearching))
		{
			if (myScreen == mySearcher)
			{
				mySearcher->Main()->Highlight(SearchEngine::SearchButton);
				mySearcher->Main()->Highlighting(0);
				mySearcher->Main()->Refresh();
				mySearcher->Main()->Highlighting(1);
				mySearcher->EnterPressed();
			}
		}
		else if (Keypressed(input, Key.GoToPosition))
		{
			if (!myPlaylist->isPlaying())
				continue;
			
			const Song &s = Mpd->GetCurrentSong();
			
			if (!s.GetTotalLength())
			{
				ShowMessage("Unknown item length!");
				continue;
			}
			LockStatusbar();
			Statusbar() << "Position to go (in %): ";
			string position = wFooter->GetString(3);
			int newpos = StrToInt(position);
			if (newpos > 0 && newpos < 100 && !position.empty())
				Mpd->Seek(s.GetTotalLength()*newpos/100.0);
			UnlockStatusbar();
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
				if (myScreen->GetList()->Deselect())
				{
					ShowMessage("Items deselected!");
				}
			}
		}
		else if (Keypressed(input, Key.AddSelected))
		{
			if (!myScreen->allowsSelection())
				continue;
			
			SongList result;
			myScreen->GetSelectedSongs(result);
			
			if (result.empty())
			{
				ShowMessage("No selected items!");
				continue;
			}
			
			const size_t dialog_width = COLS*0.8;
			const size_t dialog_height = LINES*0.6;
			
			Menu<string> *mDialog = new Menu<string>((COLS-dialog_width)/2, (LINES-dialog_height)/2, dialog_width, dialog_height, "Add selected items to...", Config.main_color, Config.window_border);
			mDialog->SetTimeout(ncmpcpp_window_timeout);
			mDialog->SetItemDisplayer(Display::Generic);
			
			bool playlists_not_active = myScreen == myBrowser && Config.local_browser;
			
			if (playlists_not_active)
			{
				ShowMessage("Local items cannot be added to m3u playlist!");
			}
			
			mDialog->AddOption("Current MPD playlist");
			mDialog->AddOption("Create new playlist (m3u file)", 0, playlists_not_active);
			mDialog->AddSeparator();
			TagList playlists;
			Mpd->GetPlaylists(playlists);
			for (TagList::iterator it = playlists.begin(); it != playlists.end(); it++)
			{
				utf_to_locale(*it);
				mDialog->AddOption("'" + *it + "' playlist", 0, playlists_not_active);
			}
			mDialog->AddSeparator();
			mDialog->AddOption("Cancel");
			
			mDialog->Display();
			
			myOldScreen = myScreen;
			myScreen = myHelp; // temp hack, prevent playlist from updating
			
			while (!Keypressed(input, Key.Enter))
			{
				TraceMpdStatus();
				mDialog->Refresh();
				mDialog->ReadKey(input);
				
				if (Keypressed(input, Key.Up))
					mDialog->Scroll(wUp);
				else if (Keypressed(input, Key.Down))
					mDialog->Scroll(wDown);
				else if (Keypressed(input, Key.PageUp))
					mDialog->Scroll(wPageUp);
				else if (Keypressed(input, Key.PageDown))
					mDialog->Scroll(wPageDown);
				else if (Keypressed(input, Key.Home))
					mDialog->Scroll(wHome);
				else if (Keypressed(input, Key.End))
					mDialog->Scroll(wEnd);
			}
			
			myScreen = myOldScreen;
			
			size_t id = mDialog->Choice();
			
			myScreen->Refresh();
			
			if (id == 0)
			{
				for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
					Mpd->QueueAddSong(**it);
				if (Mpd->CommitQueue())
				{
					ShowMessage("Selected items added!");
					Song &s = myPlaylist->Main()->at(myPlaylist->Main()->Size()-result.size());
					if (s.GetHash() != result[0]->GetHash())
						ShowMessage("%s", MPD::Message::PartOfSongsAdded);
				}
			}
			else if (id == 1)
			{
				LockStatusbar();
				Statusbar() << "Save playlist as: ";
				string playlist = wFooter->GetString();
				string real_playlist = playlist;
				locale_to_utf(real_playlist);
				UnlockStatusbar();
				if (!playlist.empty())
				{
					for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
						Mpd->QueueAddToPlaylist(real_playlist, **it);
					Mpd->CommitQueue();
					ShowMessage("Selected items added to playlist '%s'!", playlist.c_str());
				}
			}
			else if (id > 1 && id < mDialog->Size()-1)
			{
				locale_to_utf(playlists[id-3]);
				for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
					Mpd->QueueAddToPlaylist(playlists[id-3], **it);
				Mpd->CommitQueue();
				utf_to_locale(playlists[id-3]);
				ShowMessage("Selected items added to playlist '%s'!", playlists[id-3].c_str());
			}
			
			if (id != mDialog->Size()-1)
			{
				// refresh playlist's lists
				if (!Config.local_browser && myBrowser->CurrentDir() == "/")
					myBrowser->GetDirectory("/");
				myPlaylistEditor->Playlists->Clear(0); // make playlist editor update itself
			}
			timer = time(NULL);
			delete mDialog;
			FreeSongList(result);
		}
		else if (Keypressed(input, Key.Crop))
		{
			if (myPlaylist->Main()->isFiltered())
			{
				ShowMessage("%s", MPD::Message::FunctionDisabledFilteringEnabled);
				continue;
			}
			if (myPlaylist->Main()->hasSelected())
			{
				for (int i = myPlaylist->Main()->Size()-1; i >= 0; i--)
				{
					if (!myPlaylist->Main()->isSelected(i) && i != myPlaylist->NowPlaying)
						Mpd->QueueDeleteSong(i);
				}
				// if mpd deletes now playing song deletion will be sluggishly slow
				// then so we have to assure it will be deleted at the very end.
				if (myPlaylist->isPlaying() && !myPlaylist->Main()->isSelected(myPlaylist->NowPlaying))
					Mpd->QueueDeleteSongId(myPlaylist->NowPlayingSong().GetID());
				
				ShowMessage("Deleting all items but selected...");
				Mpd->CommitQueue();
				ShowMessage("Items deleted!");
			}
			else
			{
				if (!myPlaylist->isPlaying())
				{
					ShowMessage("Nothing is playing now!");
					continue;
				}
				for (int i = myPlaylist->Main()->Size()-1; i >= 0; i--)
					if (i != myPlaylist->NowPlaying)
						Mpd->QueueDeleteSong(i);
				ShowMessage("Deleting all items except now playing one...");
				Mpd->CommitQueue();
				ShowMessage("Items deleted!");
			}
		}
		else if (Keypressed(input, Key.Clear))
		{
			ShowMessage("Clearing playlist...");
			Mpd->ClearPlaylist();
			ShowMessage("Cleared playlist!");
		}
		else if (Keypressed(input, Key.ApplyFilter))
		{
			List *mList = myScreen->GetList();
			
			if (!mList)
				continue;
			
			CLEAR_FIND_HISTORY;
			
			LockStatusbar();
			Statusbar() << fmtBold << "Apply filter: " << fmtBoldEnd;
			wFooter->SetGetStringHelper(StatusbarApplyFilterImmediately);
			wFooter->GetString(mList->GetFilter());
			wFooter->SetGetStringHelper(StatusbarGetStringHelper);
			UnlockStatusbar();
			
			if (mList->isFiltered())
				ShowMessage("Using filter '%s'", mList->GetFilter().c_str());
			else
				ShowMessage("Filtering disabled");
			
			if (myScreen == myPlaylist)
			{
				timer = time(NULL);
				myPlaylist->Main()->Highlighting(1);
				redraw_header = 1;
			}
		}
		else if (Keypressed(input, Key.FindForward) || Keypressed(input, Key.FindBackward))
		{
			List *mList = myScreen->GetList();
			
			if (!mList)
				continue;
			
			string how = Keypressed(input, Key.FindForward) ? "forward" : "backward";
			LockStatusbar();
			Statusbar() << "Find " << how << ": ";
			string findme = wFooter->GetString();
			UnlockStatusbar();
			timer = time(NULL);
			if (findme.empty())
				continue;
			ToLower(findme);
			
			CLEAR_FIND_HISTORY;
			
			ShowMessage("Searching...");
			for (size_t i = (myScreen == mySearcher ? SearchEngine::StaticOptions : 0); i < mList->Size(); i++)
			{
				string name = mList->GetOption(i);
				ToLower(name);
				if (name.find(findme) != string::npos && !mList->isStatic(i))
				{
					vFoundPositions.push_back(i);
					if (Keypressed(input, Key.FindForward)) // forward
					{
						if (found_pos < 0 && i >= mList->Choice())
							found_pos = vFoundPositions.size()-1;
					}
					else // backward
					{
						if (i <= mList->Choice())
							found_pos = vFoundPositions.size()-1;
					}
				}
			}
			ShowMessage("Searching finished!");
			
			if (Config.wrapped_search ? vFoundPositions.empty() : found_pos < 0)
				ShowMessage("Unable to find \"%s\"", findme.c_str());
			else
			{
				mList->Highlight(vFoundPositions[found_pos < 0 ? 0 : found_pos]);
				if (myScreen == myPlaylist)
				{
					timer = time(NULL);
					myPlaylist->Main()->Highlighting(1);
				}
			}
		}
		else if (Keypressed(input, Key.NextFoundPosition) || Keypressed(input, Key.PrevFoundPosition))
		{
			List *mList = myScreen->GetList();
			
			if (!mList || vFoundPositions.empty())
				continue;
			
			bool next = Keypressed(input, Key.NextFoundPosition);
			
			try
			{
				mList->Highlight(vFoundPositions.at(next ? ++found_pos : --found_pos));
			}
			catch (std::out_of_range)
			{
				if (Config.wrapped_search)
				{
					if (next)
					{
						mList->Highlight(vFoundPositions.front());
						found_pos = 0;
					}
					else // prev
					{
						mList->Highlight(vFoundPositions.back());
						found_pos = vFoundPositions.size()-1;
					}
				}
				else
					found_pos = next ? vFoundPositions.size()-1 : 0;
			}
		}
		else if (Keypressed(input, Key.ToggleFindMode))
		{
			Config.wrapped_search = !Config.wrapped_search;
			ShowMessage("Search mode: %s", Config.wrapped_search ? "Wrapped" : "Normal");
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
		else if (Keypressed(input, Key.SwitchTagTypeList))
		{
			if (myScreen == myBrowser)
			{
				myBrowser->ChangeBrowseMode();
			}
			else if (myScreen->Cmp() == myLibrary->Artists)
			{
				LockStatusbar();
				Statusbar() << "Tag type ? [" << fmtBold << 'a' << fmtBoldEnd << "rtist/" << fmtBold << 'y' << fmtBoldEnd << "ear/" << fmtBold << 'g' << fmtBoldEnd << "enre/" << fmtBold << 'c' << fmtBoldEnd << "omposer/" << fmtBold << 'p' << fmtBoldEnd << "erformer] ";
				int item;
				curs_set(1);
				do
				{
					TraceMpdStatus();
					wFooter->ReadKey(item);
				}
				while (item != 'a' && item != 'y' && item != 'g' && item != 'c' && item != 'p');
				curs_set(0);
				UnlockStatusbar();
				mpd_TagItems new_tagitem = IntoTagItem(item);
				if (new_tagitem != Config.media_lib_primary_tag)
				{
					Config.media_lib_primary_tag = new_tagitem;
					string item_type = IntoStr(Config.media_lib_primary_tag);
					myLibrary->Artists->SetTitle(item_type + "s");
					myLibrary->Artists->Reset();
					myLibrary->Artists->Clear(0);
					myLibrary->Artists->Display();
					ToLower(item_type);
					ShowMessage("Switched to list of %s tag", item_type.c_str());
				}
			}
		}
		else if (Keypressed(input, Key.SongInfo))
		{
			myInfo->GetSong();
		}
#		ifdef HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ArtistInfo))
		{
			myInfo->GetArtist();
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
			if (myScreen == myPlaylist)
				myBrowser->SwitchTo();
			else
				myPlaylist->SwitchTo();
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
#		ifdef ENABLE_CLOCK
		else if (Keypressed(input, Key.Clock))
		{
			myClock->SwitchTo();
		}
#		endif // ENABLE_CLOCK
		// key mapping end
	}
	// restore old cerr buffer
	std::cerr.rdbuf(cerr_buffer);
	errorlog.close();
	Mpd->Disconnect();
	DestroyScreen();
	return 0;
}

