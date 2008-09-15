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

#include "mpdpp.h"
#include "ncmpcpp.h"

#include "help.h"
#include "helpers.h"
#include "lyrics.h"
#include "search_engine.h"
#include "settings.h"
#include "song.h"
#include "status_checker.h"
#include "tag_editor.h"

#define REFRESH_MEDIA_LIBRARY_SCREEN \
			mLibArtists->Display(redraw_screen); \
			mvvline(main_start_y, middle_col_startx-1, 0, main_height); \
			mLibAlbums->Display(redraw_screen); \
			mvvline(main_start_y, right_col_startx-1, 0, main_height); \
			mLibSongs->Display(redraw_screen)

#define REFRESH_PLAYLIST_EDITOR_SCREEN \
			mPlaylistList->Display(redraw_screen); \
			mvvline(main_start_y, middle_col_startx-1, 0, main_height); \
			mPlaylistEditor->Display(redraw_screen)

#ifdef HAVE_TAGLIB_H
# define REFRESH_TAG_EDITOR_SCREEN \
			mEditorLeftCol->Display(redraw_screen); \
			mvvline(main_start_y, middle_col_startx-1, 0, main_height); \
			mEditorTagTypes->Display(redraw_screen); \
			mvvline(main_start_y, right_col_startx-1, 0, main_height); \
			mEditorTags->Display(redraw_screen)
#endif // HAVE_TAGLIB_H

ncmpcpp_config Config;
ncmpcpp_keys Key;

vector<int> vFoundPositions;
int found_pos = 0;

Window *wCurrent = 0;
Window *wPrev = 0;

Menu<Song> *mPlaylist;
Menu<Item> *mBrowser;
Menu< std::pair<string, Song> > *mSearcher;

Menu<string> *mLibArtists;
Menu<StringPair> *mLibAlbums;
Menu<Song> *mLibSongs;

#ifdef HAVE_TAGLIB_H
Menu<string> *mTagEditor;
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

MPDConnection *Mpd;

time_t timer;

int now_playing = -1;
int playing_song_scroll_begin = 0;
int browsed_dir_scroll_begin = 0;
int stats_scroll_begin = 0;

int lock_statusbar_delay = -1;

string browsed_dir = "/";
string editor_browsed_dir = "/";
string editor_highlighted_dir;

NcmpcppScreen current_screen;
NcmpcppScreen prev_screen;

bool dont_change_now_playing = 0;
bool block_progressbar_update = 0;
bool block_statusbar_update = 0;
bool allow_statusbar_unlock = 1;
bool block_playlist_update = 0;
bool block_item_list_update = 0;

bool messages_allowed = 0;
bool redraw_screen = 0;
bool redraw_header = 1;

extern bool header_update_status;
extern bool search_case_sensitive;
extern bool search_match_to_pattern;

extern string EMPTY_TAG;
extern string UNKNOWN_ARTIST;
extern string UNKNOWN_TITLE;
extern string UNKNOWN_ALBUM;
extern string playlist_stats;
extern string volume_state;

const string message_part_of_songs_added = "Only part of requested songs' list added to playlist!";

int main(int argc, char *argv[])
{
	DefaultConfiguration(Config);
	DefaultKeys(Key);
	ReadConfiguration(Config);
	ReadKeys(Key);
	DefineEmptyTags();
	
	Mpd = new MPDConnection;
	
	if (getenv("MPD_HOST"))
		Mpd->SetHostname(getenv("MPD_HOST"));
	if (getenv("MPD_PORT"))
		Mpd->SetPort(atoi(getenv("MPD_PORT")));
	if (getenv("MPD_PASSWORD"))
		Mpd->SetPassword(getenv("MPD_PASSWORD"));
	
	Mpd->SetTimeout(Config.mpd_connection_timeout);
	
	if (!Mpd->Connect())
	{
		printf("Cannot connect to mpd: %s\n", Mpd->GetLastErrorMessage().c_str());
		return -1;
	}
	
	setlocale(LC_ALL,"");
	initscr();
	noecho();
	cbreak();
	curs_set(0);
	
	if (Config.colors_enabled)
		Window::EnableColors();
	
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
	mPlaylist->AutoRefresh(0);
	mPlaylist->SetTimeout(ncmpcpp_window_timeout);
	mPlaylist->HighlightColor(Config.main_highlight_color);
	mPlaylist->SetSelectPrefix(Config.selected_item_prefix);
	mPlaylist->SetSelectSuffix(Config.selected_item_suffix);
	mPlaylist->SetItemDisplayer(Config.columns_in_playlist ? DisplaySongInColumns : DisplaySong);
	mPlaylist->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
	
	mBrowser = new Menu<Item>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mBrowser->HighlightColor(Config.main_highlight_color);
	mBrowser->SetTimeout(ncmpcpp_window_timeout);
	mBrowser->SetSelectPrefix(Config.selected_item_prefix);
	mBrowser->SetSelectSuffix(Config.selected_item_suffix);
	mBrowser->SetItemDisplayer(DisplayItem);
	
	mSearcher = new Menu< std::pair<string, Song> >(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mSearcher->HighlightColor(Config.main_highlight_color);
	mSearcher->SetTimeout(ncmpcpp_window_timeout);
	mSearcher->SetItemDisplayer(SearchEngineDisplayer);
	mSearcher->SetSelectPrefix(Config.selected_item_prefix);
	mSearcher->SetSelectSuffix(Config.selected_item_suffix);
	
	int left_col_width = COLS/3-1;
	int middle_col_width = COLS/3;
	int middle_col_startx = left_col_width+1;
	int right_col_width = COLS-COLS/3*2-1;
	int right_col_startx = left_col_width+middle_col_width+2;
	
	mLibArtists = new Menu<string>(0, main_start_y, left_col_width, main_height, "Artists", Config.main_color, brNone);
	mLibArtists->HighlightColor(Config.main_highlight_color);
	mLibArtists->SetTimeout(ncmpcpp_window_timeout);
	
	mLibAlbums = new Menu<StringPair>(middle_col_startx, main_start_y, middle_col_width, main_height, "Albums", Config.main_color, brNone);
	mLibAlbums->HighlightColor(Config.main_highlight_color);
	mLibAlbums->SetTimeout(ncmpcpp_window_timeout);
	mLibAlbums->SetItemDisplayer(DisplayStringPair);
	
	mLibSongs = new Menu<Song>(right_col_startx, main_start_y, right_col_width, main_height, "Songs", Config.main_color, brNone);
	mLibSongs->HighlightColor(Config.main_highlight_color);
	mLibSongs->SetTimeout(ncmpcpp_window_timeout);
	mLibSongs->SetSelectPrefix(Config.selected_item_prefix);
	mLibSongs->SetSelectSuffix(Config.selected_item_suffix);
	mLibSongs->SetItemDisplayer(DisplaySong);
	mLibSongs->SetItemDisplayerUserData(&Config.song_library_format);
	
#	ifdef HAVE_TAGLIB_H
	mTagEditor = new Menu<string>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mTagEditor->HighlightColor(Config.main_highlight_color);
	mTagEditor->SetTimeout(ncmpcpp_window_timeout);
	
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
	
	mEditorTags = new Menu<Song>(right_col_startx, main_start_y, right_col_width, main_height, "Tags", Config.main_color, brNone);
	mEditorTags->HighlightColor(Config.main_highlight_color);
	mEditorTags->SetTimeout(ncmpcpp_window_timeout);
	mEditorTags->SetSelectPrefix(Config.selected_item_prefix);
	mEditorTags->SetSelectSuffix(Config.selected_item_suffix);
	mEditorTags->SetItemDisplayer(DisplayTag);
	mEditorTags->SetItemDisplayerUserData(mEditorTagTypes);
#	endif // HAVE_TAGLIB_H
	
	mPlaylistList = new Menu<string>(0, main_start_y, left_col_width, main_height, "Playlists", Config.main_color, brNone);
	mPlaylistList->HighlightColor(Config.main_highlight_color);
	mPlaylistList->SetTimeout(ncmpcpp_window_timeout);
	
	mPlaylistEditor = new Menu<Song>(middle_col_startx, main_start_y, middle_col_width+right_col_width+1, main_height, "Playlist's content", Config.main_color, brNone);
	mPlaylistEditor->HighlightColor(Config.main_highlight_color);
	mPlaylistEditor->SetTimeout(ncmpcpp_window_timeout);
	mPlaylistEditor->SetSelectPrefix(Config.selected_item_prefix);
	mPlaylistEditor->SetSelectSuffix(Config.selected_item_suffix);
	mPlaylistEditor->SetItemDisplayer(DisplaySong);
	mPlaylistEditor->SetItemDisplayerUserData(&Config.song_list_format);
	
	sHelp = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	sHelp->SetTimeout(ncmpcpp_window_timeout);
	sHelp->Add(GetKeybindings());
	
	sLyrics = static_cast<Scrollpad *>(sHelp->EmptyClone());
	sLyrics->SetTimeout(ncmpcpp_window_timeout);
	
	sInfo = static_cast<Scrollpad *>(sHelp->EmptyClone());
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
	bool title_allowed = 0;
	
	string lyrics_title;
	string info_title;
	// local variables end
	
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
		
		const int max_allowed_title_length = wHeader->GetWidth()-volume_state.length();
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
			}
			
			if (title_allowed)
			{
				wHeader->Bold(1);
				wHeader->WriteXY(0, 0, max_allowed_title_length, title, 1);
				wHeader->Bold(0);
				
				if (current_screen == csPlaylist && !playlist_stats.empty())
					wHeader->WriteXY(title.length(), 0, max_allowed_title_length-title.length(), playlist_stats);
				else if (current_screen == csBrowser)
				{
					int max_length_without_scroll = wHeader->GetWidth()-volume_state.length()-title.length();
					my_string_t wbrowseddir = TO_WSTRING(browsed_dir);
					wHeader->Bold(1);
					if (browsed_dir.length() > max_length_without_scroll)
					{
#						ifdef UTF8_ENABLED
						wbrowseddir += L" ** ";
#						else
						wbrowseddir += " ** ";
#						endif
						const int scrollsize = max_length_without_scroll;
						my_string_t part = wbrowseddir.substr(browsed_dir_scroll_begin++, scrollsize);
						if (part.length() < scrollsize)
							part += wbrowseddir.substr(0, scrollsize-part.length());
						wHeader->WriteXY(title.length(), 0, part);
						if (browsed_dir_scroll_begin >= wbrowseddir.length())
							browsed_dir_scroll_begin = 0;
					}
					else
						wHeader->WriteXY(title.length(), 0, browsed_dir);
					wHeader->Bold(0);
				}
			}
			else
			{
				string screens = "[.b]1:[/b]Help  [.b]2:[/b]Playlist  [.b]3:[/b]Browse  [.b]4:[/b]Search  [.b]5:[/b]Library  [.b]6:[/b]Playlist editor";
#				ifdef HAVE_TAGLIB_H
				screens += "  [.b]7:[/b]Tag editor";
#				endif // HAVE_TAGLIB_H
				wHeader->WriteXY(0, 0, max_allowed_title_length, screens, 1);
			}
			
			wHeader->SetColor(Config.volume_color);
			wHeader->WriteXY(max_allowed_title_length, 0, volume_state);
			wHeader->SetColor(Config.header_color);
			redraw_header = 0;
		}
		
		// header stuff end
		
		// media library stuff
		
		if (current_screen == csLibrary)
		{
			if (mLibArtists->Empty())
			{
				found_pos = 0;
				vFoundPositions.clear();
				TagList list;
				mLibAlbums->Clear(0);
				mLibSongs->Clear(0);
				Mpd->GetArtists(list);
				sort(list.begin(), list.end(), CaseInsensitiveSorting());
				for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
					mLibArtists->AddOption(*it);
				mLibArtists->Window::Clear();
				mLibArtists->Refresh();
			}
			
			if (mLibAlbums->Empty() && mLibSongs->Empty())
			{
				mLibAlbums->Reset();
				TagList list;
				std::map<string, string, CaseInsensitiveSorting> maplist;
				Mpd->GetAlbums(mLibArtists->GetOption(), list);
				for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					bool written = 0;
					SongList l;
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetOption());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, *it);
					Mpd->CommitSearch(l);
					for (SongList::const_iterator j = l.begin(); j != l.end(); j++)
					{
						if ((*j)->GetYear() != EMPTY_TAG)
						{
							maplist["(" + (*j)->GetYear() + ") " + *it] = *it;
							written = 1;
							break;
						}
					}
					if (!written)
						maplist[*it] = *it;
					FreeSongList(l);
				}
				for (std::map<string, string>::const_iterator it = maplist.begin(); it != maplist.end(); it++)
					mLibAlbums->AddOption(StringPair(it->first, it->second));
				mLibAlbums->Window::Clear();
				mLibAlbums->Refresh();
			}
			
			if (wCurrent == mLibAlbums && mLibAlbums->Empty())
			{
				mLibAlbums->HighlightColor(Config.main_highlight_color);
				mLibArtists->HighlightColor(Config.active_column_color);
				wCurrent = mLibArtists;
			}
			
			if (mLibSongs->Empty())
			{
				mLibSongs->Reset();
				SongList list;
				if (mLibAlbums->Empty())
				{
					mLibAlbums->WriteXY(0, 0, "No albums found.");
					mLibSongs->Clear(0);
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetOption());
					Mpd->CommitSearch(list);
				}
				else
				{
					mLibSongs->Clear(0);
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetOption());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, mLibAlbums->Current().second);
					Mpd->CommitSearch(list);
				}
				sort(list.begin(), list.end(), SortSongsByTrack);
				bool bold = 0;
			
				for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					for (int j = 0; j < mPlaylist->Size(); j++)
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
		
		// playlist editor stuff
		
		if (current_screen == csPlaylistEditor)
		{
			if (mPlaylistList->Empty())
			{
				mPlaylistEditor->Clear(0);
				TagList list;
				Mpd->GetPlaylists(list);
				for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
					mPlaylistList->AddOption(*it);
				mPlaylistList->Window::Clear();
				mPlaylistList->Refresh();
			}
		
			if (mPlaylistEditor->Empty())
			{
				mPlaylistEditor->Reset();
				SongList list;
				Mpd->GetPlaylistContent(mPlaylistList->GetOption(), list);
				if (!list.empty())
					mPlaylistEditor->SetTitle("Playlist's content (" + IntoStr(list.size()) + " item" + (list.size() == 1 ? ")" : "s)"));
				else
					mPlaylistEditor->SetTitle("Playlist's content");
				bool bold = 0;
				for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					for (int j = 0; j < mPlaylist->Size(); j++)
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
				mPlaylistEditor->WriteXY(0, 0, "Playlist is empty.");
		}
		
		// playlist editor end
		
		// album editor stuff
#		ifdef HAVE_TAGLIB_H
		if (current_screen == csTagEditor)
		{
			if (mEditorLeftCol->Empty())
			{
				found_pos = 0;
				vFoundPositions.clear();
				mEditorLeftCol->Window::Clear();
				mEditorTags->Clear();
				TagList list;
				if (Config.albums_in_tag_editor)
				{
					std::map<string, string, CaseInsensitiveSorting> maplist;
					mEditorAlbums->WriteXY(0, 0, "Fetching albums' list...");
					Mpd->GetAlbums("", list);
					for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
					{
						bool written = 0;
						SongList l;
						Mpd->StartSearch(1);
						Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, *it);
						Mpd->CommitSearch(l);
						maplist[DisplaySong(*l[0], &Config.tag_editor_album_format)] = *it;
						FreeSongList(l);
					}
					for (std::map<string, string>::const_iterator it = maplist.begin(); it != maplist.end(); it++)
						mEditorAlbums->AddOption(StringPair(it->first, it->second));
				}
				else
				{
					int highlightme = -1;
					Mpd->GetDirectories(editor_browsed_dir, list);
					sort(list.begin(), list.end(), CaseInsensitiveSorting());
					if (editor_browsed_dir != "/")
					{
						int slash = editor_browsed_dir.find_last_of("/");
						string parent = slash != string::npos ? editor_browsed_dir.substr(0, slash) : "/";
						mEditorDirs->AddOption(StringPair("[..]", parent));
					}
					for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
					{
						int slash = it->find_last_of("/");
						string to_display = slash != string::npos ? it->substr(slash+1) : *it;
						mEditorDirs->AddOption(StringPair(to_display, *it));
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
			
			if (redraw_screen && wCurrent == mEditorTagTypes && mEditorTagTypes->GetChoice() < 10)
			{
				mEditorTags->Refresh(1);
				redraw_screen = 0;
			}
			else if (mEditorTagTypes->GetChoice() >= 10)
				mEditorTags->Window::Clear();
		}
#		endif // HAVE_TAGLIB_H
		// album editor end
		
		if (Config.columns_in_playlist && wCurrent == mPlaylist)
			wCurrent->Display(redraw_screen);
		else
			wCurrent->Refresh(redraw_screen);
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
					wCurrent->Go(wUp);
					wCurrent->Refresh();
					wCurrent->ReadKey(input);
				}
				wCurrent->SetTimeout(ncmpcpp_window_timeout);
			}
			else
				wCurrent->Go(wUp);
		}
		else if (Keypressed(input, Key.Down))
		{
			if (!Config.fancy_scrolling && (wCurrent == mLibArtists || wCurrent == mPlaylistList || wCurrent == mEditorLeftCol))
			{
				wCurrent->SetTimeout(50);
				while (Keypressed(input, Key.Down))
				{
					wCurrent->Go(wDown);
					wCurrent->Refresh();
					wCurrent->ReadKey(input);
				}
				wCurrent->SetTimeout(ncmpcpp_window_timeout);
			}
			else
				wCurrent->Go(wDown);
		}
		else if (Keypressed(input, Key.PageUp))
		{
			wCurrent->Go(wPageUp);
		}
		else if (Keypressed(input, Key.PageDown))
		{
			wCurrent->Go(wPageDown);
		}
		else if (Keypressed(input, Key.Home))
		{
			wCurrent->Go(wHome);
		}
		else if (Keypressed(input, Key.End))
		{
			wCurrent->Go(wEnd);
		}
		else if (input == KEY_RESIZE)
		{
			redraw_screen = 1;
			redraw_header = 1;
			
			if (COLS < 20 || LINES < 5)
			{
				endwin();
				printf("Screen too small!\n");
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
			MPDStatusChanges changes;
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
				goto GO_TO_PARENT_DIR;
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
					GO_TO_PARENT_DIR:
					
					const Item &item = mBrowser->Current();
					switch (item.type)
					{
						case itDirectory:
						{
							found_pos = 0;
							vFoundPositions.clear();
							GetDirectory(item.name, browsed_dir);
							redraw_header = 1;
							break;
						}
						case itSong:
						{
							block_item_list_update = 1;
							Song &s = *item.song;
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								Mpd->PlayID(id);
								ShowMessage("Added to playlist: " + DisplaySong(s, &Config.song_status_format));
								mBrowser->BoldOption(mBrowser->GetChoice(), 1);
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
								ShowMessage("Loading and playing playlist " + item.name + "...");
								Song *s = &mPlaylist->at(mPlaylist->Size()-list.size());
								if (s->GetHash() == list[0]->GetHash())
									Mpd->PlayID(s->GetID());
								else
									ShowMessage(message_part_of_songs_added);
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
					int id = mTagEditor->GetRealChoice()+1;
					int option = mTagEditor->GetChoice();
					LockStatusbar();
					Song &s = edited_song;
					
					switch (id)
					{
						case 1:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Title:[/b] ", 1);
							if (s.GetTitle() == UNKNOWN_TITLE)
								s.SetTitle(wFooter->GetString());
							else
								s.SetTitle(wFooter->GetString(s.GetTitle()));
							mTagEditor->UpdateOption(option, "[.b]Title:[/b] " + s.GetTitle());
							break;
						}
						case 2:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Artist:[/b] ", 1);
							if (s.GetArtist() == UNKNOWN_ARTIST)
								s.SetArtist(wFooter->GetString());
							else
								s.SetArtist(wFooter->GetString(s.GetArtist()));
							mTagEditor->UpdateOption(option, "[.b]Artist:[/b] " + s.GetArtist());
							break;
						}
						case 3:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Album:[/b] ", 1);
							if (s.GetAlbum() == UNKNOWN_ALBUM)
								s.SetAlbum(wFooter->GetString());
							else
								s.SetAlbum(wFooter->GetString(s.GetAlbum()));
							mTagEditor->UpdateOption(option, "[.b]Album:[/b] " + s.GetAlbum());
							break;
						}
						case 4:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Year:[/b] ", 1);
							if (s.GetYear() == EMPTY_TAG)
								s.SetYear(wFooter->GetString(4));
							else
								s.SetYear(wFooter->GetString(s.GetYear(), 4));
							mTagEditor->UpdateOption(option, "[.b]Year:[/b] " + s.GetYear());
							break;
						}
						case 5:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Track:[/b] ", 1);
							if (s.GetTrack() == EMPTY_TAG)
								s.SetTrack(wFooter->GetString(3));
							else
								s.SetTrack(wFooter->GetString(s.GetTrack(), 3));
							mTagEditor->UpdateOption(option, "[.b]Track:[/b] " + s.GetTrack());
							break;
						}
						case 6:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Genre:[/b] ", 1);
							if (s.GetGenre() == EMPTY_TAG)
								s.SetGenre(wFooter->GetString());
							else
								s.SetGenre(wFooter->GetString(s.GetGenre()));
							mTagEditor->UpdateOption(option, "[.b]Genre:[/b] " + s.GetGenre());
							break;
						}
						case 7:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Comment:[/b] ", 1);
							if (s.GetComment() == EMPTY_TAG)
								s.SetComment(wFooter->GetString());
							else
								s.SetComment(wFooter->GetString(s.GetComment()));
							mTagEditor->UpdateOption(option, "[.b]Comment:[/b] " + s.GetComment());
							break;
						}
						case 8:
						{
							ShowMessage("Updating tags...");
							if (WriteTags(s))
							{
								ShowMessage("Tags updated!");
								Mpd->UpdateDirectory(s.GetDirectory());
								if (prev_screen == csSearcher)
									mSearcher->Current().second = s;
							}
								ShowMessage("Error writing tags!");
						}
						case 9:
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
					
					int option = mSearcher->GetChoice();
					LockStatusbar();
					Song &s = sought_pattern;
					
					switch (option+1)
					{
						case 1:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Filename:[/b] ", 1);
							if (s.GetShortFilename() == EMPTY_TAG)
								s.SetShortFilename(wFooter->GetString());
							else
								s.SetShortFilename(wFooter->GetString(s.GetShortFilename()));
							mSearcher->Current().first = "[.b]Filename:[/b] " + s.GetShortFilename();
							break;
						}
						case 2:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Title:[/b] ", 1);
							if (s.GetTitle() == UNKNOWN_TITLE)
								s.SetTitle(wFooter->GetString());
							else
								s.SetTitle(wFooter->GetString(s.GetTitle()));
							mSearcher->Current().first = "[.b]Title:[/b] " + s.GetTitle();
							break;
						}
						case 3:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Artist:[/b] ", 1);
							if (s.GetArtist() == UNKNOWN_ARTIST)
								s.SetArtist(wFooter->GetString());
							else
								s.SetArtist(wFooter->GetString(s.GetArtist()));
							mSearcher->Current().first = "[.b]Artist:[/b] " + s.GetArtist();
							break;
						}
						case 4:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Album:[/b] ", 1);
							if (s.GetAlbum() == UNKNOWN_ALBUM)
								s.SetAlbum(wFooter->GetString());
							else
								s.SetAlbum(wFooter->GetString(s.GetAlbum()));
							mSearcher->Current().first = "[.b]Album:[/b] " + s.GetAlbum();
							break;
						}
						case 5:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Year:[/b] ", 1);
							if (s.GetYear() == EMPTY_TAG)
								s.SetYear(wFooter->GetString(4));
							else
								s.SetYear(wFooter->GetString(s.GetYear(), 4));
							mSearcher->Current().first = "[.b]Year:[/b] " + s.GetYear();
							break;
						}
						case 6:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Track:[/b] ", 1);
							if (s.GetTrack() == EMPTY_TAG)
								s.SetTrack(wFooter->GetString(3));
							else
								s.SetTrack(wFooter->GetString(s.GetTrack(), 3));
							mSearcher->Current().first = "[.b]Track:[/b] " + s.GetTrack();
							break;
						}
						case 7:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Genre:[/b] ", 1);
							if (s.GetGenre() == EMPTY_TAG)
								s.SetGenre(wFooter->GetString());
							else
								s.SetGenre(wFooter->GetString(s.GetGenre()));
							mSearcher->Current().first = "[.b]Genre:[/b] " + s.GetGenre();
							break;
						}
						case 8:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Comment:[/b] ", 1);
							if (s.GetComment() == EMPTY_TAG)
								s.SetComment(wFooter->GetString());
							else
								s.SetComment(wFooter->GetString(s.GetComment()));
							mSearcher->Current().first = "[.b]Comment:[/b] " + s.GetComment();
							break;
						}
						case 10:
						{
							search_match_to_pattern = !search_match_to_pattern;
							mSearcher->Current().first = "[.b]Search mode:[/b] " + (search_match_to_pattern ? search_mode_normal : search_mode_strict);
							break;
						}
						case 11:
						{
							search_case_sensitive = !search_case_sensitive;
							mSearcher->Current().first = "[.b]Case sensitive:[/b] " + string(search_case_sensitive ? "Yes" : "No");
							break;
						}
						case 13:
						{
							ShowMessage("Searching...");
							Search(s);
							if (mSearcher->Back().first == ".")
							{
								bool bold = 0;
								int found = mSearcher->Size()-search_engine_static_options;
								mSearcher->InsertSeparator(14);
								mSearcher->Insert(15, std::pair<string, Song>("[." + Config.color1 + "]Search results:[/" + Config.color1 + "] [." + Config.color2 + "]Found " + IntoStr(found) + (found > 1 ? " songs" : " song") + "[/" + Config.color2 + "]", Song()), 1, 1);
								mSearcher->InsertSeparator(16);
								UpdateFoundList();
								ShowMessage("Searching finished!");
								for (int i = 0; i < 13; i++)
									mSearcher->MakeStatic(i, 1);
								mSearcher->Go(wDown);
								mSearcher->Go(wDown);
							}
							else
								ShowMessage("No results found");
							break;
						}
						case 14:
						{
							found_pos = 0;
							vFoundPositions.clear();
							PrepareSearchEngine(sought_pattern);
							ShowMessage("Search state reset");
							break;
						}
						default:
						{
							block_item_list_update = 1;
							const Song &s = mSearcher->Current().second;
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								Mpd->PlayID(id);
								ShowMessage("Added to playlist: " + DisplaySong(s, &Config.song_status_format));
								mSearcher->BoldOption(mSearcher->GetChoice(), 1);
							}
							break;
						}
					}
					if (option <= 10)
						mSearcher->RefreshOption(option);
					UnlockStatusbar();
					break;
				}
				case csLibrary:
				{
					ENTER_LIBRARY_SCREEN: // same code for Key.Space, but without playing.
					
					SongList list;
					
					if (wCurrent == mLibArtists)
					{
						const string &artist = mLibArtists->GetOption();
						Mpd->StartSearch(1);
						Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, artist);
						Mpd->CommitSearch(list);
						for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
							Mpd->QueueAddSong(**it);
						if (Mpd->CommitQueue())
						{
							ShowMessage("Adding all songs artist's: " + artist);
							Song *s = &mPlaylist->at(mPlaylist->Size()-list.size());
							if (s->GetHash() == list[0]->GetHash())
							{
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(s->GetID());
							}
							else
								ShowMessage(message_part_of_songs_added);
						}
					}
					else if (wCurrent == mLibAlbums)
					{
						for (int i = 0; i < mLibSongs->Size(); i++)
							Mpd->QueueAddSong(mLibSongs->at(i));
						if (Mpd->CommitQueue())
						{
							ShowMessage("Adding songs from: " + mLibArtists->GetOption() + " \"" + mLibAlbums->Current().second + "\"");
							Song *s = &mPlaylist->at(mPlaylist->Size()-mLibSongs->Size());
							if (s->GetHash() == mLibSongs->at(0).GetHash())
							{
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(s->GetID());
							}
							else
								ShowMessage(message_part_of_songs_added);
						}
					}
					else if (wCurrent == mLibSongs)
					{
						if (!mLibSongs->Empty())
						{
							block_item_list_update = 1;
							Song &s = mLibSongs->Current();
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								ShowMessage("Added to playlist: " + DisplaySong(s, &Config.song_status_format));
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(id);
								mLibSongs->BoldOption(mLibSongs->GetChoice(), 1);
							}
						}
					}
					FreeSongList(list);
					if (Keypressed(input, Key.Space))
					{
						wCurrent->Go(wDown);
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
						const string &playlist = mPlaylistList->GetOption();
						Mpd->GetPlaylistContent(playlist, list);
						for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
							Mpd->QueueAddSong(**it);
						if (Mpd->CommitQueue())
						{
							ShowMessage("Loading playlist " + playlist + "...");
							Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
							if (s.GetHash() == list[0]->GetHash())
							{
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(s.GetID());
							}
							else
								ShowMessage(message_part_of_songs_added);
						}
					}
					else if (wCurrent == mPlaylistEditor)
					{
						if (!mPlaylistEditor->Empty())
						{
							block_item_list_update = 1;
							Song &s = mPlaylistEditor->at(mPlaylistEditor->GetChoice());
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								ShowMessage("Added to playlist: " + DisplaySong(s, &Config.song_status_format));
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(id);
								mPlaylistEditor->BoldOption(mPlaylistEditor->GetChoice(), 1);
							}
						}
					}
					FreeSongList(list);
					if (Keypressed(input, Key.Space))
						wCurrent->Go(wDown);
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
					if (mEditorTags->IsAnySelected())
					{
						vector<int> selected;
						mEditorTags->GetSelectedList(selected);
						for (vector<int>::const_iterator it = selected.begin(); it != selected.end(); it++)
							list.push_back(&mEditorTags->at(*it));
					}
					else
						for (int i = 0; i < mEditorTags->Size(); i++)
							list.push_back(&mEditorTags->at(i));
					
					SongSetFunction set = 0;
					int id = mEditorTagTypes->GetRealChoice();
					switch (id)
					{
						case 0:
							set = &Song::SetTitle;
							break;
						case 1:
							set = &Song::SetArtist;
							break;
						case 2:
							set = &Song::SetAlbum;
							break;
						case 3:
							set = &Song::SetYear;
							break;
						case 4:
							set = &Song::SetTrack;
							if (wCurrent == mEditorTagTypes)
							{
								LockStatusbar();
								wFooter->WriteXY(0, Config.statusbar_visibility, "Number tracks? [y/n] ", 1);
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
								UnlockStatusbar();
							}
							break;
						case 5:
							set = &Song::SetGenre;
							break;
						case 6:
							set = &Song::SetComment;
							break;
						case 7:
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
								string old_name = s.GetNewName().empty() ? s.GetShortFilename() : s.GetNewName();
								int last_dot = old_name.find_last_of(".");
								string extension = old_name.substr(last_dot);
								old_name = old_name.substr(0, last_dot);
								LockStatusbar();
								wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]New filename:[/b] ", 1);
								string new_name = wFooter->GetString(old_name);
								UnlockStatusbar();
								if (!new_name.empty() && new_name != old_name)
									s.SetNewName(new_name + extension);
								mEditorTags->Go(wDown);
							}
							continue;
						}
						case 8: // reset
						{
							mEditorTags->Clear(0);
							ShowMessage("Changes reset");
							continue;
						}
						case 9: // save
						{
							bool success = 1;
							ShowMessage("Writing changes...");
							for (SongList::iterator it = list.begin(); it != list.end(); it++)
							{
								if (!WriteTags(**it))
								{
									ShowMessage("Error writing tags!");
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
						wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]" + mEditorTagTypes->GetOption() + "[/b]: ", 1);
						mEditorTags->Current().GetEmptyFields(1);
						string new_tag = wFooter->GetString(mEditorTags->GetOption());
						mEditorTags->Current().GetEmptyFields(0);
						UnlockStatusbar();
						for (SongList::iterator it = list.begin(); it != list.end(); it++)
							(**it.*set)(new_tag);
					}
					else if (wCurrent == mEditorTags && set != NULL)
					{
						LockStatusbar();
						wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]" + mEditorTagTypes->GetOption() + "[/b]: ", 1);
						mEditorTags->Current().GetEmptyFields(1);
						string new_tag = wFooter->GetString(mEditorTags->GetOption());
						mEditorTags->Current().GetEmptyFields(0);
						UnlockStatusbar();
						if (new_tag != mEditorTags->GetOption())
							(mEditorTags->Current().*set)(new_tag);
						mEditorTags->Go(wDown);
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
				if (wCurrent == mPlaylist || wCurrent == mEditorTags || (wCurrent == mBrowser && wCurrent->GetChoice() >= (browsed_dir != "/" ? 1 : 0)) || (wCurrent == mSearcher && mSearcher->Current().first == ".") || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
				{
					int i = wCurrent->GetChoice();
					wCurrent->Select(i, !wCurrent->Selected(i));
					wCurrent->Go(wDown);
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
							if (browsed_dir != "/" && !mBrowser->GetChoice())
								continue; // do not let add parent dir.
							
							SongList list;
							Mpd->GetDirectoryRecursive(item.name, list);
						
							for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
								Mpd->QueueAddSong(**it);
							if (Mpd->CommitQueue())
							{
								ShowMessage("Added folder: " + item.name);
								Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
								if (s.GetHash() != list[0]->GetHash())
									ShowMessage(message_part_of_songs_added);
							}
							FreeSongList(list);
							break;
						}
						case itSong:
						{
							block_item_list_update = 1;
							Song &s = *item.song;
							if (Mpd->AddSong(s) != -1)
							{
								ShowMessage("Added to playlist: " + DisplaySong(s, &Config.song_status_format));
								mBrowser->BoldOption(mBrowser->GetChoice(), 1);
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
								ShowMessage("Loading playlist " + item.name + "...");
								Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
								if (s.GetHash() != list[0]->GetHash())
									ShowMessage(message_part_of_songs_added);
							}
							FreeSongList(list);
							break;
						}
					}
					mBrowser->Go(wDown);
				}
				else if (current_screen == csSearcher && mSearcher->Current().first == ".")
				{
					block_item_list_update = 1;
					Song &s = mSearcher->Current().second;
					if (Mpd->AddSong(s) != -1)
					{
						ShowMessage("Added to playlist: " + DisplaySong(s, &Config.song_status_format));
						mSearcher->BoldOption(mSearcher->GetChoice(), 1);
					}
					mSearcher->Go(wDown);
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
					ShowMessage("Switched to " + string(Config.albums_in_tag_editor ? "albums" : "directories") + " view");
					mEditorLeftCol->Display();
					mEditorTags->Clear(0);
					redraw_screen = 1;
				}
#				endif // HAVE_TAGLIB_H
			}
		}
		else if (Keypressed(input, Key.VolumeUp))
		{
			if (current_screen == csLibrary && input == Key.VolumeUp[0])
			{
				found_pos = 0;
				vFoundPositions.clear();
				if (wCurrent == mLibArtists)
				{
					mLibArtists->HighlightColor(Config.main_highlight_color);
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
					wCurrent = mLibSongs;
					mLibSongs->HighlightColor(Config.active_column_color);
				}
			}
			else if (wCurrent == mPlaylistList && input == Key.VolumeUp[0])
			{
				found_pos = 0;
				vFoundPositions.clear();
				mPlaylistList->HighlightColor(Config.main_highlight_color);
				wCurrent->Refresh();
				wCurrent = mPlaylistEditor;
				mPlaylistEditor->HighlightColor(Config.active_column_color);
			}
#			ifdef HAVE_TAGLIB_H
			else if (current_screen == csTagEditor && input == Key.VolumeUp[0])
			{
				found_pos = 0;
				vFoundPositions.clear();
				if (wCurrent == mEditorLeftCol)
				{
					mEditorLeftCol->HighlightColor(Config.main_highlight_color);
					wCurrent->Refresh();
					wCurrent = mEditorTagTypes;
					mEditorTagTypes->HighlightColor(Config.active_column_color);
				}
				else if (wCurrent == mEditorTagTypes && mEditorTagTypes->GetChoice() < 10 && !mEditorTags->Empty())
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
				found_pos = 0;
				vFoundPositions.clear();
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
			else if (wCurrent == mPlaylistEditor && input == Key.VolumeDown[0])
			{
				found_pos = 0;
				vFoundPositions.clear();
				mPlaylistEditor->HighlightColor(Config.main_highlight_color);
				wCurrent->Refresh();
				wCurrent = mPlaylistList;
				mPlaylistList->HighlightColor(Config.active_column_color);
			}
#			ifdef HAVE_TAGLIB_H
			else if (current_screen == csTagEditor && input == Key.VolumeDown[0])
			{
				found_pos = 0;
				vFoundPositions.clear();
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
				if (mPlaylist->IsAnySelected())
				{
					vector<int> list;
					mPlaylist->GetSelectedList(list);
					for (vector<int>::const_reverse_iterator it = list.rbegin(); it != ((const vector<int> &)list).rend(); it++)
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
						int id = mPlaylist->GetChoice();
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
					wFooter->WriteXY(0, Config.statusbar_visibility, "Delete playlist " + name + " ? [y/n] ", 1);
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
						ShowMessage("Playlist " + name + " deleted!");
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
				if (mPlaylistEditor->IsAnySelected())
				{
					vector<int> list;
					mPlaylistEditor->GetSelectedList(list);
					for (vector<int>::const_reverse_iterator it = list.rbegin(); it != ((const vector<int> &)list).rend(); it++)
					{
						Mpd->QueueDeleteFromPlaylist(mPlaylistList->GetOption(), *it);
						mPlaylistEditor->DeleteOption(*it);
					}
					ShowMessage("Selected items deleted from playlist '" + mPlaylistList->GetOption() + "'!");
					redraw_screen = 1;
				}
				else
				{
					mPlaylistEditor->SetTimeout(50);
					while (!mPlaylistEditor->Empty() && Keypressed(input, Key.Delete))
					{
						TraceMpdStatus();
						timer = time(NULL);
						Mpd->QueueDeleteFromPlaylist(mPlaylistList->GetOption(), mPlaylistEditor->GetChoice());
						mPlaylistEditor->DeleteOption(mPlaylistEditor->GetChoice());
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
			wFooter->WriteXY(0, Config.statusbar_visibility, "Save playlist as: ", 1);
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
					ShowMessage("Playlist saved as: " + playlist_name);
					mPlaylistList->Clear(0); // make playlist's list update itself
				}
				else
				{
					LockStatusbar();
					wFooter->WriteXY(0, Config.statusbar_visibility, "Playlist already exists, overwrite: " + playlist_name + " ? [y/n] ", 1);
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
			if (browsed_dir == "/" && !mBrowser->Empty())
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
				if (mPlaylist->IsAnySelected())
				{
					vector<int> list;
					mPlaylist->GetSelectedList(list);
					
					for (vector<int>::iterator it = list.begin(); it != list.end(); it++)
						if (*it == now_playing && list.front() > 0)
							mPlaylist->BoldOption(now_playing, 0);
					
					vector<int> origs(list);
					
					while (Keypressed(input, Key.MvSongUp) && list.front() > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<int>::iterator it = list.begin(); it != list.end(); it++)
							mPlaylist->Swap(--*it, *it);
						mPlaylist->Highlight(list[(list.size()-1)/2]);
						mPlaylist->Refresh();
						mPlaylist->ReadKey(input);
					}
					for (int i = 0; i < list.size(); i++)
						Mpd->QueueMove(origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					int from, to;
					from = to = mPlaylist->GetChoice();
					// unbold now playing as if song changes during move, this won't be unbolded.
					if (to == now_playing && to > 0)
						mPlaylist->BoldOption(now_playing, 0);
					while (Keypressed(input, Key.MvSongUp) && to > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						mPlaylist->Swap(to--, to);
						mPlaylist->Go(wUp);
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
				if (mPlaylistEditor->IsAnySelected())
				{
					vector<int> list;
					mPlaylistEditor->GetSelectedList(list);
					
					vector<int> origs(list);
					
					while (Keypressed(input, Key.MvSongUp) && list.front() > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<int>::iterator it = list.begin(); it != list.end(); it++)
							mPlaylistEditor->Swap(--*it, *it);
						mPlaylistEditor->Highlight(list[(list.size()-1)/2]);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					for (int i = 0; i < list.size(); i++)
						if (origs[i] != list[i])
							Mpd->QueueMove(mPlaylistList->GetOption(), origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					int from, to;
					from = to = mPlaylistEditor->GetChoice();
					while (Keypressed(input, Key.MvSongUp) && to > 0)
					{
						TraceMpdStatus();
						timer = time(NULL);
						mPlaylistEditor->Swap(to--, to);
						mPlaylistEditor->Go(wUp);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					if (from != to)
						Mpd->Move(mPlaylistList->GetOption(), from, to);
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
				if (mPlaylist->IsAnySelected())
				{
					vector<int> list;
					mPlaylist->GetSelectedList(list);
					
					for (vector<int>::iterator it = list.begin(); it != list.end(); it++)
						if (*it == now_playing && list.back() < mPlaylist->Size()-1)
							mPlaylist->BoldOption(now_playing, 0);
					
					vector<int> origs(list);
					
					while (Keypressed(input, Key.MvSongDown) && list.back() < mPlaylist->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<int>::reverse_iterator it = list.rbegin(); it != list.rend(); it++)
							mPlaylist->Swap(++*it, *it);
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
					int from, to;
					from = to = mPlaylist->GetChoice();
					// unbold now playing as if song changes during move, this won't be unbolded.
					if (to == now_playing && to < mPlaylist->Size()-1)
						mPlaylist->BoldOption(now_playing, 0);
					while (Keypressed(input, Key.MvSongDown) && to < mPlaylist->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						mPlaylist->Swap(to++, to);
						mPlaylist->Go(wDown);
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
				if (mPlaylistEditor->IsAnySelected())
				{
					vector<int> list;
					mPlaylistEditor->GetSelectedList(list);
					
					vector<int> origs(list);
					
					while (Keypressed(input, Key.MvSongDown) && list.back() < mPlaylistEditor->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						for (vector<int>::reverse_iterator it = list.rbegin(); it != list.rend(); it++)
							mPlaylistEditor->Swap(++*it, *it);
						mPlaylistEditor->Highlight(list[(list.size()-1)/2]);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					for (int i = list.size()-1; i >= 0; i--)
						if (origs[i] != list[i])
							Mpd->QueueMove(mPlaylistList->GetOption(), origs[i], list[i]);
					Mpd->CommitQueue();
				}
				else
				{
					int from, to;
					from = to = mPlaylistEditor->GetChoice();
					while (Keypressed(input, Key.MvSongDown) && to < mPlaylistEditor->Size()-1)
					{
						TraceMpdStatus();
						timer = time(NULL);
						mPlaylistEditor->Swap(to++, to);
						mPlaylistEditor->Go(wDown);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					if (from != to)
						Mpd->Move(mPlaylistList->GetOption(), from, to);
				}
				mPlaylistEditor->SetTimeout(ncmpcpp_window_timeout);
			}
		}
		else if (Keypressed(input, Key.Add))
		{
			LockStatusbar();
			wFooter->WriteXY(0, Config.statusbar_visibility, "Add: ", 1);
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
							ShowMessage(message_part_of_songs_added);
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
			
			int songpos, in;
			
			songpos = Mpd->GetElapsedTime();
			Song &s = mPlaylist->at(now_playing);
			
			while (1)
			{
				TraceMpdStatus();
				timer = time(NULL);
				mPlaylist->ReadKey(in);
				if (Keypressed(in, Key.SeekForward) || Keypressed(in, Key.SeekBackward))
				{
					if (songpos < s.GetTotalLength() && Keypressed(in, Key.SeekForward))
						songpos++;
					if (songpos < s.GetTotalLength() && songpos > 0 && Keypressed(in, Key.SeekBackward))
						songpos--;
					if (songpos < 0)
						songpos = 0;
					
					wFooter->Bold(1);
					string tracklength = "[" + Song::ShowTime(songpos) + "/" + s.GetLength() + "]";
					wFooter->WriteXY(wFooter->GetWidth()-tracklength.length(), 1, tracklength);
					double progressbar_size = (double)songpos/(s.GetTotalLength());
					int howlong = wFooter->GetWidth()*progressbar_size;
					
					mvwhline(wFooter->RawWin(), 0, 0, 0, wFooter->GetWidth());
					mvwhline(wFooter->RawWin(), 0, 0, '=',howlong);
					mvwaddch(wFooter->RawWin(), 0, howlong, '>');
					wFooter->Bold(0);
					wFooter->Refresh();
				}
				else
					break;
			}
			Mpd->Seek(songpos);
			
			block_progressbar_update = 0;
			UnlockStatusbar();
		}
		else if (Keypressed(input, Key.TogglePlaylistDisplayMode) && wCurrent == mPlaylist)
		{
			Config.columns_in_playlist = !Config.columns_in_playlist;
			ShowMessage("Playlist display mode: " + string(Config.columns_in_playlist ? "Columns" : "Classic"));
			mPlaylist->SetItemDisplayer(Config.columns_in_playlist ? DisplaySongInColumns : DisplaySong);
			mPlaylist->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
			mPlaylist->SetTitle(Config.columns_in_playlist ? DisplayColumns(Config.song_columns_list_format) : "");
			redraw_screen = 1;
		}
		else if (Keypressed(input, Key.ToggleAutoCenter))
		{
			Config.autocenter_mode = !Config.autocenter_mode;
			ShowMessage("Auto center mode: " + string(Config.autocenter_mode ? "On" : "Off"));
		}
		else if (Keypressed(input, Key.UpdateDB))
		{
			if (current_screen == csBrowser)
				Mpd->UpdateDirectory(browsed_dir);
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
			ShowMessage("'Repeat one' mode: " + string(Config.repeat_one_mode ? "On" : "Off"));
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
			wFooter->WriteXY(0, Config.statusbar_visibility, "Set crossfade to: ", 1);
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
#			ifdef HAVE_TAGLIB_H
			if (wCurrent == mLibArtists)
			{
				LockStatusbar();
				wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Artist:[/b] ", 1);
				string new_artist = wFooter->GetString(mLibArtists->GetOption());
				UnlockStatusbar();
				if (!new_artist.empty() && new_artist != mLibArtists->GetOption())
				{
					bool success = 1;
					SongList list;
					ShowMessage("Updating tags...");
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetOption());
					Mpd->CommitSearch(list);
					for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
					{
						string path = Config.mpd_music_dir + (*it)->GetFile();
						TagLib::FileRef f(path.c_str());
						if (f.isNull())
						{
							success = 0;
							break;
						}
						f.tag()->setArtist(TO_WSTRING(new_artist));
						f.save();
					}
					if (success)
						Mpd->UpdateDirectory(FindSharedDir(list));
					FreeSongList(list);
					ShowMessage(success ? "Tags written succesfully!" : "Error while writing tags!");
				}
			}
			else if (wCurrent == mLibAlbums)
			{
				LockStatusbar();
				wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Album:[/b] ", 1);
				string new_album = wFooter->GetString(mLibAlbums->Current().second);
				UnlockStatusbar();
				if (!new_album.empty() && new_album != mLibAlbums->Current().second)
				{
					bool success = 1;
					SongList list;
					ShowMessage("Updating tags...");
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetOption());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, mLibAlbums->Current().second);
					Mpd->CommitSearch(list);
					for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
					{
						string path = Config.mpd_music_dir + (*it)->GetFile();
						TagLib::FileRef f(path.c_str());
						if (f.isNull())
						{
							success = 0;
							break;
						}
						f.tag()->setAlbum(TO_WSTRING(new_album));
						f.save();
					}
					if (success)
						Mpd->UpdateDirectory(FindSharedDir(list));
					FreeSongList(list);
					ShowMessage(success ? "Tags written succesfully!" : "Error while writing tags!");
				}
			}
			else if (
			    (wCurrent == mPlaylist && !mPlaylist->Empty())
			||  (wCurrent == mBrowser && mBrowser->Current().type == itSong)
			||  (wCurrent == mSearcher && mSearcher->Current().first == ".")
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			||  (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				int id = wCurrent->GetChoice();
				switch (current_screen)
				{
					case csPlaylist:
						edited_song = mPlaylist->at(id);
						break;
					case csBrowser:
						edited_song = *mBrowser->at(id).song;
						break;
					case csSearcher:
						edited_song = mSearcher->at(id).second;
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
				if (GetSongTags(edited_song))
				{
					wPrev = wCurrent;
					wCurrent = mTagEditor;
					prev_screen = current_screen;
					current_screen = csTinyTagEditor;
					redraw_header = 1;
				}
				else
					ShowMessage("Cannot read file '" + Config.mpd_music_dir + edited_song.GetFile() + "'!");
			}
			else if (wCurrent == mEditorDirs)
			{
				LockStatusbar();
				wFooter->WriteXY(0, Config.statusbar_visibility, "Directory: ", 1);
				string new_dir = wFooter->GetString(mEditorDirs->Current().first);
				UnlockStatusbar();
				if (!new_dir.empty() && new_dir != mEditorDirs->Current().first)
				{
					string old_dir = Config.mpd_music_dir + mEditorDirs->Current().second;
					new_dir = Config.mpd_music_dir + editor_browsed_dir + "/" + new_dir;
					if (rename(old_dir.c_str(), new_dir.c_str()) == 0)
					{
						ShowMessage("'" + mEditorDirs->Current().first + "' renamed to '" + new_dir);
						Mpd->UpdateDirectory(editor_browsed_dir);
					}
					else
						ShowMessage("Cannot rename '" + old_dir + "' to '" + new_dir + "'!");
				}
			}
			else
#			endif // HAVE_TAGLIB_H
			if (wCurrent == mPlaylistList)
			{
				LockStatusbar();
				wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Playlist:[/b] ", 1);
				string new_name = wFooter->GetString(mPlaylistList->GetOption());
				UnlockStatusbar();
				if (!new_name.empty() && new_name != mPlaylistList->GetOption())
				{
					Mpd->Rename(mPlaylistList->GetOption(), new_name);
					ShowMessage("Playlist '" + mPlaylistList->GetOption() + "' renamed to '" + new_name + "'");
					mPlaylistList->Clear(0);
				}
			}
		}
		else if (Keypressed(input, Key.GoToContainingDir))
		{
			if ((wCurrent == mPlaylist && !mPlaylist->Empty())
			|| (wCurrent == mSearcher && mSearcher->Current().first == ".")
			|| (wCurrent == mLibSongs && !mLibSongs->Empty())
			|| (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			|| (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				int id = wCurrent->GetChoice();
				Song *s;
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csSearcher:
						s = &mSearcher->at(id).second;
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
				
				string option = DisplaySong(*s);
				GetDirectory(s->GetDirectory());
				for (int i = 0; i < mBrowser->Size(); i++)
				{
					if (option == mBrowser->GetOption(i))
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
				mSearcher->Highlight(12); // highlight 'search' button
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
			wFooter->WriteXY(0, Config.statusbar_visibility, "Position to go (in %): ", 1);
			string position = wFooter->GetString(3);
			int newpos = StrToInt(position);
			if (newpos > 0 && newpos < 100 && !position.empty())
				Mpd->Seek(mPlaylist->at(now_playing).GetTotalLength()*newpos/100.0);
			UnlockStatusbar();
		}
		else if (Keypressed(input, Key.ReverseSelection))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || (wCurrent == mSearcher && mSearcher->Current().first == ".") || wCurrent == mLibSongs || wCurrent == mPlaylistEditor || wCurrent == mEditorTags)
			{
				for (int i = 0; i < wCurrent->Size(); i++)
					wCurrent->Select(i, !wCurrent->Selected(i) && !wCurrent->IsStatic(i));
				// hackish shit begins
				if (wCurrent == mBrowser && browsed_dir != "/")
					wCurrent->Select(0, 0); // [..] cannot be selected, uhm.
				if (wCurrent == mSearcher)
					wCurrent->Select(13, 0); // 'Reset' cannot be selected, omgplz.
				// hacking shit ends. need better solution :/
				ShowMessage("Selection reversed!");
			}
		}
		else if (Keypressed(input, Key.DeselectAll))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || wCurrent == mSearcher || wCurrent == mLibSongs || wCurrent == mPlaylistEditor || wCurrent == mEditorTags)
			{
				if (wCurrent->IsAnySelected())
				{
					for (int i = 0; i < wCurrent->Size(); i++)
						wCurrent->Select(i, 0);
					ShowMessage("Items deselected!");
				}
			}
		}
		else if (Keypressed(input, Key.AddSelected))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || wCurrent == mSearcher || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
			{
				if (wCurrent->IsAnySelected())
				{
					vector<int> list;
					wCurrent->GetSelectedList(list);
					SongList result;
					for (vector<int>::const_iterator it = list.begin(); it != list.end(); it++)
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
								Song *s = new Song(mSearcher->at(*it).second);
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
							mDialog->Go(wUp);
						else if (Keypressed(input, Key.Down))
							mDialog->Go(wDown);
						else if (Keypressed(input, Key.PageUp))
							mDialog->Go(wPageUp);
						else if (Keypressed(input, Key.PageDown))
							mDialog->Go(wPageDown);
						else if (Keypressed(input, Key.Home))
							mDialog->Go(wHome);
						else if (Keypressed(input, Key.End))
							mDialog->Go(wEnd);
					}
					
					int id = mDialog->GetChoice();
					
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
						wCurrent->Refresh(1);
					
					if (id == 0)
					{
						for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
							Mpd->QueueAddSong(**it);
						if (Mpd->CommitQueue())
						{
							ShowMessage("Selected items added!");
							Song &s = mPlaylist->at(mPlaylist->Size()-result.size());
							if (s.GetHash() != result[0]->GetHash())
								ShowMessage(message_part_of_songs_added);
						}
					}
					else if (id == 1)
					{
						LockStatusbar();
						wFooter->WriteXY(0, Config.statusbar_visibility, "Save playlist as: ", 1);
						string playlist = wFooter->GetString();
						UnlockStatusbar();
						if (!playlist.empty())
						{
							for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
								Mpd->QueueAddToPlaylist(playlist, **it);
							Mpd->CommitQueue();
							ShowMessage("Selected items added to playlist '" + playlist + "'!");
						}
						
					}
					else if (id > 1 && id < mDialog->Size()-1)
					{
						for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
							Mpd->QueueAddToPlaylist(playlists[id-3], **it);
						Mpd->CommitQueue();
						ShowMessage("Selected items added to playlist '" + playlists[id-3] + "'!");
					}
					
					if (id != mDialog->Size()-1)
					{
						// refresh playlist's lists
						if (browsed_dir == "/")
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
			if (mPlaylist->IsAnySelected())
			{
				for (int i = 0; i < mPlaylist->Size(); i++)
				{
					if (!mPlaylist->Selected(i) && i != now_playing)
						Mpd->QueueDeleteSongId(mPlaylist->at(i).GetID());
				}
				// if mpd deletes now playing song deletion will be sluggishly slow
				// then so we have to assure it will be deleted at the very end.
				if (!mPlaylist->Selected(now_playing))
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
				for (int i = now_playing+1; i < mPlaylist->Size(); i++)
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
			||  (current_screen == csSearcher && mSearcher->Current().first == "."))
			{
				string how = Keypressed(input, Key.FindForward) ? "forward" : "backward";
				found_pos = -1;
				vFoundPositions.clear();
				LockStatusbar();
				wFooter->WriteXY(0, Config.statusbar_visibility, "Find " + how + ": ", 1);
				string findme = wFooter->GetString();
				UnlockStatusbar();
				timer = time(NULL);
				if (findme.empty())
					continue;
				transform(findme.begin(), findme.end(), findme.begin(), tolower);
				
				ShowMessage("Searching...");
				for (int i = (wCurrent == mSearcher ? search_engine_static_options-1 : 0); i < wCurrent->Size(); i++)
				{
					string name = Window::OmitBBCodes(wCurrent->GetOption(i));
					transform(name.begin(), name.end(), name.begin(), tolower);
					if (name.find(findme) != string::npos && !wCurrent->IsStatic(i))
					{
						vFoundPositions.push_back(i);
						if (Keypressed(input, Key.FindForward)) // forward
						{
							if (found_pos < 0 && i >= wCurrent->GetChoice())
								found_pos = vFoundPositions.size()-1;
						}
						else // backward
						{
							if (i <= wCurrent->GetChoice())
								found_pos = vFoundPositions.size()-1;
						}
					}
				}
				ShowMessage("Searching finished!");
				
				if (Config.wrapped_search ? vFoundPositions.empty() : found_pos < 0)
					ShowMessage("Unable to find \"" + findme + "\"");
				else
				{
					wCurrent->Highlight(vFoundPositions[found_pos < 0 ? 0 : found_pos]);
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
				try
				{
					wCurrent->Highlight(vFoundPositions.at(Keypressed(input, Key.NextFoundPosition) ? ++found_pos : --found_pos));
				}
				catch (std::out_of_range)
				{
					if (Config.wrapped_search)
					{
						if (Keypressed(input, Key.NextFoundPosition))
						{
							wCurrent->Highlight(vFoundPositions.front());
							found_pos = 0;
						}
						else
						{
							wCurrent->Highlight(vFoundPositions.back());
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
			ShowMessage("Search mode: " + string(Config.wrapped_search ? "Wrapped" : "Normal"));
		}
		else if (Keypressed(input, Key.ToggleSpaceMode))
		{
			Config.space_selects = !Config.space_selects;
			ShowMessage("Space mode: " + string(Config.space_selects ? "Select/deselect" : "Add") + " item");
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
			||  (wCurrent == mSearcher && mSearcher->Current().first == ".")
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			||  (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				Song *s;
				int id = wCurrent->GetChoice();
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csBrowser:
						s = mBrowser->at(id).song;
						break;
					case csSearcher:
						s = &mSearcher->at(id).second;
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
				sInfo->Add(GetInfo(*s));
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
			||  (wCurrent == mSearcher && mSearcher->Current().first == ".")
			||  (wCurrent == mLibArtists && !mLibArtists->Empty())
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			||  (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				string artist;
				int id = wCurrent->GetChoice();
				switch (current_screen)
				{
					case csPlaylist:
						artist = mPlaylist->at(id).GetArtist();
						break;
					case csBrowser:
						artist = mBrowser->at(id).song->GetArtist();
						break;
					case csSearcher:
						artist = mSearcher->at(id).second.GetArtist();
						break;
					case csLibrary:
						artist = mLibArtists->at(id);
						break;
					case csPlaylistEditor:
						artist = mPlaylistEditor->at(id).GetArtist();
						break;
					case csTagEditor:
						artist = mEditorTags->at(id).GetArtist();
						break;
					default:
						break;
				}
				if (artist != UNKNOWN_ARTIST)
				{
					wPrev = wCurrent;
					wCurrent = sInfo;
					prev_screen = current_screen;
					current_screen = csInfo;
					redraw_header = 1;
					info_title = "Artist's info - " + artist;
					sInfo->Clear();
					sInfo->WriteXY(0, 0, "Fetching artist's info...");
					sInfo->Refresh();
					sInfo->Add(GetArtistInfo(artist));
					sInfo->Hide();
				}
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
			||  (wCurrent == mSearcher && mSearcher->Current().first == ".")
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			||  (wCurrent == mEditorTags && !mEditorTags->Empty()))
			{
				Song *s;
				int id = wCurrent->GetChoice();
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csBrowser:
						s = mBrowser->at(id).song;
						break;
					case csSearcher:
						s = &mSearcher->at(id).second;
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
				if (s->GetArtist() != UNKNOWN_ARTIST && s->GetTitle() != UNKNOWN_TITLE)
				{
					wPrev = wCurrent;
					prev_screen = current_screen;
					wCurrent = sLyrics;
					redraw_header = 1;
					wCurrent->Clear();
					current_screen = csLyrics;
					lyrics_title = "Lyrics: " + s->GetArtist() + " - " + s->GetTitle();
					sLyrics->WriteXY(0, 0, "Fetching lyrics...");
					sLyrics->Refresh();
					sLyrics->Add(GetLyrics(s->GetArtist(), s->GetTitle()));
				}
			}
		}
		else if (Keypressed(input, Key.Help))
		{
			if (wCurrent != sHelp)
			{
				wCurrent = sHelp;
				wCurrent->Hide();
				current_screen = csHelp;
				redraw_header = 1;
			}
		}
		else if (Keypressed(input, Key.ScreenSwitcher))
		{
			if (wCurrent == mPlaylist)
				goto SWITCHER_BROWSER_REDIRECT;
			else
				goto SWITCHER_PLAYLIST_REDIRECT;
		}
		else if (Keypressed(input, Key.Playlist))
		{
			SWITCHER_PLAYLIST_REDIRECT:
			if (wCurrent != mPlaylist && current_screen != csTinyTagEditor)
			{
				found_pos = 0;
				vFoundPositions.clear();
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
			if (browsed_dir.empty())
				browsed_dir = "/";
			
			mBrowser->Empty() ? GetDirectory(browsed_dir) : UpdateItemList(mBrowser);
			
			if (wCurrent != mBrowser && current_screen != csTinyTagEditor)
			{
				found_pos = 0;
				vFoundPositions.clear();
				wCurrent = mBrowser;
				wCurrent->Hide();
				current_screen = csBrowser;
				redraw_screen = 1;
				redraw_header = 1;
			}
		}
		else if (Keypressed(input, Key.SearchEngine))
		{
			if (current_screen != csTinyTagEditor && current_screen != csSearcher)
			{
				found_pos = 0;
				vFoundPositions.clear();
				if (mSearcher->Empty())
					PrepareSearchEngine(sought_pattern);
				wCurrent = mSearcher;
				wCurrent->Hide();
				current_screen = csSearcher;
				redraw_screen = 1;
				redraw_header = 1;
				if (mSearcher->Back().first == ".")
				{
					wCurrent->WriteXY(0, 0, "Updating list...");
					UpdateFoundList();
				}
			}
		}
		else if (Keypressed(input, Key.MediaLibrary))
		{
			if (current_screen != csLibrary)
			{
				found_pos = 0;
				vFoundPositions.clear();
				
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
			if (current_screen != csPlaylistEditor)
			{
				found_pos = 0;
				vFoundPositions.clear();
				
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
			if (current_screen != csTagEditor)
			{
				found_pos = 0;
				vFoundPositions.clear();
				
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
					mEditorTagTypes->AddOption("Comment");
					mEditorTagTypes->AddSeparator();
					mEditorTagTypes->AddOption("Filename");
					mEditorTagTypes->AddSeparator();
					mEditorTagTypes->AddOption("Options", 1, 1, 0, lCenter);
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
	curs_set(1);
	endwin();
	printf("\n");
	return 0;
}

