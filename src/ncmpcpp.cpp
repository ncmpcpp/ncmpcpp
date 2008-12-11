/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <clocale>
#include <csignal>

#include <algorithm>
#include <iostream>
#include <map>

#include "mpdpp.h"
#include "ncmpcpp.h"

#include "browser.h"
#include "help.h"
#include "helpers.h"
#include "lyrics.h"
#include "search_engine.h"
#include "settings.h"
#include "song.h"
#include "status_checker.h"
#include "tag_editor.h"

#define CHECK_MPD_MUSIC_DIR \
			if (Config.mpd_music_dir.empty()) \
			{ \
				ShowMessage("configuration variable mpd_music_dir is not set!"); \
				continue; \
			}

#define REFRESH_MEDIA_LIBRARY_SCREEN \
			do { \
				mLibArtists->Display(); \
				mvvline(main_start_y, middle_col_startx-1, 0, main_height); \
				mLibAlbums->Display(); \
				mvvline(main_start_y, right_col_startx-1, 0, main_height); \
				mLibSongs->Display(); \
				if (mLibAlbums->Empty()) \
				{ \
					mLibAlbums->WriteXY(0, 0, 0, "No albums found."); \
					mLibAlbums->Refresh(); \
				} \
			} while (0)

#define REFRESH_PLAYLIST_EDITOR_SCREEN \
			do { \
				mPlaylistList->Display(); \
				mvvline(main_start_y, middle_col_startx-1, 0, main_height); \
				mPlaylistEditor->Display(); \
			} while (0)

#ifdef HAVE_TAGLIB_H
# define REFRESH_TAG_EDITOR_SCREEN \
			do { \
				mEditorLeftCol->Display(); \
				mvvline(main_start_y, middle_col_startx-1, 0, main_height); \
				mEditorTagTypes->Display(); \
				mvvline(main_start_y, right_col_startx-1, 0, main_height); \
				mEditorTags->Display(); \
			} while (0)
#endif // HAVE_TAGLIB_H

#define CLEAR_FIND_HISTORY \
			do { \
				found_pos = 0; \
				vFoundPositions.clear(); \
			} while (0)

using namespace MPD;

ncmpcpp_config Config;
ncmpcpp_keys Key;

vector<int> vFoundPositions;
int found_pos = 0;

Window *wCurrent = 0;
Window *wPrev = 0;

Menu<Song> *mPlaylist;
Menu<Item> *mBrowser;
Menu< std::pair<Buffer *, Song *> > *mSearcher;

Menu<string> *mLibArtists;
Menu<StringPair> *mLibAlbums;
Menu<Song> *mLibSongs;

#ifdef HAVE_TAGLIB_H
Menu<Buffer> *mTagEditor;
Menu<StringPair> *mEditorAlbums;
Menu<StringPair> *mEditorDirs;
Menu<string> *mEditorTagTypes;
#endif // HAVE_TAGLIB_H
// blah, I use them in conditionals, so just let them be.
Menu<StringPair> *mEditorLeftCol = 0;
Menu<Song> *mEditorTags = 0;

Menu<string> *mPlaylistList;
Menu<Song> *mPlaylistEditor;

Scrollpad *sHelp;
Scrollpad *sLyrics;
Scrollpad *sInfo;

Window *wHeader;
Window *wFooter;

Connection *Mpd;

int now_playing = -1;
int lock_statusbar_delay = -1;

size_t browsed_dir_scroll_begin = 0;

time_t timer;

string browsed_dir = "/";
string editor_browsed_dir = "/";
string editor_highlighted_dir;

NcmpcppScreen current_screen;
NcmpcppScreen prev_screen;

#ifdef HAVE_CURL_CURL_H
pthread_t lyrics_downloader;
pthread_t artist_info_downloader;
#endif

bool dont_change_now_playing = 0;
bool block_progressbar_update = 0;
bool block_playlist_update = 0;
bool block_item_list_update = 0;

bool messages_allowed = 0;
bool redraw_screen = 0;
bool redraw_header = 1;
bool reload_lyrics = 0;

extern bool header_update_status;
extern bool search_case_sensitive;
extern bool search_match_to_pattern;

extern string EMPTY_TAG;
extern string playlist_stats;
extern string volume_state;

extern const char *search_mode_normal;
extern const char *search_mode_strict;

const char *message_part_of_songs_added = "Only part of requested songs' list added to playlist!";

int main(int argc, char *argv[])
{
	CreateConfigDir();
	DefaultConfiguration(Config);
	DefaultKeys(Key);
	ReadConfiguration(Config);
	ReadKeys(Key);
	DefineEmptyTags();
	
	Mpd = new Connection;
	
	if (getenv("MPD_HOST"))
		Mpd->SetHostname(getenv("MPD_HOST"));
	if (getenv("MPD_PORT"))
		Mpd->SetPort(atoi(getenv("MPD_PORT")));
	if (getenv("MPD_PASSWORD"))
		Mpd->SetPassword(getenv("MPD_PASSWORD"));
	
	if (Config.mpd_host != "localhost")
		Mpd->SetHostname(Config.mpd_host);
	if (Config.mpd_port != 6600)
		Mpd->SetPort(Config.mpd_port);
	
	Mpd->SetTimeout(Config.mpd_connection_timeout);
	
	if (argc > 1)
	{
		if (ParseArgv(argc, argv))
		{
			Mpd->Disconnect();
			return 0;
		}
	}
	
	if (!ConnectToMPD())
		return -1;
	
	InitScreen(Config.colors_enabled);
	
	int main_start_y = 2;
	int main_height = LINES-4;
	
	if (!Config.header_visibility)
	{
		main_start_y -= 2;
		main_height += 2;
	}
	if (!Config.statusbar_visibility)
		main_height++;
	
	mPlaylist = new Menu<Song>(0, main_start_y, COLS, main_height, Config.columns_in_playlist ? DisplayColumns(Config.song_columns_list_format) : "", Config.main_color, brNone);
	mPlaylist->SetTimeout(ncmpcpp_window_timeout);
	mPlaylist->HighlightColor(Config.main_highlight_color);
	mPlaylist->SetSelectPrefix(&Config.selected_item_prefix);
	mPlaylist->SetSelectSuffix(&Config.selected_item_suffix);
	mPlaylist->SetItemDisplayer(Config.columns_in_playlist ? DisplaySongInColumns : DisplaySong);
	mPlaylist->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
	
	mBrowser = new Menu<Item>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mBrowser->HighlightColor(Config.main_highlight_color);
	mBrowser->SetTimeout(ncmpcpp_window_timeout);
	mBrowser->SetSelectPrefix(&Config.selected_item_prefix);
	mBrowser->SetSelectSuffix(&Config.selected_item_suffix);
	mBrowser->SetItemDisplayer(DisplayItem);
	
	mSearcher = new Menu< std::pair<Buffer *, Song *> >(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mSearcher->HighlightColor(Config.main_highlight_color);
	mSearcher->SetTimeout(ncmpcpp_window_timeout);
	mSearcher->SetItemDisplayer(SearchEngineDisplayer);
	mSearcher->SetSelectPrefix(&Config.selected_item_prefix);
	mSearcher->SetSelectSuffix(&Config.selected_item_suffix);
	
	int left_col_width = COLS/3-1;
	int middle_col_width = COLS/3;
	int middle_col_startx = left_col_width+1;
	int right_col_width = COLS-COLS/3*2-1;
	int right_col_startx = left_col_width+middle_col_width+2;
	
	mLibArtists = new Menu<string>(0, main_start_y, left_col_width, main_height, IntoStr(Config.media_lib_primary_tag) + "s", Config.main_color, brNone);
	mLibArtists->HighlightColor(Config.main_highlight_color);
	mLibArtists->SetTimeout(ncmpcpp_window_timeout);
	mLibArtists->SetItemDisplayer(GenericDisplayer);
	
	mLibAlbums = new Menu<StringPair>(middle_col_startx, main_start_y, middle_col_width, main_height, "Albums", Config.main_color, brNone);
	mLibAlbums->HighlightColor(Config.main_highlight_color);
	mLibAlbums->SetTimeout(ncmpcpp_window_timeout);
	mLibAlbums->SetItemDisplayer(DisplayStringPair);
	
	mLibSongs = new Menu<Song>(right_col_startx, main_start_y, right_col_width, main_height, "Songs", Config.main_color, brNone);
	mLibSongs->HighlightColor(Config.main_highlight_color);
	mLibSongs->SetTimeout(ncmpcpp_window_timeout);
	mLibSongs->SetSelectPrefix(&Config.selected_item_prefix);
	mLibSongs->SetSelectSuffix(&Config.selected_item_suffix);
	mLibSongs->SetItemDisplayer(DisplaySong);
	mLibSongs->SetItemDisplayerUserData(&Config.song_library_format);
	
#	ifdef HAVE_TAGLIB_H
	mTagEditor = new Menu<Buffer>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mTagEditor->HighlightColor(Config.main_highlight_color);
	mTagEditor->SetTimeout(ncmpcpp_window_timeout);
	mTagEditor->SetItemDisplayer(GenericDisplayer);
	
	mEditorAlbums = new Menu<StringPair>(0, main_start_y, left_col_width, main_height, "Albums", Config.main_color, brNone);
	mEditorAlbums->HighlightColor(Config.main_highlight_color);
	mEditorAlbums->SetTimeout(ncmpcpp_window_timeout);
	mEditorAlbums->SetItemDisplayer(DisplayStringPair);
	
	mEditorDirs = new Menu<StringPair>(0, main_start_y, left_col_width, main_height, "Directories", Config.main_color, brNone);
	mEditorDirs->HighlightColor(Config.main_highlight_color);
	mEditorDirs->SetTimeout(ncmpcpp_window_timeout);
	mEditorDirs->SetItemDisplayer(DisplayStringPair);
	mEditorLeftCol = Config.albums_in_tag_editor ? mEditorAlbums : mEditorDirs;
	
	mEditorTagTypes = new Menu<string>(middle_col_startx, main_start_y, middle_col_width, main_height, "Tag types", Config.main_color, brNone);
	mEditorTagTypes->HighlightColor(Config.main_highlight_color);
	mEditorTagTypes->SetTimeout(ncmpcpp_window_timeout);
	mEditorTagTypes->SetItemDisplayer(GenericDisplayer);
	
	mEditorTags = new Menu<Song>(right_col_startx, main_start_y, right_col_width, main_height, "Tags", Config.main_color, brNone);
	mEditorTags->HighlightColor(Config.main_highlight_color);
	mEditorTags->SetTimeout(ncmpcpp_window_timeout);
	mEditorTags->SetSelectPrefix(&Config.selected_item_prefix);
	mEditorTags->SetSelectSuffix(&Config.selected_item_suffix);
	mEditorTags->SetItemDisplayer(DisplayTag);
	mEditorTags->SetItemDisplayerUserData(mEditorTagTypes);
#	endif // HAVE_TAGLIB_H
	
	mPlaylistList = new Menu<string>(0, main_start_y, left_col_width, main_height, "Playlists", Config.main_color, brNone);
	mPlaylistList->HighlightColor(Config.main_highlight_color);
	mPlaylistList->SetTimeout(ncmpcpp_window_timeout);
	mPlaylistList->SetItemDisplayer(GenericDisplayer);
	
	mPlaylistEditor = new Menu<Song>(middle_col_startx, main_start_y, middle_col_width+right_col_width+1, main_height, "Playlist's content", Config.main_color, brNone);
	mPlaylistEditor->HighlightColor(Config.main_highlight_color);
	mPlaylistEditor->SetTimeout(ncmpcpp_window_timeout);
	mPlaylistEditor->SetSelectPrefix(&Config.selected_item_prefix);
	mPlaylistEditor->SetSelectSuffix(&Config.selected_item_suffix);
	mPlaylistEditor->SetItemDisplayer(DisplaySong);
	mPlaylistEditor->SetItemDisplayerUserData(&Config.song_list_format);
	
	sHelp = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	sHelp->SetTimeout(ncmpcpp_window_timeout);
	GetKeybindings(*sHelp);
	sHelp->Flush();
	
	sLyrics = sHelp->EmptyClone();
	sLyrics->SetTimeout(ncmpcpp_window_timeout);
	
	sInfo = sHelp->EmptyClone();
	sInfo->SetTimeout(ncmpcpp_window_timeout);
	
	if (Config.header_visibility)
	{
		wHeader = new Window(0, 0, COLS, 2, "", Config.header_color, brNone);
		wHeader->Display();
	}
	
	int footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
	int footer_height = Config.statusbar_visibility ? 2 : 1;
	
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
	Song sought_pattern;
	
	bool main_exit = 0;
	bool title_allowed = 1;
	
	string lyrics_title;
	string info_title;
	// local variables end
	
	signal(SIGPIPE, SIG_IGN);
	
#	ifdef HAVE_CURL_CURL_H
	pthread_attr_t attr_detached;
	pthread_attr_init(&attr_detached);
	pthread_attr_setdetachstate(&attr_detached, PTHREAD_CREATE_DETACHED);
#	endif // HAVE_CURL_CURL_H
	
	while (!main_exit)
	{
		if (!Mpd->Connected())
		{
			ShowMessage("Attempting to reconnect...");
			Mpd->Disconnect();
			if (Mpd->Connect())
				ShowMessage("Connected!");
			messages_allowed = 0;
		}
		
		TraceMpdStatus();
		
		block_item_list_update = 0;
		block_playlist_update = 0;
		messages_allowed = 1;
		
		// header stuff
		const size_t max_allowed_title_length = wHeader ? wHeader->GetWidth()-volume_state.length() : 0;
		if (current_screen == csBrowser && input == ERR && browsed_dir.length() > max_allowed_title_length)
			redraw_header = 1;
		if (Config.header_visibility && redraw_header)
		{
			string title;
			
			switch (current_screen)
			{
				case csHelp:
					title = "Help";
					break;
				case csPlaylist:
					title = "Playlist ";
					break;
				case csBrowser:
					title = "Browse: ";
					break;
				case csTinyTagEditor:
				case csTagEditor:
					title = "Tag editor";
					break;
				case csInfo:
					title = info_title;
					break;
				case csSearcher:
					title = "Search engine";
					break;
				case csLibrary:
					title = "Media library";
					break;
				case csLyrics:
					title = lyrics_title;
					break;
				case csPlaylistEditor:
					title = "Playlist editor";
					break;
				default:
					break;
			}
			
			if (title_allowed)
			{
				wHeader->Bold(1);
				wHeader->WriteXY(0, 0, 1, "%s", title.c_str());
				wHeader->Bold(0);
				
				if (current_screen == csPlaylist && !playlist_stats.empty())
					wHeader->WriteXY(title.length(), 0, 0, "%s", playlist_stats.c_str());
				else if (current_screen == csBrowser)
				{
					size_t max_length_without_scroll = wHeader->GetWidth()-volume_state.length()-title.length();
					my_string_t wbrowseddir = TO_WSTRING(browsed_dir);
					wHeader->Bold(1);
					if (browsed_dir.length() > max_length_without_scroll)
					{
#						ifdef _UTF8
						wbrowseddir += L" ** ";
#						else
						wbrowseddir += " ** ";
#						endif
						const size_t scrollsize = max_length_without_scroll;
						my_string_t part = wbrowseddir.substr(browsed_dir_scroll_begin++, scrollsize);
						if (part.length() < scrollsize)
							part += wbrowseddir.substr(0, scrollsize-part.length());
						wHeader->WriteXY(title.length(), 0, 0, UTF_S_FMT, part.c_str());
						if (browsed_dir_scroll_begin >= wbrowseddir.length())
							browsed_dir_scroll_begin = 0;
					}
					else
						wHeader->WriteXY(title.length(), 0, 0, "%s", browsed_dir.c_str());
					wHeader->Bold(0);
				}
			}
			else
			{
				string screens = "[.b]1:[/b]Help  [.b]2:[/b]Playlist  [.b]3:[/b]Browse  [.b]4:[/b]Search  [.b]5:[/b]Library  [.b]6:[/b]Playlist editor";
#				ifdef HAVE_TAGLIB_H
				screens += "  [.b]7:[/b]Tag editor";
#				endif // HAVE_TAGLIB_H
				wHeader->WriteXY(0, 0, 1, "%s", screens.c_str());
			}
			
			wHeader->SetColor(Config.volume_color);
			wHeader->WriteXY(max_allowed_title_length, 0, 0, "%s", volume_state.c_str());
			wHeader->SetColor(Config.header_color);
			wHeader->Refresh();
			redraw_header = 0;
		}
		// header stuff end
		
		if (current_screen == csHelp
		||  current_screen == csPlaylist
		||  current_screen == csBrowser);
		else
		// media library stuff
		if (current_screen == csLibrary)
		{
			if (mLibArtists->Empty())
			{
				CLEAR_FIND_HISTORY;
				TagList list;
				mLibAlbums->Clear(0);
				mLibSongs->Clear(0);
				Mpd->GetList(list, Config.media_lib_primary_tag);
				sort(list.begin(), list.end(), CaseInsensitiveSorting());
				for (TagList::iterator it = list.begin(); it != list.end(); it++)
				{
					if ((mLibArtists->Empty() || mLibArtists->Back() != *it) && !it->empty())
					{
						mLibArtists->AddOption(*it);
					}
				}
				mLibArtists->Window::Clear();
				mLibArtists->Refresh();
			}
			
			if (!mLibArtists->Empty() && mLibAlbums->Empty() && mLibSongs->Empty())
			{
				mLibAlbums->Reset();
				TagList list;
				std::map<string, string, CaseInsensitiveSorting> maplist;
				if (Config.media_lib_primary_tag == MPD_TAG_ITEM_ARTIST)
					Mpd->GetAlbums(mLibArtists->Current(), list);
				else
				{
					Mpd->StartSearch(1);
					Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
					Mpd->StartFieldSearch(MPD_TAG_ITEM_ALBUM);
					Mpd->CommitSearch(list);
				}
				for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					SongList l;
					Mpd->StartSearch(1);
					Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, *it);
					Mpd->CommitSearch(l);
					if (!l.empty() && l[0]->GetAlbum() != EMPTY_TAG)
						maplist[l[0]->toString(Config.media_lib_album_format)] = *it;
					FreeSongList(l);
				}
				for (std::map<string, string>::const_iterator it = maplist.begin(); it != maplist.end(); it++)
					mLibAlbums->AddOption(make_pair(it->first, it->second));
				mLibAlbums->Window::Clear();
				mLibAlbums->Refresh();
			}
			
			if (!mLibArtists->Empty() && wCurrent == mLibAlbums && mLibAlbums->Empty())
			{
				mLibAlbums->HighlightColor(Config.main_highlight_color);
				mLibArtists->HighlightColor(Config.active_column_color);
				wCurrent = mLibArtists;
			}
			
			if (!mLibArtists->Empty() && mLibSongs->Empty())
			{
				mLibSongs->Reset();
				SongList list;
				if (mLibAlbums->Empty())
				{
					mLibAlbums->WriteXY(0, 0, 0, "No albums found.");
					mLibAlbums->Refresh();
					mLibSongs->Clear(0);
					Mpd->StartSearch(1);
					Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
					Mpd->CommitSearch(list);
				}
				else
				{
					mLibSongs->Clear(0);
					Mpd->StartSearch(1);
					Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, mLibAlbums->Current().second);
					Mpd->CommitSearch(list);
				}
				sort(list.begin(), list.end(), SortSongsByTrack);
				bool bold = 0;
			
				for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					for (size_t j = 0; j < mPlaylist->Size(); j++)
					{
						if ((*it)->GetHash() == mPlaylist->at(j).GetHash())
						{
							bold = 1;
							break;
						}
					}
					mLibSongs->AddOption(**it, bold);
					bold = 0;
				}
				FreeSongList(list);
				mLibSongs->Window::Clear();
				mLibSongs->Refresh();
			}
		}
		// media library end
		else
		// playlist editor stuff
		if (current_screen == csPlaylistEditor)
		{
			if (mPlaylistList->Empty())
			{
				mPlaylistEditor->Clear(0);
				TagList list;
				Mpd->GetPlaylists(list);
				for (TagList::iterator it = list.begin(); it != list.end(); it++)
					mPlaylistList->AddOption(*it);
				mPlaylistList->Window::Clear();
				mPlaylistList->Refresh();
			}
			
			if (!mPlaylistList->Empty() && mPlaylistEditor->Empty())
			{
				mPlaylistEditor->Reset();
				SongList list;
				Mpd->GetPlaylistContent(mPlaylistList->Current(), list);
				if (!list.empty())
					mPlaylistEditor->SetTitle("Playlist's content (" + IntoStr(list.size()) + " item" + (list.size() == 1 ? ")" : "s)"));
				else
					mPlaylistEditor->SetTitle("Playlist's content");
				bool bold = 0;
				for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					for (size_t j = 0; j < mPlaylist->Size(); j++)
					{
						if ((*it)->GetHash() == mPlaylist->at(j).GetHash())
						{
							bold = 1;
							break;
						}
					}
					mPlaylistEditor->AddOption(**it, bold);
					bold = 0;
				}
				FreeSongList(list);
				mPlaylistEditor->Window::Clear();
				mPlaylistEditor->Display();
			}
			
			if (wCurrent == mPlaylistEditor && mPlaylistEditor->Empty())
			{
				mPlaylistEditor->HighlightColor(Config.main_highlight_color);
				mPlaylistList->HighlightColor(Config.active_column_color);
				wCurrent = mPlaylistList;
			}
			
			if (mPlaylistEditor->Empty())
			{
				mPlaylistEditor->WriteXY(0, 0, 0, "Playlist is empty.");
				mPlaylistEditor->Refresh();
			}
		}
		// playlist editor end
		else
#		ifdef HAVE_TAGLIB_H
		// album editor stuff
		if (current_screen == csTagEditor)
		{
			if (mEditorLeftCol->Empty())
			{
				CLEAR_FIND_HISTORY;
				mEditorLeftCol->Window::Clear();
				mEditorTags->Clear();
				TagList list;
				if (Config.albums_in_tag_editor)
				{
					std::map<string, string, CaseInsensitiveSorting> maplist;
					mEditorAlbums->WriteXY(0, 0, 0, "Fetching albums' list...");
					Mpd->GetAlbums("", list);
					for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
					{
						SongList l;
						Mpd->StartSearch(1);
						Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, *it);
						Mpd->CommitSearch(l);
						if (!l.empty())
							maplist[l[0]->toString(Config.tag_editor_album_format)] = *it;
						FreeSongList(l);
					}
					for (std::map<string, string>::const_iterator it = maplist.begin(); it != maplist.end(); it++)
						mEditorAlbums->AddOption(make_pair(it->first, it->second));
				}
				else
				{
					int highlightme = -1;
					Mpd->GetDirectories(editor_browsed_dir, list);
					sort(list.begin(), list.end(), CaseInsensitiveSorting());
					if (editor_browsed_dir != "/")
					{
						size_t slash = editor_browsed_dir.find_last_of("/");
						string parent = slash != string::npos ? editor_browsed_dir.substr(0, slash) : "/";
						mEditorDirs->AddOption(make_pair("[..]", parent));
					}
					else
					{
						mEditorDirs->AddOption(make_pair(".", "/"));
					}
					for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
					{
						size_t slash = it->find_last_of("/");
						string to_display = slash != string::npos ? it->substr(slash+1) : *it;
						mEditorDirs->AddOption(make_pair(to_display, *it));
						if (*it == editor_highlighted_dir)
							highlightme = mEditorDirs->Size()-1;
					}
					if (highlightme != -1)
						mEditorDirs->Highlight(highlightme);
				}
				mEditorLeftCol->Display();
				mEditorTagTypes->Refresh();
			}
			
			if (mEditorTags->Empty())
			{
				mEditorTags->Reset();
				SongList list;
				if (Config.albums_in_tag_editor)
				{
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, mEditorAlbums->Current().second);
					Mpd->CommitSearch(list);
					sort(list.begin(), list.end(), CaseInsensitiveSorting());
					for (SongList::iterator it = list.begin(); it != list.end(); it++)
						mEditorTags->AddOption(**it);
				}
				else
				{
					Mpd->GetSongs(mEditorDirs->Current().second, list);
					sort(list.begin(), list.end(), CaseInsensitiveSorting());
					for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
						mEditorTags->AddOption(**it);
				}
				FreeSongList(list);
				mEditorTags->Window::Clear();
				mEditorTags->Refresh();
			}
			
			if (redraw_screen && wCurrent == mEditorTagTypes && mEditorTagTypes->Choice() < 13)
			{
				mEditorTags->Refresh();
				redraw_screen = 0;
			}
			else if (mEditorTagTypes->Choice() >= 13)
				mEditorTags->Window::Clear();
		}
		// album editor end
		else
#		endif // HAVE_TAGLIB_H
		// lyrics stuff
		if (current_screen == csLyrics && reload_lyrics)
		{
			const Song &s = mPlaylist->at(now_playing);
			if (s.GetArtist() != EMPTY_TAG && s.GetTitle() != EMPTY_TAG)
				goto LOAD_LYRICS;
			else
				reload_lyrics = 0;
		}
		
		if (Config.columns_in_playlist && wCurrent == mPlaylist)
			wCurrent->Display();
		else
			wCurrent->Refresh();
		redraw_screen = 0;
		
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
			case csTagEditor:
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
					else if (wCurrent == mEditorTagTypes)
						redraw_screen = 1;
#					endif // HAVE_TAGLIB_H
				}
			}
			default:
				break;
		}
		
		// key mapping beginning
		
		if (Keypressed(input, Key.Up))
		{
			if (!Config.fancy_scrolling && (wCurrent == mLibArtists || wCurrent == mPlaylistList || wCurrent == mEditorLeftCol))
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
		else if (Keypressed(input, Key.Down))
		{
			if (!Config.fancy_scrolling && (wCurrent == mLibArtists || wCurrent == mPlaylistList || wCurrent == mEditorLeftCol))
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
		else if (Keypressed(input, Key.PageUp))
		{
			wCurrent->Scroll(wPageUp);
		}
		else if (Keypressed(input, Key.PageDown))
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
			redraw_screen = 1;
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
			
			sHelp->Resize(COLS, main_height);
			mPlaylist->Resize(COLS, main_height);
			mPlaylist->SetTitle(Config.columns_in_playlist ? DisplayColumns(Config.song_columns_list_format) : "");
			mBrowser->Resize(COLS, main_height);
			mSearcher->Resize(COLS, main_height);
			sInfo->Resize(COLS, main_height);
			sLyrics->Resize(COLS, main_height);
			
			left_col_width = COLS/3-1;
			middle_col_startx = left_col_width+1;
			middle_col_width = COLS/3;
			right_col_startx = left_col_width+middle_col_width+2;
			right_col_width = COLS-COLS/3*2-1;
			
			mLibArtists->Resize(left_col_width, main_height);
			mLibAlbums->Resize(middle_col_width, main_height);
			mLibSongs->Resize(right_col_width, main_height);
			mLibAlbums->MoveTo(middle_col_startx, main_start_y);
			mLibSongs->MoveTo(right_col_startx, main_start_y);
			
#			ifdef HAVE_TAGLIB_H
			mTagEditor->Resize(COLS, main_height);
			
			mEditorAlbums->Resize(left_col_width, main_height);
			mEditorDirs->Resize(left_col_width, main_height);
			mEditorTagTypes->Resize(middle_col_width, main_height);
			mEditorTags->Resize(right_col_width, main_height);
			mEditorTagTypes->MoveTo(middle_col_startx, main_start_y);
			mEditorTags->MoveTo(right_col_startx, main_start_y);
			
			mPlaylistList->Resize(left_col_width, main_height);
			mPlaylistEditor->Resize(middle_col_width+right_col_width+1, main_height);
			mPlaylistEditor->MoveTo(middle_col_startx, main_start_y);
#			endif // HAVE_TAGLIB_H
			
			if (Config.header_visibility)
				wHeader->Resize(COLS, wHeader->GetHeight());
			
			footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
			wFooter->MoveTo(0, footer_start_y);
			wFooter->Resize(COLS, wFooter->GetHeight());
			
			wCurrent->Hide();
			
#			ifdef HAVE_TAGLIB_H
			if (current_screen == csLibrary)
			{
				REFRESH_MEDIA_LIBRARY_SCREEN;
			}
			else if (current_screen == csTagEditor)
			{
				REFRESH_TAG_EDITOR_SCREEN;
			}
			else
#			endif // HAVE_TAGLIB_H
			if (current_screen == csPlaylistEditor)
			{
				REFRESH_PLAYLIST_EDITOR_SCREEN;
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
				goto ENTER_BROWSER_SCREEN;
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
					ENTER_BROWSER_SCREEN:
					
					const Item &item = mBrowser->Current();
					switch (item.type)
					{
						case itDirectory:
						{
							CLEAR_FIND_HISTORY;
							GetDirectory(item.name, browsed_dir);
							redraw_header = 1;
							break;
						}
						case itSong:
						{
							block_item_list_update = 1;
							if (Config.ncmpc_like_songs_adding && mBrowser->isBold())
							{
								bool found = 0;
								long long hash = mBrowser->Current().song->GetHash();
								for (size_t i = 0; i < mPlaylist->Size(); i++)
								{
									if (mPlaylist->at(i).GetHash() == hash)
									{
										Mpd->Play(i);
										found = 1;
										break;
									}
								}
								if (found)
									break;
							}
							Song &s = *item.song;
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								Mpd->PlayID(id);
								ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
								mBrowser->BoldOption(mBrowser->Choice(), 1);
							}
							break;
						}
						case itPlaylist:
						{
							SongList list;
							Mpd->GetPlaylistContent(item.name, list);
							for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
								Mpd->QueueAddSong(**it);
							if (Mpd->CommitQueue())
							{
								ShowMessage("Loading and playing playlist %s...", item.name.c_str());
								Song *s = &mPlaylist->at(mPlaylist->Size()-list.size());
								if (s->GetHash() == list[0]->GetHash())
									Mpd->PlayID(s->GetID());
								else
									ShowMessage("%s", message_part_of_songs_added);
							}
							FreeSongList(list);
							break;
						}
					}
					break;
				}
#				ifdef HAVE_TAGLIB_H
				case csTinyTagEditor:
				{
					int option = mTagEditor->Choice();
					LockStatusbar();
					Song &s = edited_song;
					
					if (option >= 8 && option <= 20)
						mTagEditor->at(option).Clear();
					
					switch (option-7)
					{
						case 1:
						{
							Statusbar() << fmtBold << "Title: " << fmtBoldEnd;
							if (s.GetTitle() == EMPTY_TAG)
								s.SetTitle(wFooter->GetString());
							else
								s.SetTitle(wFooter->GetString(s.GetTitle()));
							mTagEditor->at(option) << fmtBold << "Title:" << fmtBoldEnd << ' ' << s.GetTitle();
							break;
						}
						case 2:
						{
							Statusbar() << fmtBold << "Artist: " << fmtBoldEnd;
							if (s.GetArtist() == EMPTY_TAG)
								s.SetArtist(wFooter->GetString());
							else
								s.SetArtist(wFooter->GetString(s.GetArtist()));
							mTagEditor->at(option) << fmtBold << "Artist:" << fmtBoldEnd << ' ' << s.GetArtist();
							break;
						}
						case 3:
						{
							Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
							if (s.GetAlbum() == EMPTY_TAG)
								s.SetAlbum(wFooter->GetString());
							else
								s.SetAlbum(wFooter->GetString(s.GetAlbum()));
							mTagEditor->at(option) << fmtBold << "Album:" << fmtBoldEnd << ' ' << s.GetAlbum();
							break;
						}
						case 4:
						{
							Statusbar() << fmtBold << "Year: " << fmtBoldEnd;
							if (s.GetYear() == EMPTY_TAG)
								s.SetYear(wFooter->GetString(4));
							else
								s.SetYear(wFooter->GetString(s.GetYear(), 4));
							mTagEditor->at(option) << fmtBold << "Year:" << fmtBoldEnd << ' ' << s.GetYear();
							break;
						}
						case 5:
						{
							Statusbar() << fmtBold << "Track: " << fmtBoldEnd;
							if (s.GetTrack() == EMPTY_TAG)
								s.SetTrack(wFooter->GetString(3));
							else
								s.SetTrack(wFooter->GetString(s.GetTrack(), 3));
							mTagEditor->at(option) << fmtBold << "Track:" << fmtBoldEnd << ' ' << s.GetTrack();
							break;
						}
						case 6:
						{
							Statusbar() << fmtBold << "Genre: " << fmtBoldEnd;
							if (s.GetGenre() == EMPTY_TAG)
								s.SetGenre(wFooter->GetString());
							else
								s.SetGenre(wFooter->GetString(s.GetGenre()));
							mTagEditor->at(option) << fmtBold << "Genre:" << fmtBoldEnd << ' ' << s.GetGenre();
							break;
						}
						case 7:
						{
							Statusbar() << fmtBold << "Composer: " << fmtBoldEnd;
							if (s.GetComposer() == EMPTY_TAG)
								s.SetComposer(wFooter->GetString());
							else
								s.SetComposer(wFooter->GetString(s.GetComposer()));
							mTagEditor->at(option) << fmtBold << "Composer:" << fmtBoldEnd << ' ' << s.GetComposer();
							break;
						}
						case 8:
						{
							Statusbar() << fmtBold << "Performer: " << fmtBoldEnd;
							if (s.GetPerformer() == EMPTY_TAG)
								s.SetPerformer(wFooter->GetString());
							else
								s.SetPerformer(wFooter->GetString(s.GetPerformer()));
							mTagEditor->at(option) << fmtBold << "Performer:" << fmtBoldEnd << ' ' << s.GetPerformer();
							break;
						}
						case 9:
						{
							Statusbar() << fmtBold << "Disc: " << fmtBoldEnd;
							if (s.GetDisc() == EMPTY_TAG)
								s.SetDisc(wFooter->GetString());
							else
								s.SetDisc(wFooter->GetString(s.GetDisc()));
							mTagEditor->at(option) << fmtBold << "Disc:" << fmtBoldEnd << ' ' << s.GetDisc();
							break;
						}
						case 10:
						{
							Statusbar() << fmtBold << "Comment: " << fmtBoldEnd;
							if (s.GetComment() == EMPTY_TAG)
								s.SetComment(wFooter->GetString());
							else
								s.SetComment(wFooter->GetString(s.GetComment()));
							mTagEditor->at(option) << fmtBold << "Comment:" << fmtBoldEnd << ' ' << s.GetComment();
							break;
						}
						case 12:
						{
							Statusbar() << fmtBold << "Filename: " << fmtBoldEnd;
							string filename = s.GetNewName().empty() ? s.GetName() : s.GetNewName();
							int dot = filename.find_last_of(".");
							string extension = filename.substr(dot);
							filename = filename.substr(0, dot);
							string new_name = wFooter->GetString(filename);
							s.SetNewName(new_name + extension);
							mTagEditor->at(option) << fmtBold << "Filename:" << fmtBoldEnd << ' ' << (s.GetNewName().empty() ? s.GetName() : s.GetNewName());
							break;
						}
						case 14:
						{
							ShowMessage("Updating tags...");
							if (WriteTags(s))
							{
								ShowMessage("Tags updated!");
								if (s.IsFromDB())
								{
									Mpd->UpdateDirectory(s.GetDirectory());
									if (prev_screen == csSearcher)
										*mSearcher->Current().second = s;
								}
								else
								{
									if (wPrev == mPlaylist)
										mPlaylist->Current() = s;
									else if (wPrev == mBrowser)
										*mBrowser->Current().song = s;
								}
							}
							else
								ShowMessage("Error writing tags!");
						}
						case 15:
						{
							wCurrent->Clear();
							wCurrent = wPrev;
							current_screen = prev_screen;
							redraw_screen = 1;
							redraw_header = 1;
							if (current_screen == csLibrary)
							{
								REFRESH_MEDIA_LIBRARY_SCREEN;
							}
							else if (current_screen == csPlaylistEditor)
							{
								REFRESH_PLAYLIST_EDITOR_SCREEN;
							}
							else if (current_screen == csTagEditor)
							{
								REFRESH_TAG_EDITOR_SCREEN;
							}
							break;
						}
					}
					UnlockStatusbar();
					break;
				}
#				endif // HAVE_TAGLIB_H
				case csSearcher:
				{
					ENTER_SEARCH_ENGINE_SCREEN:
					
					int option = mSearcher->Choice();
					LockStatusbar();
					Song &s = sought_pattern;
					
					if (option >= 0 && option <= 11)
						mSearcher->Current().first->Clear();
					
					switch (option+1)
					{
						case 1:
						{
							Statusbar() << fmtBold << "Filename: " << fmtBoldEnd;
							if (s.GetName() == EMPTY_TAG)
								s.SetFile(wFooter->GetString());
							else
								s.SetFile(wFooter->GetString(s.GetFile()));
							*mSearcher->Current().first << fmtBold << "Filename:" << fmtBoldEnd << ' ' << s.GetFile();
							break;
						}
						case 2:
						{
							Statusbar() << fmtBold << "Title: " << fmtBoldEnd;
							if (s.GetTitle() == EMPTY_TAG)
								s.SetTitle(wFooter->GetString());
							else
								s.SetTitle(wFooter->GetString(s.GetTitle()));
							*mSearcher->Current().first << fmtBold << "Title:" << fmtBoldEnd << ' ' << s.GetTitle();
							break;
						}
						case 3:
						{
							Statusbar() << fmtBold << "Artist: " << fmtBoldEnd;
							if (s.GetArtist() == EMPTY_TAG)
								s.SetArtist(wFooter->GetString());
							else
								s.SetArtist(wFooter->GetString(s.GetArtist()));
							*mSearcher->Current().first << fmtBold << "Artist:" << fmtBoldEnd << ' ' << s.GetArtist();
							break;
						}
						case 4:
						{
							Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
							if (s.GetAlbum() == EMPTY_TAG)
								s.SetAlbum(wFooter->GetString());
							else
								s.SetAlbum(wFooter->GetString(s.GetAlbum()));
							*mSearcher->Current().first << fmtBold << "Album:" << fmtBoldEnd << ' ' << s.GetAlbum();
							break;
						}
						case 5:
						{
							Statusbar() << fmtBold << "Year: " << fmtBoldEnd;
							if (s.GetYear() == EMPTY_TAG)
								s.SetYear(wFooter->GetString(4));
							else
								s.SetYear(wFooter->GetString(s.GetYear(), 4));
							*mSearcher->Current().first << fmtBold << "Year:" << fmtBoldEnd << ' ' << s.GetYear();
							break;
						}
						case 6:
						{
							Statusbar() << fmtBold << "Track: " << fmtBoldEnd;
							if (s.GetTrack() == EMPTY_TAG)
								s.SetTrack(wFooter->GetString(3));
							else
								s.SetTrack(wFooter->GetString(s.GetTrack(), 3));
							*mSearcher->Current().first << fmtBold << "Track:" << fmtBoldEnd << ' ' << s.GetTrack();
							break;
						}
						case 7:
						{
							Statusbar() << fmtBold << "Genre: " << fmtBoldEnd;
							if (s.GetGenre() == EMPTY_TAG)
								s.SetGenre(wFooter->GetString());
							else
								s.SetGenre(wFooter->GetString(s.GetGenre()));
							*mSearcher->Current().first << fmtBold << "Genre:" << fmtBoldEnd << ' ' << s.GetGenre();
							break;
						}
						case 8:
						{
							Statusbar() << fmtBold << "Comment: " << fmtBoldEnd;
							if (s.GetComment() == EMPTY_TAG)
								s.SetComment(wFooter->GetString());
							else
								s.SetComment(wFooter->GetString(s.GetComment()));
							*mSearcher->Current().first << fmtBold << "Comment:" << fmtBoldEnd << ' ' << s.GetComment();
							break;
						}
						case 10:
						{
							Config.search_in_db = !Config.search_in_db;
							*mSearcher->Current().first << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
							break;
						}
						case 11:
						{
							search_match_to_pattern = !search_match_to_pattern;
							*mSearcher->Current().first << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << (search_match_to_pattern ? search_mode_normal : search_mode_strict);
							break;
						}
						case 12:
						{
							search_case_sensitive = !search_case_sensitive;
							*mSearcher->Current().first << fmtBold << "Case sensitive:" << fmtBoldEnd << ' ' << (search_case_sensitive ? "Yes" : "No");
							break;
						}
						case 14:
						{
							ShowMessage("Searching...");
							Search(s);
							if (!mSearcher->Back().first)
							{
								int found = mSearcher->Size()-search_engine_static_options;
								found += 3; // don't count options inserted below
								mSearcher->InsertSeparator(15);
								mSearcher->InsertOption(16, make_pair((Buffer *)0, (Song *)0), 1, 1);
								mSearcher->at(16).first = new Buffer();
								*mSearcher->at(16).first << clWhite << "Search results: " << clGreen << "Found " << found  << (found > 1 ? " songs" : " song") << clDefault;
								mSearcher->InsertSeparator(17);
								UpdateFoundList();
								ShowMessage("Searching finished!");
								for (int i = 0; i < search_engine_static_options-4; i++)
									mSearcher->Static(i, 1);
								mSearcher->Scroll(wDown);
								mSearcher->Scroll(wDown);
							}
							else
								ShowMessage("No results found");
							break;
						}
						case 15:
						{
							CLEAR_FIND_HISTORY;
							PrepareSearchEngine(sought_pattern);
							ShowMessage("Search state reset");
							break;
						}
						default:
						{
							block_item_list_update = 1;
							if (Config.ncmpc_like_songs_adding && mSearcher->isBold())
							{
								long long hash = mSearcher->Current().second->GetHash();
								for (size_t i = 0; i < mPlaylist->Size(); i++)
								{
									if (mPlaylist->at(i).GetHash() == hash)
									{
										Mpd->Play(i);
										break;
									}
								}
							}
							else
							{
								const Song &s = *mSearcher->Current().second;
								int id = Mpd->AddSong(s);
								if (id >= 0)
								{
									Mpd->PlayID(id);
									ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
									mSearcher->BoldOption(mSearcher->Choice(), 1);
								}
							}
							break;
						}
					}
					UnlockStatusbar();
					break;
				}
				case csLibrary:
				{
					ENTER_LIBRARY_SCREEN: // same code for Key.Space, but without playing.
					
					SongList list;
					
					if (wCurrent == mLibArtists)
					{
						const string &tag = mLibArtists->Current();
						Mpd->StartSearch(1);
						Mpd->AddSearch(Config.media_lib_primary_tag, tag);
						Mpd->CommitSearch(list);
						for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
							Mpd->QueueAddSong(**it);
						if (Mpd->CommitQueue())
						{
							string tag_type = IntoStr(Config.media_lib_primary_tag);
							ToLower(tag_type);
							ShowMessage("Adding songs of %s \"%s\"", tag_type.c_str(), tag.c_str());
							Song *s = &mPlaylist->at(mPlaylist->Size()-list.size());
							if (s->GetHash() == list[0]->GetHash())
							{
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(s->GetID());
							}
							else
								ShowMessage("%s", message_part_of_songs_added);
						}
					}
					else if (wCurrent == mLibAlbums)
					{
						for (size_t i = 0; i < mLibSongs->Size(); i++)
							Mpd->QueueAddSong(mLibSongs->at(i));
						if (Mpd->CommitQueue())
						{
							ShowMessage("Adding songs from album \"%s\"", mLibAlbums->Current().second.c_str());
							Song *s = &mPlaylist->at(mPlaylist->Size()-mLibSongs->Size());
							if (s->GetHash() == mLibSongs->at(0).GetHash())
							{
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(s->GetID());
							}
							else
								ShowMessage("%s", message_part_of_songs_added);
						}
					}
					else if (wCurrent == mLibSongs)
					{
						if (!mLibSongs->Empty())
						{
							block_item_list_update = 1;
							if (Config.ncmpc_like_songs_adding && mLibSongs->isBold())
							{
								long long hash = mLibSongs->Current().GetHash();
								if (Keypressed(input, Key.Enter))
								{
									for (size_t i = 0; i < mPlaylist->Size(); i++)
									{
										if (mPlaylist->at(i).GetHash() == hash)
										{
											Mpd->Play(i);
											break;
										}
									}
								}
								else
								{
									block_playlist_update = 1;
									for (size_t i = 0; i < mPlaylist->Size(); i++)
									{
										if (mPlaylist->at(i).GetHash() == hash)
										{
											Mpd->QueueDeleteSong(i);
											mPlaylist->DeleteOption(i);
											i--;
										}
									}
									Mpd->CommitQueue();
									mLibSongs->BoldOption(mLibSongs->Choice(), 0);
								}
							}
							else
							{
								Song &s = mLibSongs->Current();
								int id = Mpd->AddSong(s);
								if (id >= 0)
								{
									ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
									if (Keypressed(input, Key.Enter))
										Mpd->PlayID(id);
									mLibSongs->BoldOption(mLibSongs->Choice(), 1);
								}
							}
						}
					}
					FreeSongList(list);
					if (Keypressed(input, Key.Space))
					{
						wCurrent->Scroll(wDown);
						if (wCurrent == mLibArtists)
						{
							mLibAlbums->Clear(0);
							mLibSongs->Clear(0);
						}
						else if (wCurrent == mLibAlbums)
							mLibSongs->Clear(0);
					}
					break;
				}
				case csPlaylistEditor:
				{
					ENTER_PLAYLIST_EDITOR_SCREEN: // same code for Key.Space, but without playing.
					
					SongList list;
					
					if (wCurrent == mPlaylistList)
					{
						const string &playlist = mPlaylistList->Current();
						Mpd->GetPlaylistContent(playlist, list);
						for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
							Mpd->QueueAddSong(**it);
						if (Mpd->CommitQueue())
						{
							ShowMessage("Loading playlist %s...", playlist.c_str());
							Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
							if (s.GetHash() == list[0]->GetHash())
							{
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(s.GetID());
							}
							else
								ShowMessage("%s", message_part_of_songs_added);
						}
					}
					else if (wCurrent == mPlaylistEditor)
					{
						if (!mPlaylistEditor->Empty())
						{
							block_item_list_update = 1;
							if (Config.ncmpc_like_songs_adding && mPlaylistEditor->isBold())
							{
								long long hash = mPlaylistEditor->Current().GetHash();
								if (Keypressed(input, Key.Enter))
								{
									for (size_t i = 0; i < mPlaylist->Size(); i++)
									{
										if (mPlaylist->at(i).GetHash() == hash)
										{
											Mpd->Play(i);
											break;
										}
									}
								}
								else
								{
									block_playlist_update = 1;
									for (size_t i = 0; i < mPlaylist->Size(); i++)
									{
										if (mPlaylist->at(i).GetHash() == hash)
										{
											Mpd->QueueDeleteSong(i);
											mPlaylist->DeleteOption(i);
											i--;
										}
									}
									Mpd->CommitQueue();
									mPlaylistEditor->BoldOption(mPlaylistEditor->Choice(), 0);
								}
							}
							else
							{
								Song &s = mPlaylistEditor->at(mPlaylistEditor->Choice());
								int id = Mpd->AddSong(s);
								if (id >= 0)
								{
									ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
									if (Keypressed(input, Key.Enter))
										Mpd->PlayID(id);
									mPlaylistEditor->BoldOption(mPlaylistEditor->Choice(), 1);
								}
							}
						}
					}
					FreeSongList(list);
					if (Keypressed(input, Key.Space))
						wCurrent->Scroll(wDown);
					break;
				}
#				ifdef HAVE_TAGLIB_H
				case csTagEditor:
				{
					if (wCurrent == mEditorDirs)
					{
						TagList test;
						Mpd->GetDirectories(mEditorLeftCol->Current().second, test);
						if (!test.empty())
						{
							editor_highlighted_dir = editor_browsed_dir;
							editor_browsed_dir = mEditorLeftCol->Current().second;
							mEditorLeftCol->Clear(0);
							mEditorLeftCol->Reset();
						}
						else
							ShowMessage("No subdirs found");
						break;
					}
					
					if (mEditorTags->Empty()) // we need songs to deal with, don't we?
						break;
					
					// if there are selected songs, perform operations only on them
					SongList list;
					if (mEditorTags->hasSelected())
					{
						vector<size_t> selected;
						mEditorTags->GetSelected(selected);
						for (vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); it++)
							list.push_back(&mEditorTags->at(*it));
					}
					else
						for (size_t i = 0; i < mEditorTags->Size(); i++)
							list.push_back(&mEditorTags->at(i));
					
					SongGetFunction get = 0;
					SongSetFunction set = 0;
					
					int id = mEditorTagTypes->RealChoice();
					switch (id)
					{
						case 0:
							get = &Song::GetTitle;
							set = &Song::SetTitle;
							break;
						case 1:
							get = &Song::GetArtist;
							set = &Song::SetArtist;
							break;
						case 2:
							get = &Song::GetAlbum;
							set = &Song::SetAlbum;
							break;
						case 3:
							get = &Song::GetYear;
							set = &Song::SetYear;
							break;
						case 4:
							get = &Song::GetTrack;
							set = &Song::SetTrack;
							if (wCurrent == mEditorTagTypes)
							{
								LockStatusbar();
								Statusbar() << "Number tracks? [y/n] ";
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
									int i = 1;
									for (SongList::iterator it = list.begin(); it != list.end(); it++, i++)
										(*it)->SetTrack(i);
									ShowMessage("Tracks numbered!");
								}
								else
									ShowMessage("Aborted!");
								curs_set(0);
								redraw_screen = 1;
								UnlockStatusbar();
							}
							break;
						case 5:
							get = &Song::GetGenre;
							set = &Song::SetGenre;
							break;
						case 6:
							get = &Song::GetComposer;
							set = &Song::SetComposer;
							break;
						case 7:
							get = &Song::GetPerformer;
							set = &Song::SetPerformer;
							break;
						case 8:
							get = &Song::GetDisc;
							set = &Song::SetDisc;
							break;
						case 9:
							get = &Song::GetComment;
							set = &Song::SetComment;
							break;
						case 10:
						{
							if (wCurrent == mEditorTagTypes)
							{
								current_screen = csOther;
								__deal_with_filenames(list);
								current_screen = csTagEditor;
								redraw_screen = 1;
								REFRESH_TAG_EDITOR_SCREEN;
							}
							else if (wCurrent == mEditorTags)
							{
								Song &s = mEditorTags->Current();
								string old_name = s.GetNewName().empty() ? s.GetName() : s.GetNewName();
								int last_dot = old_name.find_last_of(".");
								string extension = old_name.substr(last_dot);
								old_name = old_name.substr(0, last_dot);
								LockStatusbar();
								Statusbar() << fmtBold << "New filename: " << fmtBoldEnd;
								string new_name = wFooter->GetString(old_name);
								UnlockStatusbar();
								if (!new_name.empty() && new_name != old_name)
									s.SetNewName(new_name + extension);
								mEditorTags->Scroll(wDown);
							}
							continue;
						}
						case 11: // reset
						{
							mEditorTags->Clear(0);
							ShowMessage("Changes reset");
							continue;
						}
						case 12: // save
						{
							bool success = 1;
							ShowMessage("Writing changes...");
							for (SongList::iterator it = list.begin(); it != list.end(); it++)
							{
								ShowMessage("Writing tags in '%s'...", (*it)->GetName().c_str());
								if (!WriteTags(**it))
								{
									ShowMessage("Error writing tags in '%s'!", (*it)->GetFile().c_str());
									success = 0;
									break;
								}
							}
							if (success)
							{
								ShowMessage("Tags updated!");
								mEditorTagTypes->HighlightColor(Config.main_highlight_color);
								mEditorTagTypes->Reset();
								wCurrent->Refresh();
								wCurrent = mEditorLeftCol;
								mEditorLeftCol->HighlightColor(Config.active_column_color);
								Mpd->UpdateDirectory(FindSharedDir(mEditorTags));
							}
							else
								mEditorTags->Clear(0);
							continue;
						}
						default:
							break;
					}
					
					if (wCurrent == mEditorTagTypes && id != 0 && id != 4 && set != NULL)
					{
						LockStatusbar();
						Statusbar() << fmtBold << mEditorTagTypes->Current() << fmtBoldEnd << ": ";
						mEditorTags->Current().GetEmptyFields(1);
						string new_tag = wFooter->GetString((mEditorTags->Current().*get)());
						mEditorTags->Current().GetEmptyFields(0);
						UnlockStatusbar();
						for (SongList::iterator it = list.begin(); it != list.end(); it++)
							(**it.*set)(new_tag);
						redraw_screen = 1;
					}
					else if (wCurrent == mEditorTags && set != NULL)
					{
						LockStatusbar();
						Statusbar() << fmtBold << mEditorTagTypes->Current() << fmtBoldEnd << ": ";
						mEditorTags->Current().GetEmptyFields(1);
						string new_tag = wFooter->GetString((mEditorTags->Current().*get)());
						mEditorTags->Current().GetEmptyFields(0);
						UnlockStatusbar();
						if (new_tag != (mEditorTags->Current().*get)())
							(mEditorTags->Current().*set)(new_tag);
						mEditorTags->Scroll(wDown);
					}
				}
#				endif // HAVE_TAGLIB_H
				default:
					break;
			}
		}
		else if (Keypressed(input, Key.Space))
		{
			if (Config.space_selects || wCurrent == mPlaylist || wCurrent == mEditorTags)
			{
				if (wCurrent == mPlaylist || wCurrent == mEditorTags || (wCurrent == mBrowser && ((Menu<Song> *)wCurrent)->Choice() >= (browsed_dir != "/" ? 1 : 0)) || (wCurrent == mSearcher && !mSearcher->Current().first) || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
				{
					List *mList = (Menu<Song> *)wCurrent;
					int i = mList->Choice();
					mList->Select(i, !mList->isSelected(i));
					wCurrent->Scroll(wDown);
				}
			}
			else
			{
				if (current_screen == csBrowser)
				{
					const Item &item = mBrowser->Current();
					switch (item.type)
					{
						case itDirectory:
						{
							if (browsed_dir != "/" && !mBrowser->Choice())
								continue; // do not let add parent dir.
							
							SongList list;
							Mpd->GetDirectoryRecursive(item.name, list);
							
							for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
								Mpd->QueueAddSong(**it);
							if (Mpd->CommitQueue())
							{
								ShowMessage("Added folder: %s", item.name.c_str());
								Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
								if (s.GetHash() != list[0]->GetHash())
									ShowMessage("%s", message_part_of_songs_added);
							}
							FreeSongList(list);
							break;
						}
						case itSong:
						{
							block_item_list_update = 1;
							if (Config.ncmpc_like_songs_adding && mBrowser->isBold())
							{
								block_playlist_update = 1;
								long long hash = mBrowser->Current().song->GetHash();
								for (size_t i = 0; i < mPlaylist->Size(); i++)
								{
									if (mPlaylist->at(i).GetHash() == hash)
									{
										Mpd->QueueDeleteSong(i);
										mPlaylist->DeleteOption(i);
										i--;
									}
								}
								Mpd->CommitQueue();
								mBrowser->BoldOption(mBrowser->Choice(), 0);
							}
							else
							{
								Song &s = *item.song;
								if (Mpd->AddSong(s) != -1)
								{
									ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
									mBrowser->BoldOption(mBrowser->Choice(), 1);
								}
							}
							break;
						}
						case itPlaylist:
						{
							SongList list;
							Mpd->GetPlaylistContent(item.name, list);
							for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
								Mpd->QueueAddSong(**it);
							if (Mpd->CommitQueue())
							{
								ShowMessage("Loading playlist %s...", item.name.c_str());
								Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
								if (s.GetHash() != list[0]->GetHash())
									ShowMessage("%s", message_part_of_songs_added);
							}
							FreeSongList(list);
							break;
						}
					}
					mBrowser->Scroll(wDown);
				}
				else if (current_screen == csSearcher && !mSearcher->Current().first)
				{
					block_item_list_update = 1;
					if (Config.ncmpc_like_songs_adding && mSearcher->isBold())
					{
						block_playlist_update = 1;
						long long hash = mSearcher->Current().second->GetHash();
						for (size_t i = 0; i < mPlaylist->Size(); i++)
						{
							if (mPlaylist->at(i).GetHash() == hash)
							{
								Mpd->QueueDeleteSong(i);
								mPlaylist->DeleteOption(i);
								i--;
							}
						}
						Mpd->CommitQueue();
						mSearcher->BoldOption(mSearcher->Choice(), 0);
					}
					else
					{
						const Song &s = *mSearcher->Current().second;
						if (Mpd->AddSong(s) != -1)
						{
							ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
							mSearcher->BoldOption(mSearcher->Choice(), 1);
						}
					}
					mSearcher->Scroll(wDown);
				}
				else if (current_screen == csLibrary)
					goto ENTER_LIBRARY_SCREEN; // sorry, but that's stupid to copy the same code here.
				else if (current_screen == csPlaylistEditor)
					goto ENTER_PLAYLIST_EDITOR_SCREEN; // same what in library screen.
#				ifdef HAVE_TAGLIB_H
				else if (wCurrent == mEditorLeftCol)
				{
					Config.albums_in_tag_editor = !Config.albums_in_tag_editor;
					mEditorLeftCol = Config.albums_in_tag_editor ? mEditorAlbums : mEditorDirs;
					wCurrent = mEditorLeftCol;
					ShowMessage("Switched to %s view", Config.albums_in_tag_editor ? "albums" : "directories");
					mEditorLeftCol->Display();
					mEditorTags->Clear(0);
					redraw_screen = 1;
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
					wCurrent = mLibAlbums;
					mLibAlbums->HighlightColor(Config.active_column_color);
					if (!mLibAlbums->Empty())
						continue;
				}
				if (wCurrent == mLibAlbums && !mLibSongs->Empty())
				{
					mLibAlbums->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = mLibSongs;
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
					wCurrent = mPlaylistEditor;
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
					wCurrent = mEditorTagTypes;
					mEditorTagTypes->HighlightColor(Config.active_column_color);
				}
				else if (wCurrent == mEditorTagTypes && mEditorTagTypes->Choice() < 12 && !mEditorTags->Empty())
				{
					mEditorTagTypes->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = mEditorTags;
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
					wCurrent = mLibAlbums;
					mLibAlbums->HighlightColor(Config.active_column_color);
					if (!mLibAlbums->Empty())
						continue;
				}
				if (wCurrent == mLibAlbums)
				{
					mLibAlbums->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = mLibArtists;
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
					wCurrent = mPlaylistList;
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
					wCurrent = mEditorTagTypes;
					mEditorTagTypes->HighlightColor(Config.active_column_color);
				}
				else if (wCurrent == mEditorTagTypes)
				{
					mEditorTagTypes->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = mEditorLeftCol;
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
					redraw_screen = 1;
				}
				else
				{
					dont_change_now_playing = 1;
					mPlaylist->SetTimeout(50);
					while (!mPlaylist->Empty() && Keypressed(input, Key.Delete))
					{
						int id = mPlaylist->Choice();
						TraceMpdStatus();
						timer = time(NULL);
						if (now_playing > id)  // needed for keeping proper
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
				const string &name = wCurrent == mBrowser ? mBrowser->Current().name : mPlaylistList->Current();
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
						Mpd->DeletePlaylist(name);
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
					for (vector<size_t>::const_reverse_iterator it = list.rbegin(); it != ((const vector<size_t> &)list).rend(); it++)
					{
						Mpd->QueueDeleteFromPlaylist(mPlaylistList->Current(), *it);
						mPlaylistEditor->DeleteOption(*it);
					}
					ShowMessage("Selected items deleted from playlist '%s'!", mPlaylistList->Current().c_str());
					redraw_screen = 1;
				}
				else
				{
					mPlaylistEditor->SetTimeout(50);
					while (!mPlaylistEditor->Empty() && Keypressed(input, Key.Delete))
					{
						TraceMpdStatus();
						timer = time(NULL);
						Mpd->QueueDeleteFromPlaylist(mPlaylistList->Current(), mPlaylistEditor->Choice());
						mPlaylistEditor->DeleteOption(mPlaylistEditor->Choice());
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
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
			UnlockStatusbar();
			if (playlist_name.find("/") != string::npos)
			{
				ShowMessage("Playlist name cannot contain slashes!");
				continue;
			}
			if (!playlist_name.empty())
			{
				if (Mpd->SavePlaylist(playlist_name))
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
						Mpd->DeletePlaylist(playlist_name);
						if (Mpd->SavePlaylist(playlist_name))
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
			if (current_screen == csPlaylist)
			{
				block_playlist_update = 1;
				mPlaylist->SetTimeout(50);
				if (mPlaylist->hasSelected())
				{
					vector<size_t> list;
					mPlaylist->GetSelected(list);
					
					for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						if (*it == now_playing && list.front() > 0)
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
					int from, to;
					from = to = mPlaylist->Choice();
					// unbold now playing as if song changes during move, this won't be unbolded.
					if (to == now_playing && to > 0)
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
			else if (wCurrent == mPlaylistEditor)
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
					int from, to;
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
			if (current_screen == csPlaylist)
			{
				block_playlist_update = 1;
				mPlaylist->SetTimeout(50);
				if (mPlaylist->hasSelected())
				{
					vector<size_t> list;
					mPlaylist->GetSelected(list);
					
					for (vector<size_t>::iterator it = list.begin(); it != list.end(); it++)
						if (*it == now_playing && list.back() < mPlaylist->Size()-1)
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
					if (to == now_playing && to < mPlaylist->Size()-1)
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
			else if (wCurrent == mPlaylistEditor)
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
		else if (Keypressed(input, Key.TogglePlaylistDisplayMode) && wCurrent == mPlaylist)
		{
			Config.columns_in_playlist = !Config.columns_in_playlist;
			ShowMessage("Playlist display mode: %s", Config.columns_in_playlist ? "Columns" : "Classic");
			mPlaylist->SetItemDisplayer(Config.columns_in_playlist ? DisplaySongInColumns : DisplaySong);
			mPlaylist->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
			mPlaylist->SetTitle(Config.columns_in_playlist ? DisplayColumns(Config.song_columns_list_format) : "");
			redraw_screen = 1;
		}
		else if (Keypressed(input, Key.ToggleAutoCenter))
		{
			Config.autocenter_mode = !Config.autocenter_mode;
			ShowMessage("Auto center mode: %s", Config.autocenter_mode ? "On" : "Off");
		}
		else if (Keypressed(input, Key.UpdateDB))
		{
			if (current_screen == csBrowser)
				Mpd->UpdateDirectory(browsed_dir);
			else if (current_screen == csTagEditor && !Config.albums_in_tag_editor)
				Mpd->UpdateDirectory(editor_browsed_dir);
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
				Statusbar() << fmtBold << Config.media_lib_primary_tag << fmtBoldEnd << ": ";
				string new_tag = wFooter->GetString(mLibArtists->Current());
				UnlockStatusbar();
				if (!new_tag.empty() && new_tag != mLibArtists->Current())
				{
					bool success = 1;
					SongList list;
					ShowMessage("Updating tags...");
					Mpd->StartSearch(1);
					Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
					Mpd->CommitSearch(list);
					SongSetFunction set = IntoSetFunction(Config.media_lib_primary_tag);
					if (!set)
						continue;
					for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
					{
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
					SongList list;
					ShowMessage("Updating tags...");
					Mpd->StartSearch(1);
					Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, mLibAlbums->Current().second);
					Mpd->CommitSearch(list);
					for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
					{
						ShowMessage("Updating tags in '%s'...", (*it)->GetName().c_str());
						string path = Config.mpd_music_dir + (*it)->GetFile();
						TagLib::FileRef f(path.c_str());
						if (f.isNull())
						{
							ShowMessage("Error updating tags in '%s'!", (*it)->GetFile().c_str());
							success = 0;
							break;
						}
						f.tag()->setAlbum(TO_WSTRING(new_album));
						f.save();
					}
					if (success)
					{
						Mpd->UpdateDirectory(FindSharedDir(list));
						ShowMessage("Tags updated succesfully!");
					}
					FreeSongList(list);
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
				int id = mList->Choice();
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
					string full_old_dir = Config.mpd_music_dir + editor_browsed_dir + "/" + old_dir;
					string full_new_dir = Config.mpd_music_dir + editor_browsed_dir + "/" + new_dir;
					if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
					{
						Mpd->UpdateDirectory(editor_browsed_dir);
						ShowMessage("'%s' renamed to '%s'", old_dir.c_str(), new_dir.c_str());
					}
					else
						ShowMessage("Cannot rename '%s' to '%s'!", full_old_dir.c_str(), full_new_dir.c_str());
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
					full_old_dir += old_dir;
					string full_new_dir;
					if (!Config.local_browser)
						full_new_dir += Config.mpd_music_dir;
					full_new_dir += new_dir;
					if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
					{
						ShowMessage("'%s' renamed to '%s'", old_dir.c_str(), new_dir.c_str());
						if (!Config.local_browser)
							Mpd->UpdateDirectory(FindSharedDir(old_dir, new_dir));
						GetDirectory(browsed_dir);
					}
					else
						ShowMessage("Cannot rename '%s' to '%s'!", full_old_dir.c_str(), full_new_dir.c_str());
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
					Mpd->Rename(old_name, new_name);
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
			|| (wCurrent == mSearcher && !mSearcher->Current().first)
			|| (wCurrent == mLibSongs && !mLibSongs->Empty())
			|| (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			|| (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				int id = ((Menu<Song> *)wCurrent)->Choice();
				Song *s;
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
					case csTagEditor:
						s = &mEditorTags->at(id);
						break;
					default:
						break;
				}
				
				if (s->GetDirectory() == EMPTY_TAG) // for streams
					continue;
				
				Config.local_browser = !s->IsFromDB();
				
				string option = s->toString(Config.song_status_format);
				GetDirectory(s->GetDirectory());
				for (size_t i = 0; i < mBrowser->Size(); i++)
				{
					if (mBrowser->at(i).type == itSong && option == mBrowser->at(i).song->toString(Config.song_status_format))
					{
						mBrowser->Highlight(i);
						break;
					}
				}
				goto SWITCHER_BROWSER_REDIRECT;
			}
		}
		else if (Keypressed(input, Key.StartSearching))
		{
			if (wCurrent == mSearcher)
			{
				mSearcher->Highlight(13); // highlight 'search' button
				goto ENTER_SEARCH_ENGINE_SCREEN;
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
			if (wCurrent == mPlaylist || wCurrent == mBrowser || (wCurrent == mSearcher && !mSearcher->Current().first) || wCurrent == mLibSongs || wCurrent == mPlaylistEditor || wCurrent == mEditorTags)
			{
				
				List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
				for (size_t i = 0; i < mList->Size(); i++)
					mList->Select(i, !mList->isSelected(i) && !mList->isStatic(i));
				// hackish shit begins
				if (wCurrent == mBrowser && browsed_dir != "/")
					mList->Select(0, 0); // [..] cannot be selected, uhm.
				if (wCurrent == mSearcher)
					mList->Select(14, 0); // 'Reset' cannot be selected, omgplz.
				// hacking shit ends. need better solution :/
				ShowMessage("Selection reversed!");
			}
		}
		else if (Keypressed(input, Key.DeselectAll))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || wCurrent == mSearcher || wCurrent == mLibSongs || wCurrent == mPlaylistEditor || wCurrent == mEditorTags)
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
			if (wCurrent == mPlaylist || wCurrent == mBrowser || wCurrent == mSearcher || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
			{
				List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
				if (mList->hasSelected())
				{
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
										if (browsed_dir != "/")
										Mpd->GetDirectoryRecursive(browsed_dir + "/" + item.name, result);
										else
										Mpd->GetDirectoryRecursive(item.name, result);
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
										Mpd->GetPlaylistContent(item.name, result);
										break;
									}
								}
								break;
							}
							case csSearcher:
							{
								Song *s = mSearcher->at(*it).second;
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
					
					const int dialog_width = COLS*0.8;
					const int dialog_height = LINES*0.6;
					Menu<string> *mDialog = new Menu<string>((COLS-dialog_width)/2, (LINES-dialog_height)/2, dialog_width, dialog_height, "Add selected items to...", Config.main_color, Config.window_border);
					mDialog->SetTimeout(ncmpcpp_window_timeout);
					mDialog->SetItemDisplayer(GenericDisplayer);
					
					mDialog->AddOption("Current MPD playlist");
					mDialog->AddOption("Create new playlist (m3u file)");
					mDialog->AddSeparator();
					TagList playlists;
					Mpd->GetPlaylists(playlists);
					for (TagList::const_iterator it = playlists.begin(); it != playlists.end(); it++)
						mDialog->AddOption("'" + *it + "' playlist");
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
					
					size_t id = mDialog->Choice();
					
					redraw_screen = 1;
					if (current_screen == csLibrary)
					{
						REFRESH_MEDIA_LIBRARY_SCREEN;
					}
					else if (current_screen == csPlaylistEditor)
					{
						REFRESH_PLAYLIST_EDITOR_SCREEN;
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
						UnlockStatusbar();
						if (!playlist.empty())
						{
							for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
								Mpd->QueueAddToPlaylist(playlist, **it);
							Mpd->CommitQueue();
							ShowMessage("Selected items added to playlist '%s'!", playlist.c_str());
						}
						
					}
					else if (id > 1 && id < mDialog->Size()-1)
					{
						for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
							Mpd->QueueAddToPlaylist(playlists[id-3], **it);
						Mpd->CommitQueue();
						ShowMessage("Selected items added to playlist '%s'!", playlists[id-3].c_str());
					}
					
					if (id != mDialog->Size()-1)
					{
						// refresh playlist's lists
						if (!Config.local_browser && browsed_dir == "/")
							GetDirectory("/");
						mPlaylistList->Clear(0); // make playlist editor update itself
					}
					current_screen = prev_screen;
					timer = time(NULL);
					delete mDialog;
					FreeSongList(result);
				}
				else
					ShowMessage("No selected items!");
			}
		}
		else if (Keypressed(input, Key.Crop))
		{
			if (mPlaylist->hasSelected())
			{
				for (size_t i = 0; i < mPlaylist->Size(); i++)
				{
					if (!mPlaylist->isSelected(i) && i != now_playing)
						Mpd->QueueDeleteSongId(mPlaylist->at(i).GetID());
				}
				// if mpd deletes now playing song deletion will be sluggishly slow
				// then so we have to assure it will be deleted at the very end.
				if (!mPlaylist->isSelected(now_playing))
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
			if ((current_screen != csHelp && current_screen != csSearcher)
			||  (current_screen == csSearcher && !mSearcher->Current().first))
			{
				string how = Keypressed(input, Key.FindForward) ? "forward" : "backward";
				found_pos = -1;
				vFoundPositions.clear();
				LockStatusbar();
				Statusbar() << "Find " << how << ": ";
				string findme = wFooter->GetString();
				UnlockStatusbar();
				timer = time(NULL);
				if (findme.empty())
					continue;
				ToLower(findme);
				
				ShowMessage("Searching...");
				List *mList = reinterpret_cast<Menu<Song> *>(wCurrent);
				for (size_t i = (wCurrent == mSearcher ? search_engine_static_options-1 : 0); i < mList->Size(); i++)
				{
					string name;
					switch (current_screen)
					{
						case csPlaylist:
							name = mPlaylist->at(i).toString(Config.columns_in_playlist ? Config.song_columns_list_format : Config.song_list_format);
							break;
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
			if (wCurrent == sInfo)
			{
				wCurrent->Hide();
				current_screen = prev_screen;
				wCurrent = wPrev;
				redraw_screen = 1;
				redraw_header = 1;
				if (current_screen == csLibrary)
				{
					REFRESH_MEDIA_LIBRARY_SCREEN;
				}
				else if (current_screen == csPlaylistEditor)
				{
					REFRESH_PLAYLIST_EDITOR_SCREEN;
				}
#				ifdef HAVE_TAGLIB_H
				else if (current_screen == csTagEditor)
				{
					REFRESH_TAG_EDITOR_SCREEN;
				}
#				endif // HAVE_TAGLIB_H
			}
			else if (
			    (wCurrent == mPlaylist && !mPlaylist->Empty())
			||  (wCurrent == mBrowser && mBrowser->Current().type == itSong)
			||  (wCurrent == mSearcher && !mSearcher->Current().first)
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			||  (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				Song *s;
				int id = ((Menu<Song> *)wCurrent)->Choice();
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csBrowser:
						s = mBrowser->at(id).song;
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
					case csTagEditor:
						s = &mEditorTags->at(id);
						break;
					default:
						break;
				}
				wPrev = wCurrent;
				wCurrent = sInfo;
				prev_screen = current_screen;
				current_screen = csInfo;
				redraw_header = 1;
				info_title = "Song info";
				sInfo->Clear();
				GetInfo(*s, *sInfo);
				sInfo->Flush();
				sInfo->Hide();
			}
		}
#		ifdef HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.ArtistInfo))
		{
			if (wCurrent == sInfo)
			{
				wCurrent->Hide();
				current_screen = prev_screen;
				wCurrent = wPrev;
				redraw_screen = 1;
				redraw_header = 1;
				if (current_screen == csLibrary)
				{
					REFRESH_MEDIA_LIBRARY_SCREEN;
				}
				else if (current_screen == csPlaylistEditor)
				{
					REFRESH_PLAYLIST_EDITOR_SCREEN;
				}
#				ifdef HAVE_TAGLIB_H
				else if (current_screen == csTagEditor)
				{
					REFRESH_TAG_EDITOR_SCREEN;
				}
#				endif // HAVE_TAGLIB_H
			}
			else if (
			    (wCurrent == mPlaylist && !mPlaylist->Empty())
			||  (wCurrent == mBrowser && mBrowser->Current().type == itSong)
			||  (wCurrent == mSearcher && !mSearcher->Current().first)
			||  (wCurrent == mLibArtists && !mLibArtists->Empty())
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			||  (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				string *artist = new string();
				int id = ((Menu<Song> *)wCurrent)->Choice();
				switch (current_screen)
				{
					case csPlaylist:
						*artist = mPlaylist->at(id).GetArtist();
						break;
					case csBrowser:
						*artist = mBrowser->at(id).song->GetArtist();
						break;
					case csSearcher:
						*artist = mSearcher->at(id).second->GetArtist();
						break;
					case csLibrary:
						*artist = mLibArtists->at(id);
						break;
					case csPlaylistEditor:
						*artist = mPlaylistEditor->at(id).GetArtist();
						break;
					case csTagEditor:
						*artist = mEditorTags->at(id).GetArtist();
						break;
					default:
						break;
				}
				if (*artist != EMPTY_TAG)
				{
					wPrev = wCurrent;
					wCurrent = sInfo;
					prev_screen = current_screen;
					current_screen = csInfo;
					redraw_header = 1;
					info_title = "Artist's info - " + *artist;
					sInfo->Clear();
					sInfo->WriteXY(0, 0, 0, "Fetching artist's info...");
					sInfo->Refresh();
					if (!artist_info_downloader)
					{
						pthread_create(&artist_info_downloader, &attr_detached, GetArtistInfo, artist);
					}
				}
				else
					delete artist;
			}
		}
#		endif // HAVE_CURL_CURL_H
		else if (Keypressed(input, Key.Lyrics))
		{
			if (wCurrent == sLyrics)
			{
				wCurrent->Hide();
				current_screen = prev_screen;
				wCurrent = wPrev;
				redraw_screen = 1;
				redraw_header = 1;
				if (current_screen == csLibrary)
				{
					REFRESH_MEDIA_LIBRARY_SCREEN;
				}
				else if (current_screen == csPlaylistEditor)
				{
					REFRESH_PLAYLIST_EDITOR_SCREEN;
				}
#				ifdef HAVE_TAGLIB_H
				else if (current_screen == csTagEditor)
				{
					REFRESH_TAG_EDITOR_SCREEN;
				}
#				endif // HAVE_TAGLIB_H
			}
			else if (
			    (wCurrent == mPlaylist && !mPlaylist->Empty())
			||  (wCurrent == mBrowser && mBrowser->Current().type == itSong)
			||  (wCurrent == mSearcher && !mSearcher->Current().first)
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			||  (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				LOAD_LYRICS:
				
				Song *s;
				int id;
				
				if (reload_lyrics)
				{
					current_screen = csPlaylist;
					wCurrent = mPlaylist;
					reload_lyrics = 0;
					id = now_playing;
				}
				else
					id = ((Menu<Song> *)wCurrent)->Choice();
				
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csBrowser:
						s = mBrowser->at(id).song;
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
					case csTagEditor:
						s = &mEditorTags->at(id);
						break;
					default:
						break;
				}
				if (s->GetArtist() != EMPTY_TAG && s->GetTitle() != EMPTY_TAG)
				{
					wPrev = wCurrent;
					prev_screen = current_screen;
					wCurrent = sLyrics;
					redraw_header = 1;
					wCurrent->Clear();
					current_screen = csLyrics;
					lyrics_title = "Lyrics: " + s->GetArtist() + " - " + s->GetTitle();
					sLyrics->WriteXY(0, 0, 0, "Fetching lyrics...");
					sLyrics->Refresh();
#					ifdef HAVE_CURL_CURL_H
					if (!lyrics_downloader)
					{
						pthread_create(&lyrics_downloader, &attr_detached, GetLyrics, s);
					}
#					else
					GetLyrics(s);
#					endif
				}
			}
		}
		else if (Keypressed(input, Key.Help))
		{
			if (current_screen != csHelp && current_screen != csTinyTagEditor)
			{
				wCurrent = sHelp;
				wCurrent->Hide();
				current_screen = csHelp;
				redraw_header = 1;
			}
		}
		else if (Keypressed(input, Key.ScreenSwitcher))
		{
			if (current_screen == csPlaylist)
				goto SWITCHER_BROWSER_REDIRECT;
			else
				goto SWITCHER_PLAYLIST_REDIRECT;
		}
		else if (Keypressed(input, Key.Playlist))
		{
			SWITCHER_PLAYLIST_REDIRECT:
			if (current_screen != csPlaylist && current_screen != csTinyTagEditor)
			{
				CLEAR_FIND_HISTORY;
				wCurrent = mPlaylist;
				wCurrent->Hide();
				current_screen = csPlaylist;
				redraw_screen = 1;
				redraw_header = 1;
			}
		}
		else if (Keypressed(input, Key.Browser))
		{
			SWITCHER_BROWSER_REDIRECT:
			if (current_screen != csBrowser && current_screen != csTinyTagEditor)
			{
				CLEAR_FIND_HISTORY;
				mBrowser->Empty() ? GetDirectory(browsed_dir) : UpdateItemList(mBrowser);
				wCurrent = mBrowser;
				wCurrent->Hide();
				current_screen = csBrowser;
				redraw_screen = 1;
				redraw_header = 1;
			}
		}
		else if (Keypressed(input, Key.SearchEngine))
		{
			if (current_screen != csSearcher && current_screen != csTinyTagEditor)
			{
				CLEAR_FIND_HISTORY;
				if (mSearcher->Empty())
					PrepareSearchEngine(sought_pattern);
				wCurrent = mSearcher;
				wCurrent->Hide();
				current_screen = csSearcher;
				redraw_screen = 1;
				redraw_header = 1;
				if (!mSearcher->Back().first)
				{
					wCurrent->WriteXY(0, 0, 0, "Updating list...");
					UpdateFoundList();
				}
			}
		}
		else if (Keypressed(input, Key.MediaLibrary))
		{
			if (current_screen != csLibrary && current_screen != csTinyTagEditor)
			{
				CLEAR_FIND_HISTORY;
				
				mLibArtists->HighlightColor(Config.active_column_color);
				mLibAlbums->HighlightColor(Config.main_highlight_color);
				mLibSongs->HighlightColor(Config.main_highlight_color);
				
				mPlaylist->Hide(); // hack, should be wCurrent, but it doesn't always have 100% width
				
				redraw_screen = 1;
				redraw_header = 1;
				REFRESH_MEDIA_LIBRARY_SCREEN;
				
				wCurrent = mLibArtists;
				current_screen = csLibrary;
				
				UpdateSongList(mLibSongs);
			}
		}
		else if (Keypressed(input, Key.PlaylistEditor))
		{
			if (current_screen != csPlaylistEditor && current_screen != csTinyTagEditor)
			{
				CLEAR_FIND_HISTORY;
				
				mPlaylistList->HighlightColor(Config.active_column_color);
				mPlaylistEditor->HighlightColor(Config.main_highlight_color);
				
				mPlaylist->Hide(); // hack, should be wCurrent, but it doesn't always have 100% width
				
				redraw_screen = 1;
				redraw_header = 1;
				REFRESH_PLAYLIST_EDITOR_SCREEN;
				
				wCurrent = mPlaylistList;
				current_screen = csPlaylistEditor;
				
				UpdateSongList(mPlaylistEditor);
			}
		}
#		ifdef HAVE_TAGLIB_H
		else if (Keypressed(input, Key.TagEditor))
		{
			CHECK_MPD_MUSIC_DIR;
			if (current_screen != csTagEditor && current_screen != csTinyTagEditor)
			{
				CLEAR_FIND_HISTORY;
				
				mEditorAlbums->HighlightColor(Config.active_column_color);
				mEditorDirs->HighlightColor(Config.active_column_color);
				mEditorTagTypes->HighlightColor(Config.main_highlight_color);
				mEditorTags->HighlightColor(Config.main_highlight_color);
				
				mPlaylist->Hide(); // hack, should be wCurrent, but it doesn't always have 100% width
				
				redraw_screen = 1;
				redraw_header = 1;
				REFRESH_TAG_EDITOR_SCREEN;
				
				if (mEditorTagTypes->Empty())
				{
					mEditorTagTypes->AddOption("Title");
					mEditorTagTypes->AddOption("Artist");
					mEditorTagTypes->AddOption("Album");
					mEditorTagTypes->AddOption("Year");
					mEditorTagTypes->AddOption("Track");
					mEditorTagTypes->AddOption("Genre");
					mEditorTagTypes->AddOption("Composer");
					mEditorTagTypes->AddOption("Performer");
					mEditorTagTypes->AddOption("Disc");
					mEditorTagTypes->AddOption("Comment");
					mEditorTagTypes->AddSeparator();
					mEditorTagTypes->AddOption("Filename");
					mEditorTagTypes->AddSeparator();
					mEditorTagTypes->AddOption("Options", 1, 1, 0);
					mEditorTagTypes->AddSeparator();
					mEditorTagTypes->AddOption("Reset");
					mEditorTagTypes->AddOption("Save");
					/*mEditorTagTypes->AddSeparator();
					mEditorTagTypes->AddOption("Capitalize first letters");*/
				}
				
				wCurrent = mEditorLeftCol;
				current_screen = csTagEditor;
			}
		}
#		endif // HAVE_TAGLIB_H
		else if (Keypressed(input, Key.Quit))
			main_exit = 1;
		
		// key mapping end
	}
	Mpd->Disconnect();
	DestroyScreen();
	return 0;
}

