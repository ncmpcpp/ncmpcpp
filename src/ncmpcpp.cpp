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

SongList vPlaylist;
SongList vSearched;
SongList vLibSongs;
SongList vPlaylistContent;

ItemList vBrowser;
TagList vArtists;


vector<int> vFoundPositions;
int found_pos = 0;

Window *wCurrent = 0;
Window *wPrev = 0;
Menu *mPlaylist;
Menu *mBrowser;
Menu *mTagEditor;
Menu *mSearcher;
Menu *mLibArtists;
Menu *mLibAlbums;
Menu *mLibSongs;
Menu *mPlaylistList;
Menu *mPlaylistEditor;
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
string browsed_subdir;
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
		EnableColors();
	
	int main_start_y = 2;
	int main_height = LINES-4;
	
	if (!Config.header_visibility)
	{
		main_start_y -= 2;
		main_height += 2;
	}
	if (!Config.statusbar_visibility)
		main_height++;
	
	mPlaylist = new Menu(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mPlaylist->SetSelectPrefix(Config.selected_item_prefix);
	mPlaylist->SetSelectSuffix(Config.selected_item_suffix);
	
	mBrowser = static_cast<Menu *>(mPlaylist->Clone());
	mTagEditor = static_cast<Menu *>(mPlaylist->Clone());
	mSearcher = static_cast<Menu *>(mPlaylist->Clone());
	
	int lib_artist_width = COLS/3-1;
	int lib_albums_width = COLS/3;
	int lib_albums_start_x = lib_artist_width+1;
	int lib_songs_width = COLS-COLS/3*2-1;
	int lib_songs_start_x = lib_artist_width+lib_albums_width+2;
	
	mLibArtists = new Menu(0, main_start_y, lib_artist_width, main_height, "Artists", Config.main_color, brNone);
	mLibAlbums = new Menu(lib_albums_start_x, main_start_y, lib_albums_width, main_height, "Albums", Config.main_color, brNone);
	mLibSongs = new Menu(lib_songs_start_x, main_start_y, lib_songs_width, main_height, "Songs", Config.main_color, brNone);
	mLibSongs->SetSelectPrefix(Config.selected_item_prefix);
	mLibSongs->SetSelectSuffix(Config.selected_item_suffix);
	
	mPlaylistList = new Menu(0, main_start_y, lib_artist_width, main_height, "Playlists", Config.main_color, brNone);
	mPlaylistEditor = new Menu(lib_albums_start_x, main_start_y, lib_albums_width+lib_songs_width+1, main_height, "Playlist's content", Config.main_color, brNone);
	mPlaylistEditor->SetSelectPrefix(Config.selected_item_prefix);
	mPlaylistEditor->SetSelectSuffix(Config.selected_item_suffix);
	
	sHelp = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	sLyrics = static_cast<Scrollpad *>(sHelp->EmptyClone());
	
	sHelp->Add("   [b]Keys - Movement\n -----------------------------------------[/b]\n");
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
	
	sHelp->Add("   [b]Keys - Global\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Stop) + "Stop\n");
	sHelp->Add(DisplayKeys(Key.Pause) + "Pause\n");
	sHelp->Add(DisplayKeys(Key.Next) + "Next track\n");
	sHelp->Add(DisplayKeys(Key.Prev) + "Previous track\n");
	sHelp->Add(DisplayKeys(Key.SeekForward) + "Seek forward\n");
	sHelp->Add(DisplayKeys(Key.SeekBackward) + "Seek backward\n");
	sHelp->Add(DisplayKeys(Key.VolumeDown) + "Decrease volume\n");
	sHelp->Add(DisplayKeys(Key.VolumeUp) + "Increase volume\n\n");
	
	sHelp->Add(DisplayKeys(Key.ToggleSpaceMode) + "Toggle space mode (select/add items)\n");
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
	sHelp->Add(DisplayKeys(Key.EditTags) + tag_screen_keydesc);
	sHelp->Add(DisplayKeys(Key.GoToPosition) + "Go to chosen position in current song\n");
	sHelp->Add(DisplayKeys(Key.Lyrics) + "Show/hide song's lyrics\n\n");
	
	sHelp->Add(DisplayKeys(Key.Quit) + "Quit\n\n\n");
	
	sHelp->Add("   [b]Keys - Playlist screen\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Play\n");
	sHelp->Add(DisplayKeys(Key.Delete) + "Delete item/selected items from playlist\n");
	sHelp->Add(DisplayKeys(Key.Clear) + "Clear whole playlist\n");
	sHelp->Add(DisplayKeys(Key.Crop) + "Clear playlist but hold currently playing/selected items\n");
	sHelp->Add(DisplayKeys(Key.MvSongUp) + "Move item up\n");
	sHelp->Add(DisplayKeys(Key.MvSongDown) + "Move item down\n");
	sHelp->Add(DisplayKeys(Key.Add) + "Add url/file/directory to playlist\n");
	sHelp->Add(DisplayKeys(Key.SavePlaylist) + "Save playlist\n");
	sHelp->Add(DisplayKeys(Key.GoToNowPlaying) + "Go to currently playing position\n");
	sHelp->Add(DisplayKeys(Key.ToggleAutoCenter) + "Toggle auto center mode\n\n\n");
	
	sHelp->Add("   [b]Keys - Browse screen\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Enter directory/Add item to playlist and play\n");
	sHelp->Add(DisplayKeys(Key.Space) + "Add item to playlist\n");
	sHelp->Add(DisplayKeys(Key.GoToParentDir) + "Go to parent directory\n");
	sHelp->Add(DisplayKeys(Key.Delete) + "Delete playlist\n\n\n");
	
	sHelp->Add("   [b]Keys - Search engine\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Change option/Add to playlist and play\n");
	sHelp->Add(DisplayKeys(Key.Space) + "Add item to playlist\n\n\n");
	
	sHelp->Add("   [b]Keys - Media library\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(&Key.VolumeDown[0], 1) + "Previous column\n");
	sHelp->Add(DisplayKeys(&Key.VolumeUp[0], 1) + "Next column\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Add to playlist and play song/album/artist's songs\n");
	sHelp->Add(DisplayKeys(Key.Space) + "Add to playlist song/album/artist's songs\n\n\n");
	
#	ifdef HAVE_TAGLIB_H
	sHelp->Add("   [b]Keys - Tag editor\n -----------------------------------------[/b]\n");
	sHelp->Add(DisplayKeys(Key.Enter) + "Change option\n");
#	else
	sHelp->Add("   [b]Keys - Tag info\n -----------------------------------------[/b]\n");
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
	wFooter->Display();
	
	wCurrent = mPlaylist;
	current_screen = csPlaylist;
	
	wCurrent->Display();
	
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
					ncmpcpp_string_t wbrowseddir = NCMPCPP_TO_WSTRING(browsed_dir);
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
				wHeader->WriteXY(0, 0, max_allowed_title_length, "[b]1:[/b]Help  [b]2:[/b]Playlist  [b]3:[/b]Browse  [b]4:[/b]Search  [b]5:[/b]Library [b]6:[/b]Playlist editor", 1);
		
			wHeader->SetColor(Config.volume_color);
			wHeader->WriteXY(max_allowed_title_length, 0, volume_state);
			wHeader->SetColor(Config.header_color);
		}
		
		// media library stuff
		
		if (current_screen == csLibrary)
		{
			if (mLibArtists->Empty())
			{
				mLibAlbums->Clear(0);
				vArtists.clear();
				Mpd->GetArtists(vArtists);
				sort(vArtists.begin(), vArtists.end(), CaseInsensitiveComparison);
				for (TagList::const_iterator it = vArtists.begin(); it != vArtists.end(); it++)
					mLibArtists->AddOption(*it);
				mLibArtists->Window::Clear();
				mLibArtists->Refresh();
			}
			
			if (mLibAlbums->Empty())
			{
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
				FreeSongList(vLibSongs);
				if (mLibAlbums->Empty())
				{
					mLibAlbums->WriteXY(0, 0, "No albums found.");
					mLibSongs->Clear(0);
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption());
					Mpd->CommitSearch(vLibSongs);
				}
				else
				{
					mLibSongs->Clear(0);
					Mpd->StartSearch(1);
					Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption());
					Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, mLibAlbums->GetCurrentOption());
					Mpd->CommitSearch(vLibSongs);
					if (vLibSongs.empty())
					{
						const string &album = mLibAlbums->GetCurrentOption();
						Mpd->StartSearch(1);
						Mpd->AddSearch(MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption());
						Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, album.substr(7, album.length()-7));
						Mpd->CommitSearch(vLibSongs);
					}
				}
				sort(vLibSongs.begin(), vLibSongs.end(), SortSongsByTrack);
				bool bold = 0;
			
				for (SongList::const_iterator it = vLibSongs.begin(); it != vLibSongs.end(); it++)
				{
					for (SongList::const_iterator j = vPlaylist.begin(); j != vPlaylist.end(); j++)
					{
						if ((*it)->GetHash() == (*j)->GetHash())
						{
							bold = 1;
							break;
						}
					}
					bold ? mLibSongs->AddBoldOption(DisplaySong(**it, Config.song_library_format)) : mLibSongs->AddOption(DisplaySong(**it, Config.song_library_format));
					bold = 0;
				}
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
		
			if (wCurrent == mPlaylistList && mPlaylistEditor->Empty())
			{
				FreeSongList(vPlaylistContent);
				Mpd->GetPlaylistContent(mPlaylistList->GetCurrentOption(), vPlaylistContent);
				for (SongList::const_iterator it = vPlaylistContent.begin(); it != vPlaylistContent.end(); it++)
					mPlaylistEditor->AddOption(DisplaySong(**it));
				mPlaylistEditor->Window::Clear();
				mPlaylistEditor->Refresh();
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
			if (wCurrent == mLibArtists)
			{
				wCurrent->Timeout(50);
				while (Keypressed(input, Key.Up))
				{
					TraceMpdStatus();
					wCurrent->Go(UP);
					wCurrent->Refresh();
					wCurrent->ReadKey(input);
				}
				wCurrent->Timeout(ncmpcpp_window_timeout);
			}
			else
				wCurrent->Go(UP);
		}
		else if (Keypressed(input, Key.Down))
		{
			if (wCurrent == mLibArtists)
			{
				wCurrent->Timeout(50);
				while (Keypressed(input, Key.Down))
				{
					TraceMpdStatus();
					wCurrent->Go(DOWN);
					wCurrent->Refresh();
					wCurrent->ReadKey(input);
				}
				wCurrent->Timeout(ncmpcpp_window_timeout);
			}
			else
				wCurrent->Go(DOWN);
		}
		else if (Keypressed(input, Key.PageUp))
			wCurrent->Go(PAGE_UP);
		else if (Keypressed(input, Key.PageDown))
			wCurrent->Go(PAGE_DOWN);
		else if (Keypressed(input, Key.Home))
			wCurrent->Go(HOME);
		else if (Keypressed(input, Key.End))
			wCurrent->Go(END);
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
				wCurrent->Hide();
			wCurrent->Display(redraw_me);
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
						Mpd->PlayID(vPlaylist[mPlaylist->GetChoice()-1]->GetID());
					break;
				}
				case csBrowser:
				{
					GO_TO_PARENT_DIR:
					
					int ci = mBrowser->GetChoice()-1;
					switch (vBrowser[ci].type)
					{
						case itDirectory:
						{
							found_pos = 0;
							vFoundPositions.clear();
							if (browsed_dir != "/" && ci == 0)
							{
								int i = browsed_dir.size();
								while (browsed_dir[--i] != '/');
								string tmp = browsed_dir.substr(0, i);
								if (tmp != browsed_dir)
								{
									browsed_subdir = browsed_dir.substr(i+1, browsed_dir.size()-i-1);
									GetDirectory(tmp);
								}
								else
								{
									browsed_subdir = tmp;
									GetDirectory("/");
								}
							}
							else
							{
								if (browsed_dir != "/")
									GetDirectory(browsed_dir + "/" + vBrowser[ci].name);
								else
									GetDirectory(vBrowser[ci].name);
							}
							break;
						}
						case itSong:
						{
							Song &s = *vBrowser[ci].song;
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
							Mpd->GetPlaylistContent(vBrowser[ci].name, list);
							for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
								Mpd->QueueAddSong(**it);
							if (Mpd->CommitQueue())
							{
								ShowMessage("Loading and playing playlist " + vBrowser[ci].name + "...");
								Song *s = vPlaylist[vPlaylist.size()-list.size()];
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
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]New title:[/b] ", 1);
							if (s.GetTitle() == UNKNOWN_TITLE)
								s.SetTitle(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetTitle(wFooter->GetString(s.GetTitle(), TraceMpdStatus));
							mTagEditor->UpdateOption(option, "[b]Title:[/b] " + s.GetTitle());
							break;
						}
						case 2:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]New artist:[/b] ", 1);
							if (s.GetArtist() == UNKNOWN_ARTIST)
								s.SetArtist(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetArtist(wFooter->GetString(s.GetArtist(), TraceMpdStatus));
							mTagEditor->UpdateOption(option, "[b]Artist:[/b] " + s.GetArtist());
							break;
						}
						case 3:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]New album:[/b] ", 1);
							if (s.GetAlbum() == UNKNOWN_ALBUM)
								s.SetAlbum(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetAlbum(wFooter->GetString(s.GetAlbum(), TraceMpdStatus));
							mTagEditor->UpdateOption(option, "[b]Album:[/b] " + s.GetAlbum());
							break;
						}
						case 4:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]New year:[/b] ", 1);
							if (s.GetYear() == EMPTY_TAG)
								s.SetYear(wFooter->GetString(4, TraceMpdStatus));
							else
								s.SetYear(wFooter->GetString(s.GetYear(), 4, TraceMpdStatus));
							mTagEditor->UpdateOption(option, "[b]Year:[/b] " + s.GetYear());
							break;
						}
						case 5:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]New track:[/b] ", 1);
							if (s.GetTrack() == EMPTY_TAG)
								s.SetTrack(wFooter->GetString(3, TraceMpdStatus));
							else
								s.SetTrack(wFooter->GetString(s.GetTrack(), 3, TraceMpdStatus));
							mTagEditor->UpdateOption(option, "[b]Track:[/b] " + s.GetTrack());
							break;
						}
						case 6:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]New genre:[/b] ", 1);
							if (s.GetGenre() == EMPTY_TAG)
								s.SetGenre(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetGenre(wFooter->GetString(s.GetGenre(), TraceMpdStatus));
							mTagEditor->UpdateOption(option, "[b]Genre:[/b] " + s.GetGenre());
							break;
						}
						case 7:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]New comment:[/b] ", 1);
							if (s.GetComment() == EMPTY_TAG)
								s.SetComment(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetComment(wFooter->GetString(s.GetComment(), TraceMpdStatus));
							mTagEditor->UpdateOption(option, "[b]Comment:[/b] " + s.GetComment());
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
								f.tag()->setTitle(NCMPCPP_TO_WSTRING(s.GetTitle()));
								f.tag()->setArtist(NCMPCPP_TO_WSTRING(s.GetArtist()));
								f.tag()->setAlbum(NCMPCPP_TO_WSTRING(s.GetAlbum()));
								f.tag()->setYear(StrToInt(s.GetYear()));
								f.tag()->setTrack(StrToInt(s.GetTrack()));
								f.tag()->setGenre(NCMPCPP_TO_WSTRING(s.GetGenre()));
								f.tag()->setComment(NCMPCPP_TO_WSTRING(s.GetComment()));
								s.GetEmptyFields(0);
								f.save();
								ShowMessage("Tags updated!");
								Mpd->UpdateDirectory(s.GetDirectory());
								if (prev_screen == csSearcher)
								{
									int upid = mSearcher->GetChoice()-search_engine_static_option-1;
									*vSearched[upid] = s;
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
#								ifdef HAVE_TAGLIB_H
								if (id == 8)
								{
									mLibSongs->HighlightColor(Config.main_highlight_color);
									mLibArtists->HighlightColor(Config.active_column_color);
									wCurrent = mLibArtists;
								}
								else
									wCurrent = wPrev;
#								else
								wCurrent = wPrev;
#								endif
								REFRESH_MEDIA_LIBRARY_SCREEN;
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
					int id = mSearcher->GetChoice();
					int option = mSearcher->GetChoice();
					LOCK_STATUSBAR;
					Song &s = searched_song;
					
					switch (id)
					{
						case 1:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]Filename:[/b] ", 1);
							if (s.GetShortFilename() == EMPTY_TAG)
								s.SetShortFilename(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetShortFilename(wFooter->GetString(s.GetShortFilename(), TraceMpdStatus));
							mSearcher->UpdateOption(option, "[b]Filename:[/b] " + s.GetShortFilename());
							break;
						}
						case 2:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]Title:[/b] ", 1);
							if (s.GetTitle() == UNKNOWN_TITLE)
								s.SetTitle(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetTitle(wFooter->GetString(s.GetTitle(), TraceMpdStatus));
							mSearcher->UpdateOption(option, "[b]Title:[/b] " + s.GetTitle());
							break;
						}
						case 3:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]Artist:[/b] ", 1);
							if (s.GetArtist() == UNKNOWN_ARTIST)
								s.SetArtist(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetArtist(wFooter->GetString(s.GetArtist(), TraceMpdStatus));
							mSearcher->UpdateOption(option, "[b]Artist:[/b] " + s.GetArtist());
							break;
						}
						case 4:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]Album:[/b] ", 1);
							if (s.GetAlbum() == UNKNOWN_ALBUM)
								s.SetAlbum(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetAlbum(wFooter->GetString(s.GetAlbum(), TraceMpdStatus));
							mSearcher->UpdateOption(option, "[b]Album:[/b] " + s.GetAlbum());
							break;
						}
						case 5:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]Year:[/b] ", 1);
							if (s.GetYear() == EMPTY_TAG)
								s.SetYear(wFooter->GetString(4, TraceMpdStatus));
							else
								s.SetYear(wFooter->GetString(s.GetYear(), 4, TraceMpdStatus));
							mSearcher->UpdateOption(option, "[b]Year:[/b] " + s.GetYear());
							break;
						}
						case 6:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]Track:[/b] ", 1);
							if (s.GetTrack() == EMPTY_TAG)
								s.SetTrack(wFooter->GetString(3, TraceMpdStatus));
							else
								s.SetTrack(wFooter->GetString(s.GetTrack(), 3, TraceMpdStatus));
							mSearcher->UpdateOption(option, "[b]Track:[/b] " + s.GetTrack());
							break;
						}
						case 7:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]Genre:[/b] ", 1);
							if (s.GetGenre() == EMPTY_TAG)
								s.SetGenre(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetGenre(wFooter->GetString(s.GetGenre(), TraceMpdStatus));
							mSearcher->UpdateOption(option, "[b]Genre:[/b] " + s.GetGenre());
							break;
						}
						case 8:
						{
							wFooter->WriteXY(0, Config.statusbar_visibility, "[b]Comment:[/b] ", 1);
							if (s.GetComment() == EMPTY_TAG)
								s.SetComment(wFooter->GetString("", TraceMpdStatus));
							else
								s.SetComment(wFooter->GetString(s.GetComment(), TraceMpdStatus));
							mSearcher->UpdateOption(option, "[b]Comment:[/b] " + s.GetComment());
							break;
						}
						case 10:
						{
							search_mode_match = !search_mode_match;
							mSearcher->UpdateOption(option, "[b]Search mode:[/b] " + (search_mode_match ? search_mode_one : search_mode_two));
							break;
						}
						case 11:
						{
							search_case_sensitive = !search_case_sensitive;
							mSearcher->UpdateOption(option, "[b]Case sensitive:[/b] " + (string)(search_case_sensitive ? "Yes" : "No"));
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
								mSearcher->AddStaticBoldOption("[white]Search results:[/white] [green]Found " + IntoStr(vSearched.size()) + (vSearched.size() > 1 ? " songs" : " song") + "[/green]");
								mSearcher->AddSeparator();
									
								for (SongList::const_iterator it = vSearched.begin(); it != vSearched.end(); it++)
								{	
									for (SongList::const_iterator j = vPlaylist.begin(); j != vPlaylist.end(); j++)
									{
										if ((*j)->GetHash() == (*it)->GetHash())
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
								mSearcher->Go(DOWN);
								mSearcher->Go(DOWN);
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
							Song *s = vPlaylist[vPlaylist.size()-list.size()];
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
						for (SongList::const_iterator it = vLibSongs.begin(); it != vLibSongs.end(); it++)
							Mpd->QueueAddSong(**it);
						if (Mpd->CommitQueue())
						{
							ShowMessage("Adding songs from: " + mLibArtists->GetCurrentOption() + " \"" + mLibAlbums->GetCurrentOption() + "\"");
							Song *s = vPlaylist[vPlaylist.size()-vLibSongs.size()];
							if (s->GetHash() == vLibSongs[0]->GetHash())
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
						if (!vLibSongs.empty())
						{
							Song &s = *vLibSongs[mLibSongs->GetChoice()-1];
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
						wCurrent->Go(DOWN);
					
					break;
				}
				default:
					break;
			}
		}
		else if (Keypressed(input, Key.Space))
		{
			if (Config.space_selects || wCurrent == mPlaylist || wCurrent == mPlaylistEditor)
			{
				if (wCurrent == mPlaylist || (wCurrent == mBrowser && wCurrent->GetChoice() > (browsed_dir != "/" ? 1 : 0)) || (wCurrent == mSearcher && !vSearched.empty() && wCurrent->GetChoice() > search_engine_static_option) || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
				{
					Menu *mCurrent = static_cast<Menu *>(wCurrent);
					int i = mCurrent->GetChoice();
					mCurrent->Select(i, !mCurrent->Selected(i));
					mCurrent->Go(DOWN);
				}
			}
			else
			{
				if (current_screen == csBrowser)
				{
					int ci = mBrowser->GetChoice()-1;
					switch (vBrowser[ci].type)
					{
						case itDirectory:
						{
							string getdir = browsed_dir == "/" ? vBrowser[ci].name : browsed_dir + "/" + vBrowser[ci].name;
						
							SongList list;
							Mpd->GetDirectoryRecursive(getdir, list);
						
							for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
								Mpd->QueueAddSong(**it);
							if (Mpd->CommitQueue())
							{
								ShowMessage("Added folder: " + getdir);
								Song *s = vPlaylist[vPlaylist.size()-list.size()];
								if (s->GetHash() != list[0]->GetHash())
									ShowMessage(message_part_of_songs_added);
							}
							FreeSongList(list);
							break;
						}
						case itSong:
						{
							Song &s = *vBrowser[ci].song;
							if (Mpd->AddSong(s) != -1)
								ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
							break;
						}
						case itPlaylist:
						{
							SongList list;
							Mpd->GetPlaylistContent(vBrowser[ci].name, list);
							for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
								Mpd->QueueAddSong(**it);
							if (Mpd->CommitQueue())
							{
								ShowMessage("Loading playlist " + vBrowser[ci].name + "...");
								Song *s = vPlaylist[vPlaylist.size()-list.size()];
								if (s->GetHash() != list[0]->GetHash())
									ShowMessage(message_part_of_songs_added);
							}
							FreeSongList(list);
							break;
						}
					}
					mBrowser->Go(DOWN);
				}
				else if (current_screen == csSearcher && !vSearched.empty())
				{
					int id = mSearcher->GetChoice()-search_engine_static_option-1;
					if (id < 0)
						continue;
				
					Song &s = *vSearched[id];
					if (Mpd->AddSong(s) != -1)
						ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
					mSearcher->Go(DOWN);
				}
				else if (current_screen == csLibrary)
					goto ENTER_LIBRARY_SCREEN; // sorry, but that's stupid to copy the same code here.
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
					while (!vPlaylist.empty() && Keypressed(input, Key.Delete))
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
				Menu *mCurrent = static_cast<Menu *>(wCurrent);
				int id = mCurrent->GetChoice()-1;
				const string &name = wCurrent == mBrowser ? vBrowser[id].name : mPlaylistList->GetCurrentOption();
				if (current_screen != csBrowser || vBrowser[id].type == itPlaylist)
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
					while (!vPlaylistContent.empty() && Keypressed(input, Key.Delete))
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
			Mpd->Prev();
		else if (Keypressed(input, Key.Next))
			Mpd->Next();
		else if (Keypressed(input, Key.Pause))
			Mpd->Pause();
		else if (Keypressed(input, Key.SavePlaylist))
		{
			LOCK_STATUSBAR;
			wFooter->WriteXY(0, Config.statusbar_visibility, "Save playlist as: ", 1);
			string playlist_name = wFooter->GetString("", TraceMpdStatus);
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
					while (in != 'y' && in != 'n')
						wFooter->ReadKey(in);
					
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
			if (browsed_dir == "/" && !vBrowser.empty())
				GetDirectory(browsed_dir);
		}
		else if (Keypressed(input, Key.Stop))
			Mpd->Stop();
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
							mPlaylist->Go(DOWN);
							break;
						}
					}
				}
				else
					if (MoveSongUp(mPlaylist->GetChoice()-1))
						mPlaylist->Go(UP);
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
							mPlaylistEditor->Go(DOWN);
							break;
						}
					}
				}
				else
					if (PlaylistMoveSongUp(mPlaylistList->GetCurrentOption(), mPlaylistEditor->GetChoice()-1))
						mPlaylistEditor->Go(UP);
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
							mPlaylist->Go(UP);
							break;
						}
					}
				}
				else
					if (MoveSongDown(mPlaylist->GetChoice()-1))
						mPlaylist->Go(DOWN);
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
							mPlaylistEditor->Go(UP);
							break;
						}
					}
				}
				else
					if (PlaylistMoveSongDown(mPlaylistList->GetCurrentOption(), mPlaylistEditor->GetChoice()-1))
						mPlaylistEditor->Go(DOWN);
			}
		}
		else if (Keypressed(input, Key.Add))
		{
			LOCK_STATUSBAR;
			wFooter->WriteXY(0, Config.statusbar_visibility, "Add: ", 1);
			string path = wFooter->GetString("", TraceMpdStatus);
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
						Song *s = vPlaylist[vPlaylist.size()-list.size()];
						if (s->GetHash() != list[0]->GetHash())
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
			if (!vPlaylist[now_playing]->GetTotalLength())
			{
				ShowMessage("Unknown item length!");
				continue;
			}
			block_progressbar_update = 1;
			LOCK_STATUSBAR;
			
			int songpos, in;
			
			songpos = Mpd->GetElapsedTime();
			Song &s = *vPlaylist[now_playing];
			
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
			Mpd->SetRepeat(!Mpd->GetRepeat());
		else if (Keypressed(input, Key.ToggleRepeatOne))
		{
			Config.repeat_one_mode = !Config.repeat_one_mode;
			ShowMessage("'Repeat one' mode: " + string(Config.repeat_one_mode ? "On" : "Off"));
		}
		else if (Keypressed(input, Key.Shuffle))
			Mpd->Shuffle();
		else if (Keypressed(input, Key.ToggleRandom))
			Mpd->SetRandom(!Mpd->GetRandom());
		else if (Keypressed(input, Key.ToggleCrossfade))
			Mpd->SetCrossfade(Mpd->GetCrossfade() ? 0 : Config.crossfade_time);
		else if (Keypressed(input, Key.SetCrossfade))
		{
			LOCK_STATUSBAR;
			wFooter->WriteXY(0, Config.statusbar_visibility, "Set crossfade to: ", 1);
			string crossfade = wFooter->GetString(3, TraceMpdStatus);
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
			if ((wCurrent == mPlaylist && !vPlaylist.empty())
			||  (wCurrent == mBrowser && vBrowser[mBrowser->GetChoice()-1].type == itSong)
			||  (wCurrent == mSearcher && !vSearched.empty() && mSearcher->GetChoice() > search_engine_static_option)
			||  (wCurrent == mLibSongs && !vLibSongs.empty()))
			{
				int id = wCurrent->GetChoice()-1;
				Song *s;
				switch (current_screen)
				{
					case csPlaylist:
						s = vPlaylist[id];
						break;
					case csBrowser:
						s = vBrowser[id].song;
						break;
					case csSearcher:
						s = vSearched[id-search_engine_static_option];
						break;
					case csLibrary:
						s = vLibSongs[id];
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
		else if (Keypressed(input, Key.GoToPosition))
		{
			if (now_playing < 0)
				continue;
			if (!vPlaylist[now_playing]->GetTotalLength())
			{
				ShowMessage("Unknown item length!");
				continue;
			}
			LOCK_STATUSBAR;
			wFooter->WriteXY(0, Config.statusbar_visibility, "Position to go (in %): ", 1);
			string position = wFooter->GetString(3, TraceMpdStatus);
			int newpos = atoi(position.c_str());
			if (newpos > 0 && newpos < 100 && !position.empty())
				Mpd->Seek(vPlaylist[now_playing]->GetTotalLength()*newpos/100.0);
			UNLOCK_STATUSBAR;
		}
		else if (Keypressed(input, Key.ReverseSelection))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || (wCurrent == mSearcher && !vSearched.empty()) || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
			{
				Menu *mCurrent = static_cast<Menu *>(wCurrent);
				for (int i = 1; i <= mCurrent->MaxChoice(); i++)
					mCurrent->Select(i, !mCurrent->Selected(i) && !mCurrent->IsStatic(i));
				// hackish shit begins
				if (mCurrent == mBrowser && browsed_dir != "/")
					mCurrent->Select(1, 0); // [..] cannot be selected, uhm.
				if (mCurrent == mSearcher)
					mCurrent->Select(14, 0); // 'Reset' cannot be selected, omgplz.
				// hacking shit ends. need better solution :/
				ShowMessage("Selection reversed!");
			}
		}
		else if (Keypressed(input, Key.DeselectAll))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || wCurrent == mSearcher || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
			{
				Menu *mCurrent = static_cast<Menu *>(wCurrent);
				if (mCurrent->IsAnySelected())
				{
					for (int i = 1; i <= mCurrent->MaxChoice(); i++)
						mCurrent->Select(i, 0);
					ShowMessage("Items deselected!");
				}
			}
		}
		else if (Keypressed(input, Key.AddSelected))
		{
			if (wCurrent == mPlaylist || wCurrent == mBrowser || wCurrent == mSearcher || wCurrent == mLibSongs || wCurrent == mPlaylistEditor)
			{
				Menu *mCurrent = static_cast<Menu *>(wCurrent);
				if (mCurrent->IsAnySelected())
				{
					vector<int> list;
					mCurrent->GetSelectedList(list);
					SongList result;
					for (vector<int>::const_iterator it = list.begin(); it != list.end(); it++)
					{
						switch (current_screen)
						{
							case csPlaylist:
							{
								Song *s = new Song(*vPlaylist[*it-1]);
								result.push_back(s);
								break;
							}
							case csBrowser:
							{
								switch (vBrowser[*it-1].type)
								{
									case itDirectory:
									{
										if (browsed_dir != "/")
											Mpd->GetDirectoryRecursive(browsed_dir + "/" + vBrowser[*it-1].name, result);
										else
											Mpd->GetDirectoryRecursive(vBrowser[*it-1].name, result);
										break;
									}
									case itSong:
									{
										Song *s = new Song(*vBrowser[*it-1].song);
										result.push_back(s);
										break;
									}
									case itPlaylist:
									{
										Mpd->GetPlaylistContent(vBrowser[*it-1].name, result);
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
								Song *s = new Song(*vLibSongs[*it-1]);
								result.push_back(s);
								break;
							}
							case csPlaylistEditor:
							{
								Song *s = new Song(*vPlaylistContent[*it-1]);
								result.push_back(s);
								break;
							}
							default:
								break;
						}
					}
					
					Menu *mDialog = new Menu((COLS-50)/2, (LINES-10)/2, 50, 10, "Add selected items to...", clYellow, brGreen);
					mDialog->Timeout(ncmpcpp_window_timeout);
					
					mDialog->AddOption("Current MPD playlist");
					mDialog->AddSeparator();
					TagList playlists;
					Mpd->GetPlaylists(playlists);
					for (TagList::const_iterator it = playlists.begin(); it != playlists.end(); it++)
						mDialog->AddOption(*it);
					mDialog->AddSeparator();
					mDialog->AddOption("Cancel");
					
					mDialog->Display();
					
					while (!Keypressed(input, Key.Enter))
					{
						mDialog->Refresh();
						mDialog->ReadKey(input);
						
						if (Keypressed(input, Key.Up))
							mDialog->Go(UP);
						else if (Keypressed(input, Key.Down))
							mDialog->Go(DOWN);
						else if (Keypressed(input, Key.PageUp))
							mDialog->Go(PAGE_UP);
						else if (Keypressed(input, Key.PageDown))
							mDialog->Go(PAGE_DOWN);
						else if (Keypressed(input, Key.Home))
							mDialog->Go(HOME);
						else if (Keypressed(input, Key.End))
							mDialog->Go(END);
					}
					
					int id = mDialog->GetChoice();
					
					if (id == 1)
					{
						for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
							Mpd->QueueAddSong(**it);
						if (Mpd->CommitQueue())
						{
							ShowMessage("Selected items added!");
							Song *s = vPlaylist[vPlaylist.size()-result.size()];
							if (s->GetHash() != result[0]->GetHash())
								ShowMessage(message_part_of_songs_added);
						}
					}
					else if (id > 1 && id < mDialog->MaxChoice())
					{
						for (SongList::const_iterator it = result.begin(); it != result.end(); it++)
							Mpd->QueueAddToPlaylist(mDialog->GetCurrentOption(), **it);
						Mpd->CommitQueue();
						ShowMessage("Selected items added to playlist '" + mDialog->GetCurrentOption() + "'!");
					}
					
					redraw_me = 1;
					mPlaylistEditor->Clear(0); // make playlist editor update itself
					if (current_screen == csLibrary)
					{
						REFRESH_MEDIA_LIBRARY_SCREEN;
					}
					else if (current_screen == csPlaylistEditor)
					{
						REFRESH_PLAYLIST_EDITOR_SCREEN;
					}
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
				for (int i = 0; i < mPlaylist->MaxChoice(); i++)
				{
					if (!mPlaylist->Selected(i+1) && i != now_playing)
						Mpd->QueueDeleteSongId(vPlaylist[i]->GetID());
				}
				// if mpd deletes now playing song deletion will be sluggishly slow
				// then so we have to assure it will be deleted at the very end.
				if (!mPlaylist->Selected(now_playing+1))
					Mpd->QueueDeleteSongId(vPlaylist[now_playing]->GetID());
				
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
				for (SongList::iterator it = vPlaylist.begin(); it != vPlaylist.begin()+now_playing; it++)
					Mpd->QueueDeleteSongId((*it)->GetID());
				for (SongList::iterator it = vPlaylist.begin()+now_playing+1; it != vPlaylist.end(); it++)
					Mpd->QueueDeleteSongId((*it)->GetID());
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
				Menu *mCurrent = static_cast<Menu *>(wCurrent);
				LOCK_STATUSBAR;
				wFooter->WriteXY(0, Config.statusbar_visibility, "Find " + how + ": ", 1);
				string findme = wFooter->GetString("", TraceMpdStatus);
				UNLOCK_STATUSBAR;
				timer = time(NULL);
				if (findme.empty())
					continue;
				transform(findme.begin(), findme.end(), findme.begin(), tolower);
				
				for (int i = 1; i <= mCurrent->MaxChoice(); i++)
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
					mCurrent->Highlight(vFoundPositions[found_pos < 0 ? 0 : found_pos]);
					mCurrent->Highlighting(1);
				}
			}
		}
		else if (Keypressed(input, Key.NextFoundPosition) || Keypressed(input, Key.PrevFoundPosition))
		{
			if (!vFoundPositions.empty())
			{
				Menu *mCurrent = static_cast<Menu *>(wCurrent);
				try
				{
					mCurrent->Highlight(vFoundPositions.at(Keypressed(input, Key.NextFoundPosition) ? ++found_pos : --found_pos));
				}
				catch (std::out_of_range)
				{
					if (Config.wrapped_search)
					{
						if (Keypressed(input, Key.NextFoundPosition))
						{
							mCurrent->Highlight(vFoundPositions.front());
							found_pos = 0;
						}
						else
						{
							mCurrent->Highlight(vFoundPositions.back());
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
				wCurrent->Hide();
				current_screen = prev_screen;
				wCurrent = wPrev;
				redraw_me = 1;
				if (current_screen == csLibrary)
				{
					REFRESH_MEDIA_LIBRARY_SCREEN;
				}
				continue;
			}
			if ((wCurrent == mPlaylist && !vPlaylist.empty())
			||  (wCurrent == mBrowser && vBrowser[mBrowser->GetChoice()-1].type == itSong)
			||  (wCurrent == mSearcher && !vSearched.empty() && mSearcher->GetChoice() > search_engine_static_option)
			||  (wCurrent == mLibSongs && !vLibSongs.empty()))
			{
				Song *s;
				switch (current_screen)
				{
					case csPlaylist:
						s = vPlaylist[mPlaylist->GetChoice()-1];
						break;
					case csBrowser:
						s = vBrowser[mBrowser->GetChoice()-1].song;
						break;
					case csSearcher:
						s = vSearched[mSearcher->GetRealChoice()-2]; // first one is 'Reset'
						break;
					case csLibrary:
						s = vLibSongs[mLibSongs->GetChoice()-1];
						break;
					default:
						break;
				}
				
				if (s->GetArtist() != UNKNOWN_ARTIST && s->GetTitle() != UNKNOWN_TITLE)
				{
					wPrev = wCurrent;
					prev_screen = current_screen;
					wCurrent = sLyrics;
					wCurrent->Hide();
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
			
			if (mBrowser->Empty())
				GetDirectory(browsed_dir);
			else
			{
				bool bold = 0;
				for (int i = 0; i < vBrowser.size(); i++)
				{
					if (vBrowser[i].type == itSong)
					{
						for (SongList::const_iterator it = vPlaylist.begin(); it != vPlaylist.end(); it++)
						{
							if ((*it)->GetHash() == vBrowser[i].song->GetHash())
							{
								bold = 1;
								break;
							}
						}
						mBrowser->BoldOption(i+1, bold);
						bold = 0;
					}
				}
			}
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
					bool bold = 0;
					int i = search_engine_static_option;
					for (SongList::const_iterator it = vSearched.begin(); it != vSearched.end(); it++, i++)
					{
						for (SongList::const_iterator j = vPlaylist.begin(); j != vPlaylist.end(); j++)
						{
							if ((*j)->GetHash() == (*it)->GetHash())
							{
								bold = 1;
								break;
							}
						}
						mSearcher->BoldOption(i+1, bold);
						bold = 0;
					}
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
				
				if (!vLibSongs.empty())
				{
					bool bold = 0;
					for (int i = 0; i < vLibSongs.size(); i++)
					{
						for (SongList::const_iterator it = vPlaylist.begin(); it != vPlaylist.end(); it++)
						{
							if ((*it)->GetHash() == vLibSongs[i]->GetHash())
							{
								bold = 1;
								break;
							}
						}
						mLibSongs->BoldOption(i+1, bold);
						bold = 0;
					}
					mLibSongs->Refresh();
				}
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

