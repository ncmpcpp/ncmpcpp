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

#include "ncmpcpp.h"
#include "mpdpp.h"
#include "status_checker.h"
#include "helpers.h"
#include "settings.h"
#include "song.h"
#include "lyrics.h"

#define LOCK_STATUSBAR \
			if (Config.statusbar_visibility) \
				block_statusbar_update = 1; \
			else \
				block_progressbar_update = 1; \
			allow_statusbar_unblock = 0

#define UNLOCK_STATUSBAR \
			allow_statusbar_unblock = 1; \
			if (block_statusbar_update_delay < 0) \
			{ \
				if (Config.statusbar_visibility) \
					block_statusbar_update = 0; \
				else \
					block_progressbar_update = 0; \
			}

#define REFRESH_MEDIA_LIBRARY_SCREEN \
			mLibArtists->Display(redraw_me); \
			mvvline(main_start_y, lib_albums_start_x-1, 0, main_height); \
			mLibAlbums->Display(redraw_me); \
			mvvline(main_start_y, lib_songs_start_x-1, 0, main_height); \
			mLibSongs->Display(redraw_me)

#define REFRESH_PLAYLIST_EDITOR_SCREEN \
			mPlaylistList->Display(redraw_me); \
			mvvline(main_start_y, lib_albums_start_x-1, 0, main_height); \
			mPlaylistEditor->Display(redraw_me)

#ifdef HAVE_TAGLIB_H
 const string tag_screen = "Tag editor";
 const string tag_screen_keydesc = "Edit song's tags\n";
#else
 const string tag_screen = "Tag info";
 const string tag_screen_keydesc = "Show song's tags\n";
#endif

ncmpcpp_config Config;
ncmpcpp_keys Key;

SongList vSearched;

vector<int> vFoundPositions;
int found_pos = 0;

Window *wCurrent = 0;
Window *wPrev = 0;
Menu<Song> *mPlaylist;
Menu<Item> *mBrowser;
Menu<string> *mTagEditor;
Menu<string> *mSearcher;
Menu<string> *mLibArtists;
Menu<string> *mLibAlbums;
Menu<Song> *mLibSongs;
Menu<string> *mPlaylistList;
Menu<Song> *mPlaylistEditor;
Scrollpad *sHelp;
Scrollpad *sLyrics;

Window *wHeader;
Window *wFooter;

MPDConnection *Mpd;

time_t block_delay;
time_t timer;
time_t now;

int now_playing = -1;
int playing_song_scroll_begin = 0;
int browsed_dir_scroll_begin = 0;
int stats_scroll_begin = 0;

int block_statusbar_update_delay = -1;

string browsed_dir = "/";
string song_lyrics;
string player_state;
string volume_state;
string switch_state;

string mpd_repeat;
string mpd_random;
string mpd_crossfade;
string mpd_db_updating;

NcmpcppScreen current_screen;
NcmpcppScreen prev_screen;

Song edited_song;
Song searched_song;

bool main_exit = 0;
bool messages_allowed = 0;
bool title_allowed = 0;

bool header_update_status = 0;

bool dont_change_now_playing = 0;
bool block_progressbar_update = 0;
bool block_statusbar_update = 0;
bool allow_statusbar_unblock = 1;
bool block_playlist_update = 0;

bool search_case_sensitive = 1;
bool search_mode_match = 1;

bool redraw_me = 0;

extern string EMPTY_TAG;
extern string UNKNOWN_ARTIST;
extern string UNKNOWN_TITLE;
extern string UNKNOWN_ALBUM;

extern string playlist_stats;

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
		printf("Cannot connect to mpd (%s)\n", Mpd->GetLastErrorMessage().c_str());
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
	mPlaylist->SetSelectPrefix(Config.selected_item_prefix);
	mPlaylist->SetSelectSuffix(Config.selected_item_suffix);
	mPlaylist->SetItemDisplayer(Config.columns_in_playlist ? DisplaySongInColumns : DisplaySong);
	mPlaylist->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
	
	mBrowser = new Menu<Item>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mBrowser->SetSelectPrefix(Config.selected_item_prefix);
	mBrowser->SetSelectSuffix(Config.selected_item_suffix);
	mBrowser->SetItemDisplayer(DisplayItem);
	
	mTagEditor = new Menu<string>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mSearcher = static_cast<Menu<string> *>(mTagEditor->Clone());
	
	int lib_artist_width = COLS/3-1;
	int lib_albums_width = COLS/3;
	int lib_albums_start_x = lib_artist_width+1;
	int lib_songs_width = COLS-COLS/3*2-1;
	int lib_songs_start_x = lib_artist_width+lib_albums_width+2;
	
	mLibArtists = new Menu<string>(0, main_start_y, lib_artist_width, main_height, "Artists", Config.main_color, brNone);
	mLibAlbums = new Menu<string>(lib_albums_start_x, main_start_y, lib_albums_width, main_height, "Albums", Config.main_color, brNone);
	mLibSongs = new Menu<Song>(lib_songs_start_x, main_start_y, lib_songs_width, main_height, "Songs", Config.main_color, brNone);
	mLibSongs->SetSelectPrefix(Config.selected_item_prefix);
	mLibSongs->SetSelectSuffix(Config.selected_item_suffix);
	mLibSongs->SetItemDisplayer(DisplaySong);
	mLibSongs->SetItemDisplayerUserData(&Config.song_library_format);
	
	mPlaylistList = new Menu<string>(0, main_start_y, lib_artist_width, main_height, "Playlists", Config.main_color, brNone);
	mPlaylistEditor = new Menu<Song>(lib_albums_start_x, main_start_y, lib_albums_width+lib_songs_width+1, main_height, "Playlist's content", Config.main_color, brNone);
	mPlaylistEditor->SetSelectPrefix(Config.selected_item_prefix);
	mPlaylistEditor->SetSelectSuffix(Config.selected_item_suffix);
	mPlaylistEditor->SetItemDisplayer(DisplaySong);
	mPlaylistEditor->SetItemDisplayerUserData(&Config.song_list_format);
	
	sHelp = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	sLyrics = static_cast<Scrollpad *>(sHelp->EmptyClone());
	
	sHelp->Add("   [.b]Keys - Movement\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Up) + "Move Cursor up\n");
	sHelp->Add(DisplayKeys(Key.Down) + "Move Cursor down\n");
	sHelp->Add(DisplayKeys(Key.PageUp) + "Page up\n");
	sHelp->Add(DisplayKeys(Key.PageDown) + "Page down\n");
	sHelp->Add(DisplayKeys(Key.Home) + "Home\n");
	sHelp->Add(DisplayKeys(Key.End) + "End\n\n");
	
	sHelp->Add(DisplayKeys(Key.ScreenSwitcher) + "Switch between playlist and browser\n");
	sHelp->Add(DisplayKeys(Key.Help) + "Help screen\n");
	sHelp->Add(DisplayKeys(Key.Playlist) + "Playlist screen\n");
	sHelp->Add(DisplayKeys(Key.Browser) + "Browse screen\n");
	sHelp->Add(DisplayKeys(Key.SearchEngine) + "Search engine\n");
	sHelp->Add(DisplayKeys(Key.MediaLibrary) + "Media library\n");
	sHelp->Add(DisplayKeys(Key.PlaylistEditor) + "Playlist editor\n\n\n");
	
	sHelp->Add("   [.b]Keys - Global\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Stop) + "Stop\n");
	sHelp->Add(DisplayKeys(Key.Pause) + "Pause\n");
	sHelp->Add(DisplayKeys(Key.Next) + "Next track\n");
	sHelp->Add(DisplayKeys(Key.Prev) + "Previous track\n");
	sHelp->Add(DisplayKeys(Key.SeekForward) + "Seek forward\n");
	sHelp->Add(DisplayKeys(Key.SeekBackward) + "Seek backward\n");
	sHelp->Add(DisplayKeys(Key.VolumeDown) + "Decrease volume\n");
	sHelp->Add(DisplayKeys(Key.VolumeUp) + "Increase volume\n\n");
	
	sHelp->Add(DisplayKeys(Key.ToggleSpaceMode) + "Toggle space mode (select/add)\n");
	sHelp->Add(DisplayKeys(Key.ReverseSelection) + "Reverse selection\n");
	sHelp->Add(DisplayKeys(Key.DeselectAll) + "Deselect all items\n");
	sHelp->Add(DisplayKeys(Key.AddSelected) + "Add selected items to playlist/m3u file\n\n");
	
	sHelp->Add(DisplayKeys(Key.ToggleRepeat) + "Toggle repeat mode\n");
	sHelp->Add(DisplayKeys(Key.ToggleRepeatOne) + "Toggle \"repeat one\" mode\n");
	sHelp->Add(DisplayKeys(Key.ToggleRandom) + "Toggle random mode\n");
	sHelp->Add(DisplayKeys(Key.Shuffle) + "Shuffle playlist\n");
	sHelp->Add(DisplayKeys(Key.ToggleCrossfade) + "Toggle crossfade mode\n");
	sHelp->Add(DisplayKeys(Key.SetCrossfade) + "Set crossfade\n");
	sHelp->Add(DisplayKeys(Key.UpdateDB) + "Start a music database update\n\n");
	
	sHelp->Add(DisplayKeys(Key.FindForward) + "Forward find\n");
	sHelp->Add(DisplayKeys(Key.FindBackward) + "Backward find\n");
	sHelp->Add(DisplayKeys(Key.PrevFoundPosition) + "Go to previous found position\n");
	sHelp->Add(DisplayKeys(Key.NextFoundPosition) + "Go to next found position\n");
	sHelp->Add(DisplayKeys(Key.ToggleFindMode) + "Toggle find mode (normal/wrapped)\n");
	sHelp->Add(DisplayKeys(Key.GoToContainingDir) + "Go to directory containing current item\n");
	sHelp->Add(DisplayKeys(Key.EditTags) + tag_screen_keydesc);
	sHelp->Add(DisplayKeys(Key.GoToPosition) + "Go to chosen position in current song\n");
	sHelp->Add(DisplayKeys(Key.Lyrics) + "Show/hide song's lyrics\n\n");
	
	sHelp->Add(DisplayKeys(Key.Quit) + "Quit\n\n\n");
	
	sHelp->Add("   [.b]Keys - Playlist screen\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Play\n");
	sHelp->Add(DisplayKeys(Key.Delete) + "Delete item/selected items from playlist\n");
	sHelp->Add(DisplayKeys(Key.Clear) + "Clear playlist\n");
	sHelp->Add(DisplayKeys(Key.Crop) + "Clear playlist but hold currently playing/selected items\n");
	sHelp->Add(DisplayKeys(Key.MvSongUp) + "Move item/group of items up\n");
	sHelp->Add(DisplayKeys(Key.MvSongDown) + "Move item/group of items down\n");
	sHelp->Add(DisplayKeys(Key.Add) + "Add url/file/directory to playlist\n");
	sHelp->Add(DisplayKeys(Key.SavePlaylist) + "Save playlist\n");
	sHelp->Add(DisplayKeys(Key.GoToNowPlaying) + "Go to currently playing position\n");
	sHelp->Add(DisplayKeys(Key.ToggleAutoCenter) + "Toggle auto center mode\n\n\n");
	
	sHelp->Add("   [.b]Keys - Browse screen\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Enter directory/Add item to playlist and play\n");
	sHelp->Add(DisplayKeys(Key.Space) + "Add item to playlist\n");
	sHelp->Add(DisplayKeys(Key.GoToParentDir) + "Go to parent directory\n");
	sHelp->Add(DisplayKeys(Key.Delete) + "Delete playlist\n\n\n");
	
	sHelp->Add("   [.b]Keys - Search engine\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Add item to playlist and play/change option\n");
	sHelp->Add(DisplayKeys(Key.Space) + "Add item to playlist\n");
	sHelp->Add(DisplayKeys(Key.StartSearching) + "Start searching immediately\n\n\n");
	
	sHelp->Add("   [.b]Keys - Media library\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(&Key.VolumeDown[0], 1) + "Previous column\n");
	sHelp->Add(DisplayKeys(&Key.VolumeUp[0], 1) + "Next column\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Add to playlist and play song/album/artist's songs\n");
	sHelp->Add(DisplayKeys(Key.Space) + "Add to playlist song/album/artist's songs\n\n\n");
	
	sHelp->Add("   [.b]Keys - Playlist Editor\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(&Key.VolumeDown[0], 1) + "Previous column\n");
	sHelp->Add(DisplayKeys(&Key.VolumeUp[0], 1) + "Next column\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Add item to playlist and play\n");
	sHelp->Add(DisplayKeys(Key.Space) + "Add to playlist/select item\n");
	sHelp->Add(DisplayKeys(Key.MvSongUp) + "Move item/group of items up\n");
	sHelp->Add(DisplayKeys(Key.MvSongDown) + "Move item/group of items down\n\n\n");
	
#	ifdef HAVE_TAGLIB_H
	sHelp->Add("   [.b]Keys - Tag editor\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Change option\n");
#	else
	sHelp->Add("   [.b]Keys - Tag info\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Return\n");
#	endif
	
	if (Config.header_visibility)
	{
		wHeader = new Window(0, 0, COLS, 2, "", Config.header_color, brNone);
		wHeader->Display();
	}
	
	int footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
	int footer_height = Config.statusbar_visibility ? 2 : 1;
	
	wFooter = new Window(0, footer_start_y, COLS, footer_height, "", Config.statusbar_color, brNone);
	wFooter->SetGetStringHelper(TraceMpdStatus);
	wFooter->Display();
	
	wCurrent = mPlaylist;
	current_screen = csPlaylist;
	
	int input;
	timer = time(NULL);
	
	sHelp->Timeout(ncmpcpp_window_timeout);
	mPlaylist->Timeout(ncmpcpp_window_timeout);
	mBrowser->Timeout(ncmpcpp_window_timeout);
	mTagEditor->Timeout(ncmpcpp_window_timeout);
	mSearcher->Timeout(ncmpcpp_window_timeout);
	mLibArtists->Timeout(ncmpcpp_window_timeout);
	mLibAlbums->Timeout(ncmpcpp_window_timeout);
	mLibSongs->Timeout(ncmpcpp_window_timeout);
	sLyrics->Timeout(ncmpcpp_window_timeout);
	wFooter->Timeout(ncmpcpp_window_timeout);
	mPlaylistList->Timeout(ncmpcpp_window_timeout);
	mPlaylistEditor->Timeout(ncmpcpp_window_timeout);
	
	mPlaylist->HighlightColor(Config.main_highlight_color);
	mBrowser->HighlightColor(Config.main_highlight_color);
	mTagEditor->HighlightColor(Config.main_highlight_color);
	mSearcher->HighlightColor(Config.main_highlight_color);
	mLibArtists->HighlightColor(Config.main_highlight_color);
	mLibAlbums->HighlightColor(Config.main_highlight_color);
	mLibSongs->HighlightColor(Config.main_highlight_color);
	mPlaylistList->HighlightColor(Config.main_highlight_color);
	mPlaylistEditor->HighlightColor(Config.main_highlight_color);
	
	Mpd->SetStatusUpdater(NcmpcppStatusChanged, NULL);
	Mpd->SetErrorHandler(NcmpcppErrorCallback, NULL);
	
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
		
		block_playlist_update = 0;
		messages_allowed = 1;
		
		if (Config.header_visibility)
		{
			string title;
			const int max_allowed_title_length = wHeader->GetWidth()-volume_state.length();
			
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
				case csTagEditor:
					title = tag_screen;
					break;
				case csSearcher:
					title = "Search engine";
					break;
				case csLibrary:
					title = "Media library";
					break;
				case csLyrics:
					title = song_lyrics;
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
				
				if (current_screen == csBrowser)
				{
					int max_length_without_scroll = wHeader->GetWidth()-volume_state.length()-title.length();
					ncmpcpp_string_t wbrowseddir = TO_WSTRING(browsed_dir);
					wHeader->Bold(1);
					if (browsed_dir.length() > max_length_without_scroll)
					{
#						ifdef UTF8_ENABLED
						wbrowseddir += L" ** ";
#						else
						wbrowseddir += " ** ";
#						endif
						const int scrollsize = max_length_without_scroll;
						ncmpcpp_string_t part = wbrowseddir.substr(browsed_dir_scroll_begin++, scrollsize);
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
				wHeader->WriteXY(0, 0, max_allowed_title_length, "[.b]1:[/b]Help  [.b]2:[/b]Playlist  [.b]3:[/b]Browse  [.b]4:[/b]Search  [.b]5:[/b]Library [.b]6:[/b]Playlist editor", 1);
		
			wHeader->SetColor(Config.volume_color);
			wHeader->WriteXY(max_allowed_title_length, 0, volume_state);
			wHeader->SetColor(Config.header_color);
		}
		
		// media library stuff
		
		if (current_screen == csLibrary)
		{
			if (mLibArtists->Empty())
			{
				found_pos = 0;
				vFoundPositions.clear();
				TagList list;
				mLibAlbums->Clear(0);
				Mpd->GetArtists(list);
				sort(list.begin(), list.end(), CaseInsensitiveComparison);
				for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
					mLibArtists->AddOption(*it);
				mLibArtists->Window::Clear();
				mLibArtists->Refresh();
			}
			
			if (mLibAlbums->Empty())
			{
				mLibAlbums->Reset();
				mLibSongs->Clear(0);
				TagList list;
				Mpd->GetAlbums(mLibArtists->GetCurrentOption(), list);
				for (TagList::iterator it = list.begin(); it != list.end(); it++)
				{
					SongList l;
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, *it);
					Mpd->CommitSearch(l);
					for (SongList::const_iterator j = l.begin(); j != l.end(); j++)
					{
						if ((*j)->GetYear() != EMPTY_TAG)
						{
							*it = "(" + (*j)->GetYear() + ") " + *it;
							break;
						}
					}
					FreeSongList(l);
				}
				sort(list.begin(), list.end());
				for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
					mLibAlbums->AddOption(*it);
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
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption());
					Mpd->CommitSearch(list);
				}
				else
				{
					mLibSongs->Clear(0);
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, mLibAlbums->GetCurrentOption());
					Mpd->CommitSearch(list);
					if (list.empty())
					{
						const int year_length = 7;
						const string &album = mLibAlbums->GetCurrentOption();
						Mpd->StartSearch(1);
						Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption());
						Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, album.substr(year_length));
						Mpd->CommitSearch(list);
					}
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
					bold ? mLibSongs->AddBoldOption(**it) : mLibSongs->AddOption(**it);
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
				Mpd->GetPlaylistContent(mPlaylistList->GetCurrentOption(), list);
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
					bold ? mPlaylistEditor->AddBoldOption(**it) : mPlaylistEditor->AddOption(**it);
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
		
		if (Config.columns_in_playlist && wCurrent == mPlaylist)
			wCurrent->Display(redraw_me);
		else
			wCurrent->Refresh(redraw_me);
		redraw_me = 0;
		
		wCurrent->ReadKey(input);
		if (input == ERR)
			continue;
		
		title_allowed = 1;
		timer = time(NULL);
		
		switch (current_screen)
		{
			case csPlaylist:
				mPlaylist->Highlighting(1);
				break;
			case csBrowser:
				browsed_dir_scroll_begin--;
				break;
			case csLibrary:
			{
				if (Keypressed(input, Key.Up) || Keypressed(input, Key.Down) || Keypressed(input, Key.PageUp) || Keypressed(input, Key.PageDown) || Keypressed(input, Key.Home) || Keypressed(input, Key.End) || Keypressed(input, Key.FindForward) || Keypressed(input, Key.FindBackward) || Keypressed(input, Key.NextFoundPosition) || Keypressed(input, Key.PrevFoundPosition))
				{
					if (wCurrent == mLibArtists)
						mLibAlbums->Clear(0);
					else if (wCurrent == mLibAlbums)
						mLibSongs->Clear(0);
				}
				break;
			}
			case csPlaylistEditor:
			{
				if (wCurrent == mPlaylistList && (Keypressed(input, Key.Up) || Keypressed(input, Key.Down) || Keypressed(input, Key.PageUp) || Keypressed(input, Key.PageDown) || Keypressed(input, Key.Home) || Keypressed(input, Key.End) || Keypressed(input, Key.FindForward) || Keypressed(input, Key.FindBackward) || Keypressed(input, Key.NextFoundPosition) || Keypressed(input, Key.PrevFoundPosition)))
					mPlaylistEditor->Clear(0);
			}
			default:
				break;
		}
		
		// key mapping beginning
		
		if (Keypressed(input, Key.Up))
		{
			wCurrent->Go(wUp);
		}
		else if (Keypressed(input, Key.Down))
		{
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
			int in;
			
			redraw_me = 1;
			
			while (1)
			{
				wCurrent->ReadKey(in);
				if (in == KEY_RESIZE)
					continue;
				else
					break;
			}
			
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
			sHelp->Timeout(ncmpcpp_window_timeout);
			mPlaylist->Resize(COLS, main_height);
			mPlaylist->SetTitle(Config.columns_in_playlist ? DisplayColumns(Config.song_columns_list_format) : "");
			mBrowser->Resize(COLS, main_height);
			mTagEditor->Resize(COLS, main_height);
			mSearcher->Resize(COLS, main_height);
			sLyrics->Resize(COLS, main_height);
			sLyrics->Timeout(ncmpcpp_window_timeout);
			
			lib_artist_width = COLS/3-1;
			lib_albums_start_x = lib_artist_width+1;
			lib_albums_width = COLS/3;
			lib_songs_start_x = lib_artist_width+lib_albums_width+2;
			lib_songs_width = COLS-COLS/3*2-1;
			
			mLibArtists->Resize(lib_artist_width, main_height);
			mLibAlbums->Resize(lib_albums_width, main_height);
			mLibSongs->Resize(lib_songs_width, main_height);
			
			mLibAlbums->MoveTo(lib_albums_start_x, main_start_y);
			mLibSongs->MoveTo(lib_songs_start_x, main_start_y);
			
			mPlaylistList->Resize(lib_artist_width, main_height);
			mPlaylistEditor->Resize(lib_albums_width+lib_songs_width+1, main_height);
			mPlaylistEditor->MoveTo(lib_albums_start_x, main_start_y);
			
			if (Config.header_visibility)
				wHeader->Resize(COLS, wHeader->GetHeight());
			
			footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
			wFooter->MoveTo(0, footer_start_y);
			wFooter->Resize(COLS, wFooter->GetHeight());
			
			if (wCurrent != sHelp)
				wCurrent->Window::Clear();
			wCurrent->Refresh(1);
			if (current_screen == csLibrary)
			{
				REFRESH_MEDIA_LIBRARY_SCREEN;
			}
			else if (current_screen == csPlaylistEditor)
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
						Mpd->PlayID(mPlaylist->at(mPlaylist->GetChoice()-1).GetID());
					break;
				}
				case csBrowser:
				{
					GO_TO_PARENT_DIR:
					
					const Item &item = mBrowser->at(mBrowser->GetChoice()-1);
					switch (item.type)
					{
						case itDirectory:
						{
							found_pos = 0;
							vFoundPositions.clear();
							GetDirectory(item.name, browsed_dir);
							break;
						}
						case itSong:
						{
							Song &s = *item.song;
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								Mpd->PlayID(id);
								ShowMessage("Added to playlist: " +  OmitBBCodes(DisplaySong(s)));
							}
							mBrowser->Refresh();
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
				case csTagEditor:
				{
#					ifdef HAVE_TAGLIB_H
					int id = mTagEditor->GetRealChoice();
					int option = mTagEditor->GetChoice();
					LOCK_STATUSBAR;
					Song &s = edited_song;
					
					switch (id)
					{
						case 1:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]New title:[/b] ", 1);
							if (s.GetTitle() == UNKNOWN_TITLE)
								s.SetTitle(wFooter->GetString());
							else
								s.SetTitle(wFooter->GetString(s.GetTitle()));
							mTagEditor->UpdateOption(option, "[.b]Title:[/b] " + s.GetTitle());
							break;
						}
						case 2:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]New artist:[/b] ", 1);
							if (s.GetArtist() == UNKNOWN_ARTIST)
								s.SetArtist(wFooter->GetString());
							else
								s.SetArtist(wFooter->GetString(s.GetArtist()));
							mTagEditor->UpdateOption(option, "[.b]Artist:[/b] " + s.GetArtist());
							break;
						}
						case 3:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]New album:[/b] ", 1);
							if (s.GetAlbum() == UNKNOWN_ALBUM)
								s.SetAlbum(wFooter->GetString());
							else
								s.SetAlbum(wFooter->GetString(s.GetAlbum()));
							mTagEditor->UpdateOption(option, "[.b]Album:[/b] " + s.GetAlbum());
							break;
						}
						case 4:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]New year:[/b] ", 1);
							if (s.GetYear() == EMPTY_TAG)
								s.SetYear(wFooter->GetString(4));
							else
								s.SetYear(wFooter->GetString(s.GetYear(), 4));
							mTagEditor->UpdateOption(option, "[.b]Year:[/b] " + s.GetYear());
							break;
						}
						case 5:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]New track:[/b] ", 1);
							if (s.GetTrack() == EMPTY_TAG)
								s.SetTrack(wFooter->GetString(3));
							else
								s.SetTrack(wFooter->GetString(s.GetTrack(), 3));
							mTagEditor->UpdateOption(option, "[.b]Track:[/b] " + s.GetTrack());
							break;
						}
						case 6:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]New genre:[/b] ", 1);
							if (s.GetGenre() == EMPTY_TAG)
								s.SetGenre(wFooter->GetString());
							else
								s.SetGenre(wFooter->GetString(s.GetGenre()));
							mTagEditor->UpdateOption(option, "[.b]Genre:[/b] " + s.GetGenre());
							break;
						}
						case 7:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]New comment:[/b] ", 1);
							if (s.GetComment() == EMPTY_TAG)
								s.SetComment(wFooter->GetString());
							else
								s.SetComment(wFooter->GetString(s.GetComment()));
							mTagEditor->UpdateOption(option, "[.b]Comment:[/b] " + s.GetComment());
							break;
						}
						case 8:
						{
							string path_to_file = Config.mpd_music_dir + "/" + s.GetFile();
							TagLib::FileRef f(path_to_file.c_str());
							if (!f.isNull())
							{
								ShowMessage("Updating tags...");
								s.GetEmptyFields(1);
								f.tag()->setTitle(TO_WSTRING(s.GetTitle()));
								f.tag()->setArtist(TO_WSTRING(s.GetArtist()));
								f.tag()->setAlbum(TO_WSTRING(s.GetAlbum()));
								f.tag()->setYear(StrToInt(s.GetYear()));
								f.tag()->setTrack(StrToInt(s.GetTrack()));
								f.tag()->setGenre(TO_WSTRING(s.GetGenre()));
								f.tag()->setComment(TO_WSTRING(s.GetComment()));
								s.GetEmptyFields(0);
								f.save();
								ShowMessage("Tags updated!");
								Mpd->UpdateDirectory(s.GetDirectory());
								if (prev_screen == csSearcher)
								{
									*vSearched[mSearcher->GetRealChoice()-2] = s;
									mSearcher->UpdateOption(mSearcher->GetChoice(), DisplaySong(s));
								}
							}
							else
								ShowMessage("Error writing tags!");
						}
						case 9:
						{
#							endif // HAVE_TAGLIB_H
							wCurrent->Clear();
							wCurrent = wPrev;
							current_screen = prev_screen;
							redraw_me = 1;
							if (current_screen == csLibrary)
							{
								REFRESH_MEDIA_LIBRARY_SCREEN;
							}
							else if (current_screen == csPlaylistEditor)
							{
								REFRESH_PLAYLIST_EDITOR_SCREEN;
							}
#							ifdef HAVE_TAGLIB_H
							break;
						}
					}
					UNLOCK_STATUSBAR;
#					endif // HAVE_TAGLIB_H
					break;
				}
				case csSearcher:
				{
					ENTER_SEARCH_ENGINE_SCREEN:
					
					int id = mSearcher->GetChoice();
					int option = mSearcher->GetChoice();
					LOCK_STATUSBAR;
					Song &s = searched_song;
					
					switch (id)
					{
						case 1:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Filename:[/b] ", 1);
							if (s.GetShortFilename() == EMPTY_TAG)
								s.SetShortFilename(wFooter->GetString());
							else
								s.SetShortFilename(wFooter->GetString(s.GetShortFilename()));
							mSearcher->UpdateOption(option, "[.b]Filename:[/b] " + s.GetShortFilename());
							break;
						}
						case 2:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Title:[/b] ", 1);
							if (s.GetTitle() == UNKNOWN_TITLE)
								s.SetTitle(wFooter->GetString());
							else
								s.SetTitle(wFooter->GetString(s.GetTitle()));
							mSearcher->UpdateOption(option, "[.b]Title:[/b] " + s.GetTitle());
							break;
						}
						case 3:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Artist:[/b] ", 1);
							if (s.GetArtist() == UNKNOWN_ARTIST)
								s.SetArtist(wFooter->GetString());
							else
								s.SetArtist(wFooter->GetString(s.GetArtist()));
							mSearcher->UpdateOption(option, "[.b]Artist:[/b] " + s.GetArtist());
							break;
						}
						case 4:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Album:[/b] ", 1);
							if (s.GetAlbum() == UNKNOWN_ALBUM)
								s.SetAlbum(wFooter->GetString());
							else
								s.SetAlbum(wFooter->GetString(s.GetAlbum()));
							mSearcher->UpdateOption(option, "[.b]Album:[/b] " + s.GetAlbum());
							break;
						}
						case 5:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Year:[/b] ", 1);
							if (s.GetYear() == EMPTY_TAG)
								s.SetYear(wFooter->GetString(4));
							else
								s.SetYear(wFooter->GetString(s.GetYear(), 4));
							mSearcher->UpdateOption(option, "[.b]Year:[/b] " + s.GetYear());
							break;
						}
						case 6:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Track:[/b] ", 1);
							if (s.GetTrack() == EMPTY_TAG)
								s.SetTrack(wFooter->GetString(3));
							else
								s.SetTrack(wFooter->GetString(s.GetTrack(), 3));
							mSearcher->UpdateOption(option, "[.b]Track:[/b] " + s.GetTrack());
							break;
						}
						case 7:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Genre:[/b] ", 1);
							if (s.GetGenre() == EMPTY_TAG)
								s.SetGenre(wFooter->GetString());
							else
								s.SetGenre(wFooter->GetString(s.GetGenre()));
							mSearcher->UpdateOption(option, "[.b]Genre:[/b] " + s.GetGenre());
							break;
						}
						case 8:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Comment:[/b] ", 1);
							if (s.GetComment() == EMPTY_TAG)
								s.SetComment(wFooter->GetString());
							else
								s.SetComment(wFooter->GetString(s.GetComment()));
							mSearcher->UpdateOption(option, "[.b]Comment:[/b] " + s.GetComment());
							break;
						}
						case 10:
						{
							search_mode_match = !search_mode_match;
							mSearcher->UpdateOption(option, "[.b]Search mode:[/b] " + (search_mode_match ? search_mode_one : search_mode_two));
							break;
						}
						case 11:
						{
							search_case_sensitive = !search_case_sensitive;
							mSearcher->UpdateOption(option, "[.b]Case sensitive:[/b] " + (string)(search_case_sensitive ? "Yes" : "No"));
							break;
						}
						case 13:
						{
							ShowMessage("Searching...");
							Search(vSearched, s);
							ShowMessage("Searching finished!");
							if (!vSearched.empty())
							{
								bool bold = 0;
								mSearcher->AddSeparator();
								mSearcher->AddStaticBoldOption("[.white]Search results:[/white] [.green]Found " + IntoStr(vSearched.size()) + (vSearched.size() > 1 ? " songs" : " song") + "[/green]");
								mSearcher->AddSeparator();
									
								for (SongList::const_iterator it = vSearched.begin(); it != vSearched.end(); it++)
								{	
									for (int j = 0; j < mPlaylist->Size(); j++)
									{
										if (mPlaylist->at(j).GetHash() == (*it)->GetHash())
										{
											bold = 1;
											break;
										}
									}
									bold ? mSearcher->AddBoldOption(DisplaySong(**it)) : mSearcher->AddOption(DisplaySong(**it));
									bold = 0;
								}
									
								for (int i = 1; i <=13; i++)
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
							FreeSongList(vSearched);
							PrepareSearchEngine(searched_song);
							ShowMessage("Search state reset");
							break;
						}
						default:
						{
							Song &s = *vSearched[mSearcher->GetRealChoice()-2];
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								Mpd->PlayID(id);
								ShowMessage("Added to playlist: " +  OmitBBCodes(DisplaySong(s)));
							}
							break;
						}
					}
					UNLOCK_STATUSBAR;
					break;
				}
				case csLibrary:
				{
					ENTER_LIBRARY_SCREEN: // same code for Key.Space, but without playing.
					
					SongList list;
					
					if (wCurrent == mLibArtists)
					{
						const string &artist = mLibArtists->GetCurrentOption();
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
							ShowMessage("Adding songs from: " + mLibArtists->GetCurrentOption() + " \"" + mLibAlbums->GetCurrentOption() + "\"");
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
							Song &s = mLibSongs->at(mLibSongs->GetChoice()-1);
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(id);
							}
						}
					}
					FreeSongList(list);
					if (Keypressed(input, Key.Space))
						wCurrent->Go(wDown);
					break;
				}
				case csPlaylistEditor:
				{
					ENTER_PLAYLIST_EDITOR_SCREEN: // same code for Key.Space, but without playing.
					
					SongList list;
					
					if (wCurrent == mPlaylistList)
					{
						const string &playlist = mPlaylistList->GetCurrentOption();
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
							Song &s = mPlaylistEditor->at(mPlaylistEditor->GetChoice()-1);
							int id = Mpd->AddSong(s);
							if (id >= 0)
							{
								ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
								if (Keypressed(input, Key.Enter))
									Mpd->PlayID(id);
							}
						}
					}
					FreeSongList(list);
					if (Keypressed(input, Key.Space))
						wCurrent->Go(wDown);
					break;
				}
				default:
					break;
			}
		}
		else if (Keypressed(input, Key.Space))
		{
			if (Config.space_selects || wCurrent == mPlaylist)
			{
				if (wCurrent == mPlaylist || (wCurrent == mBrowser && wCurrent->GetChoice() > (browsed_dir != "/" ? 1 : 0)) || (wCurrent == mSearcher && !vSearched.empty() && wCurrent->GetChoice() > search_engine_static_option) || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
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
					const Item &item = mBrowser->at(mBrowser->GetChoice()-1);
					switch (item.type)
					{
						case itDirectory:
						{
							if (browsed_dir != "/" && mBrowser->GetChoice() == 1)
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
							Song &s = *item.song;
							if (Mpd->AddSong(s) != -1)
								ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
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
				else if (current_screen == csSearcher && !vSearched.empty())
				{
					int id = mSearcher->GetChoice()-search_engine_static_option-1;
					if (id < 0)
						continue;
				
					Song &s = *vSearched[id];
					if (Mpd->AddSong(s) != -1)
						ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
					mSearcher->Go(wDown);
				}
				else if (current_screen == csLibrary)
					goto ENTER_LIBRARY_SCREEN; // sorry, but that's stupid to copy the same code here.
				else if (current_screen == csPlaylistEditor)
					goto ENTER_PLAYLIST_EDITOR_SCREEN; // same what in library screen.
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
			else
				Mpd->SetVolume(Mpd->GetVolume()-1);
		}
		else if (Keypressed(input, Key.Delete))
		{
			if (!mPlaylist->Empty() && current_screen == csPlaylist)
			{
				if (mPlaylist->IsAnySelected())
				{
					vector<int> list;
					mPlaylist->GetSelectedList(list);
					for (vector<int>::const_reverse_iterator it = list.rbegin(); it != list.rend(); it++)
						DeleteSong(*it-1);
					ShowMessage("Selected items deleted!");
					redraw_me = 1;
				}
				else
				{
					block_playlist_update = 1;
					dont_change_now_playing = 1;
					mPlaylist->Timeout(50);
					while (!mPlaylist->Empty() && Keypressed(input, Key.Delete))
					{
						TraceMpdStatus();
						timer = time(NULL);
						DeleteSong(mPlaylist->GetChoice()-1);
						mPlaylist->Refresh();
						mPlaylist->ReadKey(input);
					}
					mPlaylist->Timeout(ncmpcpp_window_timeout);
					dont_change_now_playing = 0;
				}
				Mpd->CommitQueue();
			}
			else if (current_screen == csBrowser || wCurrent == mPlaylistList)
			{
				LOCK_STATUSBAR;
				int id = wCurrent->GetChoice()-1;
				const string &name = wCurrent == mBrowser ? mBrowser->at(id).name : mPlaylistList->at(id);
				if (current_screen != csBrowser || mBrowser->at(id).type == itPlaylist)
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
				UNLOCK_STATUSBAR;
			}
			else if (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
			{
				if (mPlaylistEditor->IsAnySelected())
				{
					vector<int> list;
					mPlaylistEditor->GetSelectedList(list);
					for (vector<int>::const_reverse_iterator it = list.rbegin(); it != list.rend(); it++)
						PlaylistDeleteSong(mPlaylistList->GetCurrentOption(), *it-1);
					ShowMessage("Selected items deleted from playlist '" + mPlaylistList->GetCurrentOption() + "'!");
					redraw_me = 1;
				}
				else
				{
					mPlaylistEditor->Timeout(50);
					while (!mPlaylistEditor->Empty() && Keypressed(input, Key.Delete))
					{
						TraceMpdStatus();
						timer = time(NULL);
						PlaylistDeleteSong(mPlaylistList->GetCurrentOption(), mPlaylistEditor->GetChoice()-1);
						mPlaylistEditor->Refresh();
						mPlaylistEditor->ReadKey(input);
					}
					mPlaylistEditor->Timeout(ncmpcpp_window_timeout);
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
			LOCK_STATUSBAR;
			wFooter->WriteXY(0, Config.statusbar_visibility, "Save playlist as: ", 1);
			string playlist_name = wFooter->GetString();
			UNLOCK_STATUSBAR;
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
					LOCK_STATUSBAR;
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
					UNLOCK_STATUSBAR;
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
				if (mPlaylist->IsAnySelected())
				{
					vector<int> list;
					mPlaylist->GetSelectedList(list);
					mPlaylist->Highlight(list[(list.size()-1)/2]-1);
					for (vector<int>::const_iterator it = list.begin(); it != list.end(); it++)
					{
						if (!MoveSongUp(*it-1))
						{
							mPlaylist->Go(wDown);
							break;
						}
					}
				}
				else
					if (MoveSongUp(mPlaylist->GetChoice()-1))
						mPlaylist->Go(wUp);
			}
			else if (wCurrent == mPlaylistEditor)
			{
				if (mPlaylistEditor->IsAnySelected())
				{
					vector<int> list;
					mPlaylistEditor->GetSelectedList(list);
					mPlaylistEditor->Highlight(list[(list.size()-1)/2]-1);
					for (vector<int>::const_iterator it = list.begin(); it != list.end(); it++)
					{
						if (!PlaylistMoveSongUp(mPlaylistList->GetCurrentOption(), *it-1))
						{
							mPlaylistEditor->Go(wDown);
							break;
						}
					}
				}
				else
					if (PlaylistMoveSongUp(mPlaylistList->GetCurrentOption(), mPlaylistEditor->GetChoice()-1))
						mPlaylistEditor->Go(wUp);
			}
		}
		else if (Keypressed(input, Key.MvSongDown))
		{
			if (current_screen == csPlaylist)
			{
				block_playlist_update = 1;
				if (mPlaylist->IsAnySelected())
				{
					vector<int> list;
					mPlaylist->GetSelectedList(list);
					mPlaylist->Highlight(list[(list.size()-1)/2]+1);
					for (vector<int>::const_reverse_iterator it = list.rbegin(); it != list.rend(); it++)
					{
						if (!MoveSongDown(*it-1))
						{
							mPlaylist->Go(wUp);
							break;
						}
					}
				}
				else
					if (MoveSongDown(mPlaylist->GetChoice()-1))
						mPlaylist->Go(wDown);
			}
			else if (wCurrent == mPlaylistEditor)
			{
				if (mPlaylistEditor->IsAnySelected())
				{
					vector<int> list;
					mPlaylistEditor->GetSelectedList(list);
					mPlaylistEditor->Highlight(list[(list.size()-1)/2]+1);
					for (vector<int>::const_reverse_iterator it = list.rbegin(); it != list.rend(); it++)
					{
						if (!PlaylistMoveSongDown(mPlaylistList->GetCurrentOption(), *it-1))
						{
							mPlaylistEditor->Go(wUp);
							break;
						}
					}
				}
				else
					if (PlaylistMoveSongDown(mPlaylistList->GetCurrentOption(), mPlaylistEditor->GetChoice()-1))
						mPlaylistEditor->Go(wDown);
			}
		}
		else if (Keypressed(input, Key.Add))
		{
			LOCK_STATUSBAR;
			wFooter->WriteXY(0, Config.statusbar_visibility, "Add: ", 1);
			string path = wFooter->GetString();
			UNLOCK_STATUSBAR;
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
			LOCK_STATUSBAR;
			
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
					string tracklength = "[" + ShowTime(songpos) + "/" + s.GetLength() + "]";
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
			UNLOCK_STATUSBAR;
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
				mPlaylist->Highlight(now_playing+1);
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
			LOCK_STATUSBAR;
			wFooter->WriteXY(0, Config.statusbar_visibility, "Set crossfade to: ", 1);
			string crossfade = wFooter->GetString(3);
			UNLOCK_STATUSBAR;
			int cf = StrToInt(crossfade);
			if (cf > 0)
			{
				Config.crossfade_time = cf;
				Mpd->SetCrossfade(cf);
			}
		}
		else if (Keypressed(input, Key.EditTags))
		{
			if ((wCurrent == mPlaylist && !mPlaylist->Empty())
			||  (wCurrent == mBrowser && mBrowser->at(mBrowser->GetChoice()-1).type == itSong)
			||  (wCurrent == mSearcher && !vSearched.empty() && mSearcher->GetChoice() > search_engine_static_option)
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty()))
			{
				int id = wCurrent->GetChoice()-1;
				Song *s;
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csBrowser:
						s = mBrowser->at(id).song;
						break;
					case csSearcher:
						s = vSearched[id-search_engine_static_option];
						break;
					case csLibrary:
						s = &mLibSongs->at(id);
						break;
					case csPlaylistEditor:
						s = &mPlaylistEditor->at(id);
						break;
					default:
						break;
				}
				if (GetSongInfo(*s))
				{
					wPrev = wCurrent;
					wCurrent = mTagEditor;
					prev_screen = current_screen;
					current_screen = csTagEditor;
				}
				else
					ShowMessage("Cannot read file!");
			}
		}
		else if (Keypressed(input, Key.GoToContainingDir))
		{
			if ((wCurrent == mPlaylist && !mPlaylist->Empty())
			|| (wCurrent == mSearcher && !vSearched.empty() && mSearcher->GetChoice() > search_engine_static_option)
			|| (wCurrent == mLibSongs && !mLibSongs->Empty())
			|| (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty()))
			{
				int id = wCurrent->GetChoice()-1;
				Song *s;
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csSearcher:
						s = vSearched[mSearcher->GetRealChoice()-2];
						break;
					case csLibrary:
						s = &mLibSongs->at(id);
						break;
					case csPlaylistEditor:
						s = &mPlaylistEditor->at(id);
						break;
					default:
						break;
				}
				
				if (s->GetDirectory() == EMPTY_TAG) // for streams
					continue;
				
				string option = OmitBBCodes(DisplaySong(*s));
				GetDirectory(s->GetDirectory());
				for (int i = 1; i <= mBrowser->Size(); i++)
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
			LOCK_STATUSBAR;
			wFooter->WriteXY(0, Config.statusbar_visibility, "Position to go (in %): ", 1);
			string position = wFooter->GetString(3);
			int newpos = StrToInt(position);
			if (newpos > 0 && newpos < 100 && !position.empty())
				Mpd->Seek(mPlaylist->at(now_playing).GetTotalLength()*newpos/100.0);
			UNLOCK_STATUSBAR;
		}
		else if (Keypressed(input, Key.ReverseSelection))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || (wCurrent == mSearcher && !vSearched.empty()) || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
			{
				for (int i = 1; i <= wCurrent->Size(); i++)
					wCurrent->Select(i, !wCurrent->Selected(i) && !wCurrent->IsStatic(i));
				// hackish shit begins
				if (wCurrent == mBrowser && browsed_dir != "/")
					wCurrent->Select(1, 0); // [..] cannot be selected, uhm.
				if (wCurrent == mSearcher)
					wCurrent->Select(14, 0); // 'Reset' cannot be selected, omgplz.
				// hacking shit ends. need better solution :/
				ShowMessage("Selection reversed!");
			}
		}
		else if (Keypressed(input, Key.DeselectAll))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || wCurrent == mSearcher || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
			{
				if (wCurrent->IsAnySelected())
				{
					for (int i = 1; i <= wCurrent->Size(); i++)
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
								Song *s = new Song(mPlaylist->at(*it-1));
								result.push_back(s);
								break;
							}
							case csBrowser:
							{
								const Item &item = mBrowser->at(*it-1);
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
								Song *s = new Song(*vSearched[*it-search_engine_static_option-1]);
								result.push_back(s);
								break;
							}
							case csLibrary:
							{
								Song *s = new Song(mLibSongs->at(*it-1));
								result.push_back(s);
								break;
							}
							case csPlaylistEditor:
							{
								Song *s = new Song(mPlaylistEditor->at(*it-1));
								result.push_back(s);
								break;
							}
							default:
								break;
						}
					}
					
					const int dialog_width = COLS*0.8;
					const int dialog_height = LINES*0.6;
					Menu<string> *mDialog = new Menu<string>((COLS-dialog_width)/2, (LINES-dialog_height)/2, dialog_width, dialog_height, "Add selected items to...", clYellow, brGreen);
					mDialog->Timeout(ncmpcpp_window_timeout);
					
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
					
					while (!Keypressed(input, Key.Enter))
					{
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
					
					redraw_me = 1;
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
					
					if (id == 1)
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
					else if (id == 2)
					{
						LOCK_STATUSBAR;
						wFooter->WriteXY(0, Config.statusbar_visibility, "Save playlist as: ", 1);
						string playlist = wFooter->GetString();
						UNLOCK_STATUSBAR;
						if (!playlist.empty())
						{
							for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
								Mpd->QueueAddToPlaylist(playlist, **it);
							Mpd->CommitQueue();
							ShowMessage("Selected items added to playlist '" + playlist + "'!");
						}
						
					}
					else if (id > 2 && id < mDialog->Size())
					{
						for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
							Mpd->QueueAddToPlaylist(playlists[id-4], **it);
						Mpd->CommitQueue();
						ShowMessage("Selected items added to playlist '" + playlists[id-4] + "'!");
					}
					
					if (id != mDialog->Size())
					{
						// refresh playlist's lists
						if (browsed_dir == "/")
							GetDirectory("/");
						mPlaylistList->Clear(0); // make playlist editor update itself
					}
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
					if (!mPlaylist->Selected(i+1) && i != now_playing)
						Mpd->QueueDeleteSongId(mPlaylist->at(i).GetID());
				}
				// if mpd deletes now playing song deletion will be sluggishly slow
				// then so we have to assure it will be deleted at the very end.
				if (!mPlaylist->Selected(now_playing+1))
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
			||  (current_screen == csSearcher && !vSearched.empty()))
			{
				string how = Keypressed(input, Key.FindForward) ? "forward" : "backward";
				found_pos = -1;
				vFoundPositions.clear();
				LOCK_STATUSBAR;
				wFooter->WriteXY(0, Config.statusbar_visibility, "Find " + how + ": ", 1);
				string findme = wFooter->GetString();
				UNLOCK_STATUSBAR;
				timer = time(NULL);
				if (findme.empty())
					continue;
				transform(findme.begin(), findme.end(), findme.begin(), tolower);
				
				Menu<Song> *mCurrent = static_cast<Menu<Song> *>(wCurrent);
				
				for (int i = (wCurrent == mBrowser ? search_engine_static_option : 1); i <= mCurrent->Size(); i++)
				{
					string name = mCurrent->GetOption(i);
					transform(name.begin(), name.end(), name.begin(), tolower);
					if (name.find(findme) != string::npos && !mCurrent->IsStatic(i))
					{
						vFoundPositions.push_back(i);
						if (Keypressed(input, Key.FindForward)) // forward
						{
							if (found_pos < 0 && i >= mCurrent->GetChoice())
								found_pos = vFoundPositions.size()-1;
						}
						else // backward
						{
							if (i <= mCurrent->GetChoice())
								found_pos = vFoundPositions.size()-1;
						}
					}
				}
				
				if (Config.wrapped_search ? vFoundPositions.empty() : found_pos < 0)
					ShowMessage("Unable to find \"" + findme + "\"");
				else
				{
					wCurrent->Highlight(vFoundPositions[found_pos < 0 ? 0 : found_pos]);
					if (wCurrent == mPlaylist)
						mPlaylist->Highlighting(1);
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
		else if (Keypressed(input, Key.Lyrics))
		{
			if (wCurrent == sLyrics)
			{
				wCurrent->Window::Clear();
				current_screen = prev_screen;
				wCurrent = wPrev;
				redraw_me = 1;
				if (current_screen == csLibrary)
				{
					REFRESH_MEDIA_LIBRARY_SCREEN;
				}
				else if (current_screen == csPlaylistEditor)
				{
					REFRESH_PLAYLIST_EDITOR_SCREEN;
				}
			}
			else if (
			    (wCurrent == mPlaylist && !mPlaylist->Empty())
			||  (wCurrent == mBrowser && mBrowser->at(mBrowser->GetChoice()-1).type == itSong)
			||  (wCurrent == mSearcher && !vSearched.empty() && mSearcher->GetChoice() > search_engine_static_option)
			||  (wCurrent == mLibSongs && !mLibSongs->Empty())
			||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty()))
			{
				Song *s;
				int id = wCurrent->GetChoice()-1;
				switch (current_screen)
				{
					case csPlaylist:
						s = &mPlaylist->at(id);
						break;
					case csBrowser:
						s = mBrowser->at(id).song;
						break;
					case csSearcher:
						s = vSearched[id-search_engine_static_option]; // first one is 'Reset'
						break;
					case csLibrary:
						s = &mLibSongs->at(id);
						break;
					case csPlaylistEditor:
						s = &mPlaylistEditor->at(id);
						break;
					default:
						break;
				}
				if (s->GetArtist() != UNKNOWN_ARTIST && s->GetTitle() != UNKNOWN_TITLE)
				{
					wPrev = wCurrent;
					prev_screen = current_screen;
					wCurrent = sLyrics;
					wCurrent->Clear();
					current_screen = csLyrics;
					song_lyrics = "Lyrics: " + s->GetArtist() + " - " + s->GetTitle();
					sLyrics->WriteXY(0, 0, "Fetching lyrics...");
					sLyrics->Refresh();
					sLyrics->Add(GetLyrics(s->GetArtist(), s->GetTitle()));
					sLyrics->Timeout(ncmpcpp_window_timeout);
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
			if (wCurrent != mPlaylist && current_screen != csTagEditor)
			{
				found_pos = 0;
				vFoundPositions.clear();
				wCurrent = mPlaylist;
				wCurrent->Hide();
				current_screen = csPlaylist;
				redraw_me = 1;
			}
		}
		else if (Keypressed(input, Key.Browser))
		{
			SWITCHER_BROWSER_REDIRECT:
			if (browsed_dir.empty())
				browsed_dir = "/";
			
			mBrowser->Empty() ? GetDirectory(browsed_dir) : UpdateItemList(mBrowser);
			
			if (wCurrent != mBrowser && current_screen != csTagEditor)
			{
				found_pos = 0;
				vFoundPositions.clear();
				wCurrent = mBrowser;
				wCurrent->Hide();
				current_screen = csBrowser;
				redraw_me = 1;
			}
		}
		else if (Keypressed(input, Key.SearchEngine))
		{
			if (current_screen != csTagEditor && current_screen != csSearcher)
			{
				found_pos = 0;
				vFoundPositions.clear();
				if (vSearched.empty())
					PrepareSearchEngine(searched_song);
				wCurrent = mSearcher;
				wCurrent->Hide();
				current_screen = csSearcher;
				redraw_me = 1;
				if (!vSearched.empty())
				{
					wCurrent->WriteXY(0, 0, "Updating list...");
					UpdateFoundList(vSearched, mSearcher);
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
				
				redraw_me = 1;
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
				
				redraw_me = 1;
				REFRESH_PLAYLIST_EDITOR_SCREEN;
				
				wCurrent = mPlaylistList;
				current_screen = csPlaylistEditor;
				
				UpdateSongList(mPlaylistEditor);
			}
		}
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

