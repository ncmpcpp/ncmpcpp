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
#include "status_checker.h"
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

ncmpcpp_config Global::Config;
ncmpcpp_keys Global::Key;

Window *Global::wCurrent;
Window *Global::wPrev;

Window *Global::wHeader;
Window *Global::wFooter;

Connection *Global::Mpd;

int Global::now_playing = -1;
int Global::lock_statusbar_delay = -1;

size_t Global::browsed_dir_scroll_begin = 0;
size_t Global::main_start_y;
size_t Global::main_height;
size_t Global::lyrics_scroll_begin = 0;

time_t Global::timer;

string Global::browsed_dir = "/";
string Global::editor_browsed_dir = "/";
string Global::editor_highlighted_dir;
string Global::info_title;

NcmpcppScreen Global::current_screen;
NcmpcppScreen Global::prev_screen;

bool Global::dont_change_now_playing = 0;
bool Global::block_progressbar_update = 0;
bool Global::block_playlist_update = 0;
bool Global::block_item_list_update = 0;

bool Global::messages_allowed = 0;
bool Global::redraw_header = 1;
bool Global::reload_lyrics = 0;

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
	
	Playlist::Init();
	Browser::Init();
	SearchEngine::Init();
	MediaLibrary::Init();
	PlaylistEditor::Init();
	
#	ifdef HAVE_TAGLIB_H
	TinyTagEditor::Init();
	TagEditor::Init();
#	endif // HAVE_TAGLIB_H
	
#	ifdef ENABLE_CLOCK
	Clock::Init();
#	endif // ENABLE_CLOCK
	
	Help::Init();
	Info::Init();
	Lyrics::Init();
	
	if (Config.header_visibility)
	{
		wHeader = new Window(0, 0, COLS, 1, "", Config.header_color, brNone);
		wHeader->Display();
	}
	
	size_t footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
	size_t footer_height = Config.statusbar_visibility ? 2 : 1;
	
	wFooter = new Window(0, footer_start_y, COLS, footer_height, "", Config.statusbar_color, brNone);
	wFooter->SetTimeout(ncmpcpp_window_timeout);
	wFooter->SetGetStringHelper(TraceMpdStatus);
	wFooter->Display();
	
	wCurrent = mPlaylist;
	current_screen = csPlaylist;
	
	timer = time(NULL);
	
	Mpd->SetStatusUpdater(NcmpcppStatusChanged, NULL);
	Mpd->SetErrorHandler(NcmpcppErrorCallback, NULL);
	
	// local variables
	int input;
	
	Song edited_song;
	
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
		block_playlist_update = 0;
		messages_allowed = 1;
		
		// header stuff
		gettimeofday(&past, 0);
		const size_t max_allowed_title_length = wHeader ? wHeader->GetWidth()-volume_state.length()-screen_title.length() : 0;
		if (((past.tv_sec == now.tv_sec && past.tv_usec >= now.tv_usec+500000)
		||    past.tv_sec > now.tv_sec)
		&&  ((current_screen == csBrowser && browsed_dir.length() > max_allowed_title_length)
		||    current_screen == csLyrics))
		{
			redraw_header = 1;
			gettimeofday(&now, 0);
		}
		if (Config.header_visibility && redraw_header)
		{
			switch (current_screen)
			{
				case csHelp:
					screen_title = "Help";
					break;
				case csPlaylist:
					screen_title = "Playlist ";
					break;
				case csBrowser:
					screen_title = "Browse: ";
					break;
#				ifdef HAVE_TAGLIB_H
				case csTinyTagEditor:
				case csTagEditor:
					screen_title = "Tag editor";
					break;
#				endif // HAVE_TAGLIB_H
				case csInfo:
					screen_title = info_title;
					break;
				case csSearcher:
					screen_title = "Search engine";
					break;
				case csLibrary:
					screen_title = "Media library";
					break;
				case csLyrics:
					screen_title = "Lyrics: ";
					break;
				case csPlaylistEditor:
					screen_title = "Playlist editor";
					break;
#				ifdef ENABLE_CLOCK
				case csClock:
					screen_title = "Clock";
					break;
#				endif // ENABLE_CLOCK
				default:
					break;
			}
			
			if (title_allowed)
			{
				wHeader->Bold(1);
				wHeader->WriteXY(0, 0, 1, "%s", screen_title.c_str());
				wHeader->Bold(0);
				
				if (current_screen == csPlaylist)
				{
					Display::TotalPlaylistLength(*wHeader);
				}
				else if (current_screen == csBrowser)
				{
					wHeader->Bold(1);
					Scroller(*wHeader, browsed_dir, max_allowed_title_length, browsed_dir_scroll_begin);
					wHeader->Bold(0);
				}
				else if (current_screen == csLyrics)
				{
					
					wHeader->Bold(1);
					Scroller(*wHeader, lyrics_song.toString("%a - %t"), max_allowed_title_length, lyrics_scroll_begin);
					wHeader->Bold(0);
				}
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
			wHeader->WriteXY(wHeader->GetWidth()-volume_state.length(), 0, 0, "%s", volume_state.c_str());
			wHeader->SetColor(Config.header_color);
			wHeader->Refresh();
			redraw_header = 0;
		}
		// header stuff end
		
		if (current_screen == csHelp
		||  current_screen == csPlaylist
		||  current_screen == csBrowser);
		else if (current_screen == csLibrary)
		{
			MediaLibrary::Update();
		}
		else if (current_screen == csPlaylistEditor)
		{
			PlaylistEditor::Update();
		}
		else
#		ifdef HAVE_TAGLIB_H
		if (current_screen == csTagEditor)
		{
			TagEditor::Update();
		}
		else
#		endif // HAVE_TAGLIB_H
#		ifdef ENABLE_CLOCK
		if (current_screen == csClock)
		{
			Clock::Update();
		}
		else
#		endif
		if (current_screen == csLyrics)
		{
			Lyrics::Update();
		}
#		ifdef HAVE_CURL_CURL_H
		if (!Info::Ready())
			Lyrics::Ready();
#		endif
		
		wCurrent->Display();
//		redraw_screen = 0;
		
		wCurrent->ReadKey(input);
		if (input == ERR)
			continue;
		
		if (!title_allowed)
			redraw_header = 1;
		title_allowed = 1;
		timer = time(NULL);
		
		switch (current_screen)
		{
			case csPlaylist:
				mPlaylist->Highlighting(1);
				break;
			case csLibrary:
			case csPlaylistEditor:
#			ifdef HAVE_TAGLIB_H
			case csTagEditor:
#			endif // HAVE_TAGLIB_H
			{
				if (Keypressed(input, Key.Up) || Keypressed(input, Key.Down) || Keypressed(input, Key.PageUp) || Keypressed(input, Key.PageDown) || Keypressed(input, Key.Home) || Keypressed(input, Key.End) || Keypressed(input, Key.FindForward) || Keypressed(input, Key.FindBackward) || Keypressed(input, Key.NextFoundPosition) || Keypressed(input, Key.PrevFoundPosition))
				{
					if (wCurrent == mLibArtists)
					{
						mLibAlbums->Clear(0);
						mLibSongs->Clear(0);
					}
					else if (wCurrent == mLibAlbums)
					{
						mLibSongs->Clear(0);
					}
					else if (wCurrent == mPlaylistList)
					{
						mPlaylistEditor->Clear(0);
					}
#					ifdef HAVE_TAGLIB_H
					else if (wCurrent == mEditorLeftCol)
					{
						mEditorTags->Clear(0);
						mEditorTagTypes->Refresh();
					}
//					else if (wCurrent == mEditorTagTypes)
//						redraw_screen = 1;
#					endif // HAVE_TAGLIB_H
				}
			}
			default:
				break;
		}
		
		// key mapping beginning
		
		if (
		    Keypressed(input, Key.Up)
#ifdef		ENABLE_CLOCK
		&&  current_screen != csClock
#		endif // ENABLE_CLOCK
		   )
		{
			if (!Config.fancy_scrolling
			&&  (wCurrent == mLibArtists
			||   wCurrent == mPlaylistList
#			ifdef HAVE_TAGLIB_H
			||   wCurrent == mEditorLeftCol
#			endif // HAVE_TAGLIB_H
			    )
			   )
			{
				wCurrent->SetTimeout(50);
				while (Keypressed(input, Key.Up))
				{
					wCurrent->Scroll(wUp);
					wCurrent->Refresh();
					wCurrent->ReadKey(input);
				}
				wCurrent->SetTimeout(ncmpcpp_window_timeout);
			}
			else
				wCurrent->Scroll(wUp);
		}
		else if (
		   Keypressed(input, Key.Down)
#ifdef		ENABLE_CLOCK
		&& current_screen != csClock
#		endif // ENABLE_CLOCK
			)
		{
			if (!Config.fancy_scrolling
			&&  (wCurrent == mLibArtists
			||   wCurrent == mPlaylistList
#			ifdef HAVE_TAGLIB_H
			|| wCurrent == mEditorLeftCol
#			endif // HAVE_TAGLIB_H
			    )
			   )
			{
				wCurrent->SetTimeout(50);
				while (Keypressed(input, Key.Down))
				{
					wCurrent->Scroll(wDown);
					wCurrent->Refresh();
					wCurrent->ReadKey(input);
				}
				wCurrent->SetTimeout(ncmpcpp_window_timeout);
			}
			else
				wCurrent->Scroll(wDown);
		}
		else if (
		   Keypressed(input, Key.PageUp)
#ifdef		ENABLE_CLOCK
		&& current_screen != csClock
#		endif // ENABLE_CLOCK
			)
		{
			wCurrent->Scroll(wPageUp);
		}
		else if (
		   Keypressed(input, Key.PageDown)
#ifdef		ENABLE_CLOCK
		&& current_screen != csClock
#		endif // ENABLE_CLOCK
			)
		{
			wCurrent->Scroll(wPageDown);
		}
		else if (Keypressed(input, Key.Home))
		{
			wCurrent->Scroll(wHome);
		}
		else if (Keypressed(input, Key.End))
		{
			wCurrent->Scroll(wEnd);
		}
		else if (input == KEY_RESIZE)
		{
//			redraw_screen = 1;
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
			
			Help::Resize();
			Playlist::Resize();
			Browser::Resize();
			SearchEngine::Resize();
			MediaLibrary::Resize();
			PlaylistEditor::Resize();
			Info::Resize();
			Lyrics::Resize();
			
#			ifdef HAVE_TAGLIB_H
			mTagEditor->Resize(COLS, main_height);
			TagEditor::Resize();
#			endif // HAVE_TAGLIB_H
			
#			ifdef ENABLE_CLOCK
			Clock::Resize();
#			endif // ENABLE_CLOCK
			
			if (Config.header_visibility)
				wHeader->Resize(COLS, wHeader->GetHeight());
			
			footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
			wFooter->MoveTo(0, footer_start_y);
			wFooter->Resize(COLS, wFooter->GetHeight());
			
#			ifdef HAVE_TAGLIB_H
			if (current_screen == csLibrary)
			{
				MediaLibrary::Refresh();
			}
			else if (current_screen == csTagEditor)
			{
				TagEditor::Refresh();
			}
			else
#			endif // HAVE_TAGLIB_H
			if (current_screen == csPlaylistEditor)
			{
				PlaylistEditor::Refresh();
			}
			header_update_status = 1;
			PlayerState mpd_state = Mpd->GetState();
			StatusChanges changes;
			if (mpd_state == psPlay || mpd_state == psPause)
				changes.ElapsedTime = 1; // restore status
			else
				changes.PlayerState = 1;
			
			NcmpcppStatusChanged(Mpd, changes, NULL);
		}
		else if (Keypressed(input, Key.GoToParentDir))
		{
			if (wCurrent == mBrowser && browsed_dir != "/")
			{
				mBrowser->Reset();
				Browser::EnterPressed();
			}
		}
		else if (Keypressed(input, Key.Enter))
		{
			switch (current_screen)
			{
				case csPlaylist:
				{
					if (!mPlaylist->Empty())
						Mpd->PlayID(mPlaylist->Current().GetID());
					break;
				}
				case csBrowser:
				{
					Browser::EnterPressed();
					break;
				}
#				ifdef HAVE_TAGLIB_H
				case csTinyTagEditor:
				{
					TinyTagEditor::EnterPressed(edited_song);
					break;
				}
#				endif // HAVE_TAGLIB_H
				case csSearcher:
				{
					SearchEngine::EnterPressed();
					break;
				}
				case csLibrary:
				{
					MediaLibrary::EnterPressed();
					break;
				}
				case csPlaylistEditor:
				{
					PlaylistEditor::EnterPressed();
					break;
				}
#				ifdef HAVE_TAGLIB_H
				case csTagEditor:
				{
					TagEditor::EnterPressed();
					break;
				}
#				endif // HAVE_TAGLIB_H
				default:
					break;
			}
		}
		else if (Keypressed(input, Key.Space))
		{
			if (Config.space_selects
			||  wCurrent == mPlaylist
#			ifdef HAVE_TAGLIB_H
			||  wCurrent == mEditorTags
#			endif // HAVE_TAGLIB_H
			   )
			{
				if (wCurrent == mPlaylist
#				ifdef HAVE_TAGLIB_H
				||  wCurrent == mEditorTags
#				endif // HAVE_TAGLIB_H
				|| (wCurrent == mBrowser && ((Menu<Song> *)wCurrent)->Choice() >= (browsed_dir != "/" ? 1 : 0)) || (wCurrent == mSearcher && !mSearcher->Current().first)
				|| wCurrent == mLibSongs
				|| wCurrent == mPlaylistEditor)
				{
					List *mList = (Menu<Song> *)wCurrent;
					if (mList->Empty())
						continue;
					size_t i = mList->Choice();
					mList->Select(i, !mList->isSelected(i));
					wCurrent->Scroll(wDown);
				}
			}
			else
			{
				if (current_screen == csBrowser)
				{
					Browser::SpacePressed();
				}
				else if (current_screen == csSearcher)
				{
					SearchEngine::SpacePressed();
				}
				else if (current_screen == csLibrary)
				{
					MediaLibrary::SpacePressed();
				}
				else if (current_screen == csPlaylistEditor)
				{
					PlaylistEditor::SpacePressed();
				}
#				ifdef HAVE_TAGLIB_H
				else if (wCurrent == mEditorLeftCol)
				{
					Config.albums_in_tag_editor = !Config.albums_in_tag_editor;
					wCurrent = wTagEditorActiveCol = mEditorLeftCol = Config.albums_in_tag_editor ? mEditorAlbums : mEditorDirs;
					ShowMessage("Switched to %s view", Config.albums_in_tag_editor ? "albums" : "directories");
					mEditorLeftCol->Display();
					mEditorTags->Clear(0);
//					redraw_screen = 1;
				}
#				endif // HAVE_TAGLIB_H
				else if (current_screen == csLyrics)
				{
					Config.now_playing_lyrics = !Config.now_playing_lyrics;
					ShowMessage("Reload lyrics if song changes: %s", Config.now_playing_lyrics ? "On" : "Off");
				}
			}
		}
		else if (Keypressed(input, Key.VolumeUp))
		{
			if (current_screen == csLibrary && input == Key.VolumeUp[0])
			{
				CLEAR_FIND_HISTORY;
				if (wCurrent == mLibArtists)
				{
					if (mLibSongs->Empty())
						continue;
					mLibArtists->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wLibActiveCol = mLibAlbums;
					mLibAlbums->HighlightColor(Config.active_column_color);
					if (!mLibAlbums->Empty())
						continue;
				}
				if (wCurrent == mLibAlbums && !mLibSongs->Empty())
				{
					mLibAlbums->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wLibActiveCol = mLibSongs;
					mLibSongs->HighlightColor(Config.active_column_color);
				}
			}
			else if (current_screen == csPlaylistEditor && input == Key.VolumeUp[0])
			{
				if (wCurrent == mPlaylistList)
				{
					CLEAR_FIND_HISTORY;
					mPlaylistList->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wPlaylistEditorActiveCol = mPlaylistEditor;
					mPlaylistEditor->HighlightColor(Config.active_column_color);
				}
			}
#			ifdef HAVE_TAGLIB_H
			else if (current_screen == csTagEditor && input == Key.VolumeUp[0])
			{
				CLEAR_FIND_HISTORY;
				if (wCurrent == mEditorLeftCol)
				{
					mEditorLeftCol->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wTagEditorActiveCol = mEditorTagTypes;
					mEditorTagTypes->HighlightColor(Config.active_column_color);
				}
				else if (wCurrent == mEditorTagTypes && mEditorTagTypes->Choice() < 12 && !mEditorTags->Empty())
				{
					mEditorTagTypes->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wTagEditorActiveCol = mEditorTags;
					mEditorTags->HighlightColor(Config.active_column_color);
				}
			}
#			endif // HAVE_TAGLIB_H
			else
				Mpd->SetVolume(Mpd->GetVolume()+1);
		}
		else if (Keypressed(input, Key.VolumeDown))
		{
			if (current_screen == csLibrary && input == Key.VolumeDown[0])
			{
				CLEAR_FIND_HISTORY;
				if (wCurrent == mLibSongs)
				{
					mLibSongs->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wLibActiveCol = mLibAlbums;
					mLibAlbums->HighlightColor(Config.active_column_color);
					if (!mLibAlbums->Empty())
						continue;
				}
				if (wCurrent == mLibAlbums)
				{
					mLibAlbums->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wLibActiveCol = mLibArtists;
					mLibArtists->HighlightColor(Config.active_column_color);
				}
			}
			else if (current_screen == csPlaylistEditor && input == Key.VolumeDown[0])
			{
				if (wCurrent == mPlaylistEditor)
				{
					CLEAR_FIND_HISTORY;
					mPlaylistEditor->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wPlaylistEditorActiveCol = mPlaylistList;
					mPlaylistList->HighlightColor(Config.active_column_color);
				}
			}
#			ifdef HAVE_TAGLIB_H
			else if (current_screen == csTagEditor && input == Key.VolumeDown[0])
			{
				CLEAR_FIND_HISTORY;
				if (wCurrent == mEditorTags)
				{
					mEditorTags->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wTagEditorActiveCol = mEditorTagTypes;
					mEditorTagTypes->HighlightColor(Config.active_column_color);
				}
				else if (wCurrent == mEditorTagTypes)
				{
					mEditorTagTypes->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = wTagEditorActiveCol = mEditorLeftCol;
					mEditorLeftCol->HighlightColor(Config.active_column_color);
				}
			}
#			endif // HAVE_TAGLIB_H
			else
				Mpd->SetVolume(Mpd->GetVolume()-1);
		}
		else if (Keypressed(input, Key.Delete))
		{
			if (!mPlaylist->Empty() && current_screen == csPlaylist)
			{
				block_playlist_update = 1;
				if (mPlaylist->hasSelected())
				{
					vector<size_t> list;
					mPlaylist->GetSelected(list);
					for (vector<size_t>::const_reverse_iterator it = list.rbegin(); it != ((const vector<size_t> &)list).rend(); it++)
					{
						Mpd->QueueDeleteSong(*it);
						mPlaylist->DeleteOption(*it);
					}
					ShowMessage("Selected items deleted!");
//					redraw_screen = 1;
				}
				else
				{
					dont_change_now_playing = 1;
					mPlaylist->SetTimeout(50);
					while (!mPlaylist->Empty() && Keypressed(input, Key.Delete))
					{
						size_t id = mPlaylist->Choice();
						TraceMpdStatus();
						timer = time(NULL);
						if (size_t(now_playing) > id)  // needed for keeping proper
							now_playing--; // position of now playing song.
						Mpd->QueueDeleteSong(id);
						mPlaylist->DeleteOption(id);
						mPlaylist->Refresh();
						mPlaylist->ReadKey(input);
					}
					mPlaylist->SetTimeout(ncmpcpp_window_timeout);
					dont_change_now_playing = 0;
				}
				Mpd->CommitQueue();
			}
			else if (current_screen == csBrowser || wCurrent == mPlaylistList)
			{
				LockStatusbar();
				string name = wCurrent == mBrowser ? mBrowser->Current().name : mPlaylistList->Current();
				if (current_screen != csBrowser || mBrowser->Current().type == itPlaylist)
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
							GetDirectory("/");
					}
					else
						ShowMessage("Aborted!");
					curs_set(0);
					mPlaylistList->Clear(0); // make playlists list update itself
				}
				UnlockStatusbar();
			}
			else if (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			{
				if (mPlaylistEditor->hasSelected())
				{
					vector<size_t> list;
					mPlaylistEditor->GetSelected(list);
					locale_to_utf(mPlaylistList->Current());
					for (vector<size_t>::const_reverse_iterator it = list.rbegin(); it != ((const vector<size_t> &)list).rend(); it++)
					{
						Mpd->QueueDeleteFromPlaylist(mPlaylistList->Current(), *it);
						mPlaylistEditor->DeleteOption(*it);
					}
					utf_to_locale(mPlaylistList->Current());
					ShowMessage("Selected items deleted from playlist '%s'!", mPlaylistList->Current().c_str());
//					redraw_screen = 1;
				}
				else
				{
					mPlaylistEditor->SetTimeout(50);
					locale_to_utf(mPlaylistList->Current());
					while (!mPlaylistEditor->Empty() && Keypressed(input, Key.Delete))
					{
						TraceMpdStatus();
						timer = time(NULL);
						Mpd->QueueDeleteFromPlaylist(mPlaylistList->Current(), mPlaylistEditor->Choice());
						mPlaylistEditor->DeleteOption(mPlaylistEditor->Choice());
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					utf_to_locale(mPlaylistList->Current());
					mPlaylistEditor->SetTimeout(ncmpcpp_window_timeout);
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
					mPlaylistList->Clear(0); // make playlist's list update itself
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
					mPlaylistList->Clear(0); // make playlist's list update itself
					UnlockStatusbar();
				}
			}
			if (!Config.local_browser && browsed_dir == "/" && !mBrowser->Empty())
				GetDirectory(browsed_dir);
		}
		else if (Keypressed(input, Key.Stop))
		{
			Mpd->Stop();
		}
		else if (Keypressed(input, Key.MvSongUp))
		{
			if (current_screen == csPlaylist && !mPlaylist->Empty())
			{
				block_playlist_update = 1;
				mPlaylist->SetTimeout(50);
				if (mPlaylist->hasSelected())
				{
					vector<size_t> list;
					mPlaylist->GetSelected(list);
					
					for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						if (*it == size_t(now_playing) && list.front() > 0)
							mPlaylist->BoldOption(now_playing, 0);
					
					vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongUp) && list.front() > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						{
							(*it)--;
							mPlaylist->Swap(*it, (*it)+1);
						}
						mPlaylist->Highlight(list[(list.size()-1)/2]);
						mPlaylist->Refresh();
						mPlaylist->ReadKey(input);
					}
					for (size_t i = 0; i < list.size(); i++)
						Mpd->QueueMove(origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					size_t from, to;
					from = to = mPlaylist->Choice();
					// unbold now playing as if song changes during move, this won't be unbolded.
					if (to == size_t(now_playing) && to > 0)
						mPlaylist->BoldOption(now_playing, 0);
					while (Keypressed(input, Key.MvSongUp) && to > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						to--;
						mPlaylist->Swap(to, to+1);
						mPlaylist->Scroll(wUp);
						mPlaylist->Refresh();
						mPlaylist->ReadKey(input);
					}
					Mpd->Move(from, to);
				}
				mPlaylist->SetTimeout(ncmpcpp_window_timeout);
			}
			else if (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			{
				mPlaylistEditor->SetTimeout(50);
				if (mPlaylistEditor->hasSelected())
				{
					vector<size_t> list;
					mPlaylistEditor->GetSelected(list);
					
					vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongUp) && list.front() > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						{
							(*it)--;
							mPlaylistEditor->Swap(*it, (*it)+1);
						}
						mPlaylistEditor->Highlight(list[(list.size()-1)/2]);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					for (size_t i = 0; i < list.size(); i++)
						if (origs[i] != list[i])
							Mpd->QueueMove(mPlaylistList->Current(), origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					size_t from, to;
					from = to = mPlaylistEditor->Choice();
					while (Keypressed(input, Key.MvSongUp) && to > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						to--;
						mPlaylistEditor->Swap(to, to+1);
						mPlaylistEditor->Scroll(wUp);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					if (from != to)
						Mpd->Move(mPlaylistList->Current(), from, to);
				}
				mPlaylistEditor->SetTimeout(ncmpcpp_window_timeout);
			}
		}
		else if (Keypressed(input, Key.MvSongDown))
		{
			if (current_screen == csPlaylist && !mPlaylist->Empty())
			{
				block_playlist_update = 1;
				mPlaylist->SetTimeout(50);
				if (mPlaylist->hasSelected())
				{
					vector<size_t> list;
					mPlaylist->GetSelected(list);
					
					for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						if (*it == size_t(now_playing) && list.back() < mPlaylist->Size()-1)
							mPlaylist->BoldOption(now_playing, 0);
					
					vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongDown) && list.back() < mPlaylist->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); it++)
						{
							(*it)++;
							mPlaylist->Swap(*it, (*it)-1);
						}
						mPlaylist->Highlight(list[(list.size()-1)/2]);
						mPlaylist->Refresh();
						mPlaylist->ReadKey(input);
					}
					for (int i = list.size()-1; i >= 0; i--)
						Mpd->QueueMove(origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					size_t from, to;
					from = to = mPlaylist->Choice();
					// unbold now playing as if song changes during move, this won't be unbolded.
					if (to == size_t(now_playing) && to < mPlaylist->Size()-1)
						mPlaylist->BoldOption(now_playing, 0);
					while (Keypressed(input, Key.MvSongDown) && to < mPlaylist->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						to++;
						mPlaylist->Swap(to, to-1);
						mPlaylist->Scroll(wDown);
						mPlaylist->Refresh();
						mPlaylist->ReadKey(input);
					}
					Mpd->Move(from, to);
				}
				mPlaylist->SetTimeout(ncmpcpp_window_timeout);
				
			}
			else if (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			{
				mPlaylistEditor->SetTimeout(50);
				if (mPlaylistEditor->hasSelected())
				{
					vector<size_t> list;
					mPlaylistEditor->GetSelected(list);
					
					vector<size_t> origs(list);
					
					while (Keypressed(input, Key.MvSongDown) && list.back() < mPlaylistEditor->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<size_t>::reverse_iterator it = list.rbegin(); it != list.rend(); it++)
						{
							(*it)++;
							mPlaylistEditor->Swap(*it, (*it)-1);
						}
						mPlaylistEditor->Highlight(list[(list.size()-1)/2]);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					for (int i = list.size()-1; i >= 0; i--)
						if (origs[i] != list[i])
							Mpd->QueueMove(mPlaylistList->Current(), origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					size_t from, to;
					from = to = mPlaylistEditor->Choice();
					while (Keypressed(input, Key.MvSongDown) && to < mPlaylistEditor->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						to++;
						mPlaylistEditor->Swap(to, to-1);
						mPlaylistEditor->Scroll(wDown);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					if (from != to)
						Mpd->Move(mPlaylistList->Current(), from, to);
				}
				mPlaylistEditor->SetTimeout(ncmpcpp_window_timeout);
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
						Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
						if (s.GetHash() != list[0]->GetHash())
							ShowMessage("%s", message_part_of_songs_added);
					}
				}
				else
					Mpd->AddSong(path);
				FreeSongList(list);
			}
		}
		else if (Keypressed(input, Key.SeekForward) || Keypressed(input, Key.SeekBackward))
		{
			if (now_playing < 0)
				continue;
			if (!mPlaylist->at(now_playing).GetTotalLength())
			{
				ShowMessage("Unknown item length!");
				continue;
			}
			block_progressbar_update = 1;
			LockStatusbar();
			
			int songpos;
			time_t t = time(NULL);
			
			songpos = Mpd->GetElapsedTime();
			Song &s = mPlaylist->at(now_playing);
			
			while (Keypressed(input, Key.SeekForward) || Keypressed(input, Key.SeekBackward))
			{
				TraceMpdStatus();
				timer = time(NULL);
				mPlaylist->ReadKey(input);
				
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
				wFooter->WriteXY(wFooter->GetWidth()-tracklength.length(), 1, 0, "%s", tracklength.c_str());
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
			if (wCurrent == mPlaylist)
			{
				Config.columns_in_playlist = !Config.columns_in_playlist;
				ShowMessage("Playlist display mode: %s", Config.columns_in_playlist ? "Columns" : "Classic");
				mPlaylist->SetItemDisplayer(Config.columns_in_playlist ? Display::SongsInColumns : Display::Songs);
				mPlaylist->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
				mPlaylist->SetTitle(Config.columns_in_playlist ? Display::Columns(Config.song_columns_list_format) : "");
			}
			else if (wCurrent == mBrowser)
			{
				Config.columns_in_browser = !Config.columns_in_browser;
				ShowMessage("Browser display mode: %s", Config.columns_in_browser ? "Columns" : "Classic");
				mBrowser->SetTitle(Config.columns_in_browser ? Display::Columns(Config.song_columns_list_format) : "");
			}
			else if (wCurrent == mSearcher)
			{
				Config.columns_in_search_engine = !Config.columns_in_search_engine;
				ShowMessage("Search engine display mode: %s", Config.columns_in_search_engine ? "Columns" : "Classic");
				if (mSearcher->Size() > search_engine_static_options)
					mSearcher->SetTitle(Config.columns_in_search_engine ? Display::Columns(Config.song_columns_list_format) : "");
			}
//			redraw_screen = 1;
		}
#		ifdef HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ToggleLyricsDB))
		{
			const char *current_lyrics_plugin = GetLyricsPluginName(++Config.lyrics_db);
			if (!current_lyrics_plugin)
			{
				current_lyrics_plugin = GetLyricsPluginName(Config.lyrics_db = 0);
			}
			ShowMessage("Using lyrics database: %s", current_lyrics_plugin);
		}
#		endif // HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ToggleAutoCenter))
		{
			Config.autocenter_mode = !Config.autocenter_mode;
			ShowMessage("Auto center mode: %s", Config.autocenter_mode ? "On" : "Off");
			if (Config.autocenter_mode && now_playing >= 0)
				mPlaylist->Highlight(now_playing);
		}
		else if (Keypressed(input, Key.UpdateDB))
		{
			if (current_screen == csBrowser)
				Mpd->UpdateDirectory(browsed_dir);
#			ifdef HAVE_TAGLIB_H
			else if (current_screen == csTagEditor && !Config.albums_in_tag_editor)
				Mpd->UpdateDirectory(editor_browsed_dir);
#			endif // HAVE_TAGLIB_H
			else
				Mpd->UpdateDirectory("/");
		}
		else if (Keypressed(input, Key.GoToNowPlaying))
		{
			if (current_screen == csPlaylist && now_playing >= 0)
				mPlaylist->Highlight(now_playing);
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
			if (wCurrent == mLibArtists)
			{
				LockStatusbar();
				Statusbar() << fmtBold << IntoStr(Config.media_lib_primary_tag) << fmtBoldEnd << ": ";
				string new_tag = wFooter->GetString(mLibArtists->Current());
				UnlockStatusbar();
				if (!new_tag.empty() && new_tag != mLibArtists->Current())
				{
					bool success = 1;
					SongList list;
					ShowMessage("Updating tags...");
					Mpd->StartSearch(1);
					Mpd->AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(mLibArtists->Current()));
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
						if (!WriteTags(**it))
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
			else if (wCurrent == mLibAlbums)
			{
				LockStatusbar();
				Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
				string new_album = wFooter->GetString(mLibAlbums->Current().second);
				UnlockStatusbar();
				if (!new_album.empty() && new_album != mLibAlbums->Current().second)
				{
					bool success = 1;
					ShowMessage("Updating tags...");
					for (size_t i = 0;  i < mLibSongs->Size(); i++)
					{
						(*mLibSongs)[i].Localize();
						ShowMessage("Updating tags in '%s'...", (*mLibSongs)[i].GetName().c_str());
						string path = Config.mpd_music_dir + (*mLibSongs)[i].GetFile();
						TagLib::FileRef f(path.c_str());
						if (f.isNull())
						{
							ShowMessage("Error opening file '%s'!", (*mLibSongs)[i].GetFile().c_str());
							success = 0;
							break;
						}
						f.tag()->setAlbum(ToWString(new_album));
						if (!f.save())
						{
							ShowMessage("Error writing tags in '%s'!", (*mLibSongs)[i].GetFile().c_str());
							success = 0;
							break;
						}
					}
					if (success)
					{
						Mpd->UpdateDirectory(FindSharedDir(mLibSongs));
						ShowMessage("Tags updated succesfully!");
					}
				}
			}
			else if (
			    (wCurrent == mPlaylist && !mPlaylist->Empty())
			||  (wCurrent == mBrowser && mBrowser->Current().type == itSong)
			||  (wCurrent == mSearcher && !mSearcher->Current().first)
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			||  (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
				size_t id = mList->Choice();
				switch (current_screen)
				{
					case csPlaylist:
						edited_song = mPlaylist->at(id);
						break;
					case csBrowser:
						edited_song = *mBrowser->at(id).song;
						break;
					case csSearcher:
						edited_song = *mSearcher->at(id).second;
						break;
					case csLibrary:
						edited_song = mLibSongs->at(id);
						break;
					case csPlaylistEditor:
						edited_song = mPlaylistEditor->at(id);
						break;
					case csTagEditor:
						edited_song = mEditorTags->at(id);
						break;
					default:
						break;
				}
				if (edited_song.IsStream())
				{
					ShowMessage("Cannot edit streams!");
				}
				else if (GetSongTags(edited_song))
				{
					wPrev = wCurrent;
					wCurrent = mTagEditor;
					prev_screen = current_screen;
					current_screen = csTinyTagEditor;
					redraw_header = 1;
				}
				else
				{
					string message = "Cannot read file '";
					if (edited_song.IsFromDB())
						message += Config.mpd_music_dir;
					message += edited_song.GetFile();
					message += "'!";
					ShowMessage("%s", message.c_str());
				}
			}
			else if (wCurrent == mEditorDirs)
			{
				string old_dir = mEditorDirs->Current().first;
				LockStatusbar();
				Statusbar() << fmtBold << "Directory: " << fmtBoldEnd;
				string new_dir = wFooter->GetString(old_dir);
				UnlockStatusbar();
				if (!new_dir.empty() && new_dir != old_dir)
				{
					string full_old_dir = Config.mpd_music_dir + editor_browsed_dir + "/" + locale_to_utf_cpy(old_dir);
					string full_new_dir = Config.mpd_music_dir + editor_browsed_dir + "/" + locale_to_utf_cpy(new_dir);
					if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
					{
						Mpd->UpdateDirectory(editor_browsed_dir);
					}
					else
					{
						ShowMessage("Cannot rename '%s' to '%s'!", old_dir.c_str(), new_dir.c_str());
					}
				}
			}
			else
#			endif // HAVE_TAGLIB_H
			if (wCurrent == mBrowser && mBrowser->Current().type == itDirectory)
			{
				string old_dir = mBrowser->Current().name;
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
						GetDirectory(browsed_dir);
					}
					else
						ShowMessage("Cannot rename '%s' to '%s'!", old_dir.c_str(), new_dir.c_str());
				}
			}
			else if (wCurrent == mPlaylistList || (wCurrent == mBrowser && mBrowser->Current().type == itPlaylist))
			{
				string old_name = wCurrent == mPlaylistList ? mPlaylistList->Current() : mBrowser->Current().name;
				LockStatusbar();
				Statusbar() << fmtBold << "Playlist: " << fmtBoldEnd;
				string new_name = wFooter->GetString(old_name);
				UnlockStatusbar();
				if (!new_name.empty() && new_name != old_name)
				{
					Mpd->Rename(locale_to_utf_cpy(old_name), locale_to_utf_cpy(new_name));
					ShowMessage("Playlist '%s' renamed to '%s'", old_name.c_str(), new_name.c_str());
					if (!Config.local_browser)
						GetDirectory("/");
					mPlaylistList->Clear(0);
				}
			}
		}
		else if (Keypressed(input, Key.GoToContainingDir))
		{
			if ((wCurrent == mPlaylist && !mPlaylist->Empty())
			||  (wCurrent == mSearcher && !mSearcher->Current().first)
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
#			ifdef HAVE_TAGLIB_H
			||  (wCurrent == mEditorTags && !mEditorTags->Empty())
#			endif // HAVE_TAGLIB_H
			   )
			{
				size_t id = ((Menu<Song> *)wCurrent)->Choice();
				Song *s = 0;
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csSearcher:
						s = mSearcher->at(id).second;
						break;
					case csLibrary:
						s = &mLibSongs->at(id);
						break;
					case csPlaylistEditor:
						s = &mPlaylistEditor->at(id);
						break;
#					ifdef HAVE_TAGLIB_H
					case csTagEditor:
						s = &mEditorTags->at(id);
						break;
#					endif // HAVE_TAGLIB_H
					default:
						break;
				}
				
				if (s->GetDirectory().empty()) // for streams
					continue;
				
				Config.local_browser = !s->IsFromDB();
				
				string option = s->toString(Config.song_status_format);
				locale_to_utf(option);
				GetDirectory(s->GetDirectory());
				for (size_t i = 0; i < mBrowser->Size(); i++)
				{
					if (mBrowser->at(i).type == itSong && option == mBrowser->at(i).song->toString(Config.song_status_format))
					{
						mBrowser->Highlight(i);
						break;
					}
				}
				Browser::SwitchTo();
			}
		}
		else if (Keypressed(input, Key.StartSearching))
		{
			if (wCurrent == mSearcher)
			{
				mSearcher->Highlight(search_engine_search_button);
				mSearcher->Highlighting(0);
				mSearcher->Refresh();
				mSearcher->Highlighting(1);
				SearchEngine::EnterPressed();
			}
		}
		else if (Keypressed(input, Key.GoToPosition))
		{
			if (now_playing < 0)
				continue;
			if (!mPlaylist->at(now_playing).GetTotalLength())
			{
				ShowMessage("Unknown item length!");
				continue;
			}
			LockStatusbar();
			Statusbar() << "Position to go (in %): ";
			string position = wFooter->GetString(3);
			int newpos = StrToInt(position);
			if (newpos > 0 && newpos < 100 && !position.empty())
				Mpd->Seek(mPlaylist->at(now_playing).GetTotalLength()*newpos/100.0);
			UnlockStatusbar();
		}
		else if (Keypressed(input, Key.ReverseSelection))
		{
			if (wCurrent == mPlaylist
			||  wCurrent == mBrowser
			||  (wCurrent == mSearcher && !mSearcher->Current().first)
			||  wCurrent == mLibSongs
			||  wCurrent == mPlaylistEditor
#			ifdef HAVE_TAGLIB_H
			||  wCurrent == mEditorTags
#			endif // HAVE_TAGLIB_H
			   )
			{
				
				List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
				for (size_t i = 0; i < mList->Size(); i++)
					mList->Select(i, !mList->isSelected(i) && !mList->isStatic(i));
				// hackish shit begins
				if (wCurrent == mBrowser && browsed_dir != "/")
					mList->Select(0, 0); // [..] cannot be selected, uhm.
				if (wCurrent == mSearcher)
					mList->Select(search_engine_reset_button, 0); // 'Reset' cannot be selected, omgplz.
				// hacking shit ends.
				ShowMessage("Selection reversed!");
			}
		}
		else if (Keypressed(input, Key.DeselectAll))
		{
			if (wCurrent == mPlaylist
			||  wCurrent == mBrowser
			||  wCurrent == mSearcher
			||  wCurrent == mLibSongs
			||  wCurrent == mPlaylistEditor
#			ifdef HAVE_TAGLIB_H
			||  wCurrent == mEditorTags
#			endif // HAVE_TAGLIB_H
			   )
			{
				List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
				if (mList->hasSelected())
				{
					for (size_t i = 0; i < mList->Size(); i++)
						mList->Select(i, 0);
					ShowMessage("Items deselected!");
				}
			}
		}
		else if (Keypressed(input, Key.AddSelected))
		{
			if (wCurrent != mPlaylist
			&&  wCurrent != mBrowser
			&&  wCurrent != mSearcher
			&&  wCurrent != mLibSongs
			&&  wCurrent != mPlaylistEditor)
				continue;
			
			List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
			if (!mList->hasSelected())
			{
				ShowMessage("No selected items!");
				continue;
			}
			
			vector<size_t> list;
			mList->GetSelected(list);
			SongList result;
			for (vector<size_t>::const_iterator it = list.begin(); it != list.end(); it++)
			{
				switch (current_screen)
				{
					case csPlaylist:
					{
						Song *s = new Song(mPlaylist->at(*it));
						result.push_back(s);
						break;
					}
					case csBrowser:
					{
						const Item &item = mBrowser->at(*it);
						switch (item.type)
						{
							case itDirectory:
							{
								Mpd->GetDirectoryRecursive(locale_to_utf_cpy(item.name), result);
								break;
							}
							case itSong:
							{
								Song *s = new Song(*item.song);
								result.push_back(s);
								break;
							}
							case itPlaylist:
							{
								Mpd->GetPlaylistContent(locale_to_utf_cpy(item.name), result);
								break;
							}
						}
						break;
					}
					case csSearcher:
					{
						Song *s = new Song(*mSearcher->at(*it).second);
						result.push_back(s);
						break;
					}
					case csLibrary:
					{
						Song *s = new Song(mLibSongs->at(*it));
						result.push_back(s);
						break;
					}
					case csPlaylistEditor:
					{
						Song *s = new Song(mPlaylistEditor->at(*it));
						result.push_back(s);
						break;
					}
					default:
						break;
				}
			}
			
			size_t dialog_width = COLS*0.8;
			size_t dialog_height = LINES*0.6;
			Menu<string> *mDialog = new Menu<string>((COLS-dialog_width)/2, (LINES-dialog_height)/2, dialog_width, dialog_height, "Add selected items to...", Config.main_color, Config.window_border);
			mDialog->SetTimeout(ncmpcpp_window_timeout);
			mDialog->SetItemDisplayer(Display::Generic);
			
			bool playlists_not_active = current_screen == csBrowser && Config.local_browser;
			
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
			prev_screen = current_screen;
			current_screen = csOther;
			
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
			
			current_screen = prev_screen;
			size_t id = mDialog->Choice();
			
//			redraw_screen = 1;
			if (current_screen == csLibrary)
			{
				MediaLibrary::Refresh();
			}
			else if (current_screen == csPlaylistEditor)
			{
				PlaylistEditor::Refresh();
			}
			else
				wCurrent->Refresh();
			
			if (id == 0)
			{
				for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
					Mpd->QueueAddSong(**it);
				if (Mpd->CommitQueue())
				{
					ShowMessage("Selected items added!");
					Song &s = mPlaylist->at(mPlaylist->Size()-result.size());
					if (s.GetHash() != result[0]->GetHash())
						ShowMessage("%s", message_part_of_songs_added);
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
				if (!Config.local_browser && browsed_dir == "/")
					GetDirectory("/");
				mPlaylistList->Clear(0); // make playlist editor update itself
			}
			timer = time(NULL);
			delete mDialog;
			FreeSongList(result);
		}
		else if (Keypressed(input, Key.Crop))
		{
			if (mPlaylist->hasSelected())
			{
				for (size_t i = 0; i < mPlaylist->Size(); i++)
				{
					if (!mPlaylist->isSelected(i) && i != size_t(now_playing))
						Mpd->QueueDeleteSongId(mPlaylist->at(i).GetID());
				}
				// if mpd deletes now playing song deletion will be sluggishly slow
				// then so we have to assure it will be deleted at the very end.
				if (now_playing >= 0 && !mPlaylist->isSelected(now_playing))
					Mpd->QueueDeleteSongId(mPlaylist->at(now_playing).GetID());
				
				ShowMessage("Deleting all items but selected...");
				Mpd->CommitQueue();
				ShowMessage("Items deleted!");
			}
			else
			{
				if (now_playing < 0)
				{
					ShowMessage("Nothing is playing now!");
					continue;
				}
				for (int i = 0; i < now_playing; i++)
					Mpd->QueueDeleteSongId(mPlaylist->at(i).GetID());
				for (size_t i = now_playing+1; i < mPlaylist->Size(); i++)
					Mpd->QueueDeleteSongId(mPlaylist->at(i).GetID());
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
		else if (Keypressed(input, Key.FindForward) || Keypressed(input, Key.FindBackward))
		{
			if ((current_screen == csHelp
			||   current_screen == csSearcher
#			ifdef HAVE_TAGLIB_H
			||   current_screen == csTinyTagEditor
			||   wCurrent == mEditorTagTypes
#			endif // HAVE_TAGLIB_H
			    )
			&&  (current_screen != csSearcher
			||   mSearcher->Current().first)
			   )
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
			List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
			for (size_t i = (wCurrent == mSearcher ? search_engine_static_options : 0); i < mList->Size(); i++)
			{
				string name;
				switch (current_screen)
				{
					case csPlaylist:
						name = mPlaylist->at(i).toString(Config.song_list_format);
						break;
					case csBrowser:
						switch (mBrowser->at(i).type)
						{
							case itDirectory:
								name = mBrowser->at(i).name;
								break;
							case itSong:
								name = mBrowser->at(i).song->toString(Config.song_list_format);
								break;
							case itPlaylist:
								name = Config.browser_playlist_prefix.Str();
								name += mBrowser->at(i).name;
								break;
						}
						break;
					case csSearcher:
						name = mSearcher->at(i).second->toString(Config.song_list_format);
						break;
					case csLibrary:
						if (wCurrent == mLibArtists)
							name = mLibArtists->at(i);
						else if (wCurrent == mLibAlbums)
							name = mLibAlbums->at(i).first;
						else
							name = mLibSongs->at(i).toString(Config.song_library_format);
						break;
					case csPlaylistEditor:
						if (wCurrent == mPlaylistList)
							name = mPlaylistList->at(i);
						else
							name = mPlaylistEditor->at(i).toString(Config.song_list_format);
						break;
#					ifdef HAVE_TAGLIB_H
					case csTagEditor:
						if (wCurrent == mEditorLeftCol)
							name = mEditorLeftCol->at(i).first;
						else
						{
							const Song &s = mEditorTags->at(i);
							switch (mEditorTagTypes->Choice())
							{
								case 0:
									name = s.GetTitle();
									break;
								case 1:
									name = s.GetArtist();
									break;
								case 2:
									name = s.GetAlbum();
									break;
								case 3:
									name = s.GetYear();
									break;
								case 4:
									name = s.GetTrack();
									break;
								case 5:
									name = s.GetGenre();
									break;
								case 6:
									name = s.GetComposer();
									break;
								case 7:
									name = s.GetPerformer();
									break;
								case 8:
									name = s.GetDisc();
									break;
								case 9:
									name = s.GetComment();
									break;
								case 11:
									if (s.GetNewName().empty())
										name = s.GetName();
									else
									{
										name = s.GetName();
										name += " -> ";
										name += s.GetNewName();
									}
									break;
								default:
									break;
							}
						}
						break;
#					endif // HAVE_TAGLIB_H
					default:
						break;
				}
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
				if (wCurrent == mPlaylist)
				{
					timer = time(NULL);
					mPlaylist->Highlighting(1);
				}
			}
		}
		else if (Keypressed(input, Key.NextFoundPosition) || Keypressed(input, Key.PrevFoundPosition))
		{
			if (!vFoundPositions.empty())
			{
				List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
				try
				{
					mList->Highlight(vFoundPositions.at(Keypressed(input, Key.NextFoundPosition) ? ++found_pos : --found_pos));
				}
				catch (std::out_of_range)
				{
					if (Config.wrapped_search)
					{
						
						if (Keypressed(input, Key.NextFoundPosition))
						{
							mList->Highlight(vFoundPositions.front());
							found_pos = 0;
						}
						else
						{
							mList->Highlight(vFoundPositions.back());
							found_pos = vFoundPositions.size()-1;
						}
					}
					else
						found_pos = Keypressed(input, Key.NextFoundPosition) ? vFoundPositions.size()-1 : 0;
				}
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
			if (wCurrent == mBrowser && Mpd->GetHostname()[0] == '/')
			{
				Config.local_browser = !Config.local_browser;
				ShowMessage("Browse mode: %s", Config.local_browser ? "Local filesystem" : "MPD music dir");
				browsed_dir = Config.local_browser ? home_folder : "/";
				mBrowser->Reset();
				GetDirectory(browsed_dir);
				redraw_header = 1;
			}
			else if (wCurrent == mLibArtists)
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
					mLibArtists->SetTitle(item_type + "s");
					mLibArtists->Reset();
					mLibArtists->Clear(0);
					mLibArtists->Display();
					ToLower(item_type);
					ShowMessage("Switched to list of %s tag", item_type.c_str());
				}
			}
		}
		else if (Keypressed(input, Key.SongInfo))
		{
			Info::GetSong();
		}
#		ifdef HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ArtistInfo))
		{
			Info::GetArtist();
		}
#		endif // HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.Lyrics))
		{
			Lyrics::Get();
		}
		else if (Keypressed(input, Key.Help))
		{
			Help::SwitchTo();
		}
		else if (Keypressed(input, Key.ScreenSwitcher))
		{
			if (current_screen == csPlaylist)
				Browser::SwitchTo();
			else
				Playlist::SwitchTo();
		}
		else if (Keypressed(input, Key.Playlist))
		{
			Playlist::SwitchTo();
		}
		else if (Keypressed(input, Key.Browser))
		{
			Browser::SwitchTo();
		}
		else if (Keypressed(input, Key.SearchEngine))
		{
			SearchEngine::SwitchTo();
		}
		else if (Keypressed(input, Key.MediaLibrary))
		{
			MediaLibrary::SwitchTo();
		}
		else if (Keypressed(input, Key.PlaylistEditor))
		{
			PlaylistEditor::SwitchTo();
		}
#		ifdef HAVE_TAGLIB_H
		else if (Keypressed(input, Key.TagEditor))
		{
			CHECK_MPD_MUSIC_DIR;
			TagEditor::SwitchTo();
		}
#		endif // HAVE_TAGLIB_H
#		ifdef ENABLE_CLOCK
		else if (Keypressed(input, Key.Clock))
		{
			Clock::SwitchTo();
		}
#		endif // ENABLE_CLOCK
		else if (Keypressed(input, Key.Quit))
			main_exit = 1;
		
		// key mapping end
	}
	// restore old cerr buffer
	std::cerr.rdbuf(cerr_buffer);
	errorlog.close();
	Mpd->Disconnect();
	DestroyScreen();
	return 0;
}

