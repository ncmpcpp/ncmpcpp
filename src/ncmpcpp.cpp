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
#include "status_checker.h"
#include "helpers.h"
#include "settings.h"
#include "song.h"
#include "lyrics.h"

#define FOR_EACH_MPD_DATA(x) for (; (x); (x) = mpd_data_get_next(x))

#define BLOCK_STATUSBAR_UPDATE \
			if (Config.statusbar_visibility) \
				block_statusbar_update = 1; \
			else \
				block_progressbar_update = 1; \
			allow_statusbar_unblock = 0

#define UNBLOCK_STATUSBAR_UPDATE \
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

#ifdef HAVE_TAGLIB_H
 const string tag_screen = "Tag editor";
 const string tag_screen_keydesc = "\tE e       : Edit song's tags\n";
#else
 const string tag_screen = "Tag info";
 const string tag_screen_keydesc = "\tE e       : Show song's tags\n";
#endif

char *MPD_HOST = getenv("MPD_HOST");
int MPD_PORT = getenv("MPD_PORT") ? atoi(getenv("MPD_PORT")) : 6600;
char *MPD_PASSWORD = getenv("MPD_PASSWORD");

ncmpcpp_config Config;

vector<Song *> vPlaylist;
vector<Song> vSearched;
vector<BrowsedItem> vBrowser;

vector<string> vArtists;
vector<Song> vSongs;

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
Scrollpad *sHelp;
Scrollpad *sLyrics;

Window *wHeader;
Window *wFooter;

MpdObj *conn;
MpdData *playlist;
MpdData *browser;

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
bool block_library_update = 0;

bool search_case_sensitive = 1;
bool search_mode_match = 1;

bool redraw_me = 0;

extern string EMPTY_TAG;
extern string UNKNOWN_ARTIST;
extern string UNKNOWN_TITLE;
extern string UNKNOWN_ALBUM;

extern string playlist_stats;

int main(int argc, char *argv[])
{
	DefaultConfiguration(Config);
	ReadConfiguration(Config);
	DefineEmptyTags();
	
	conn = mpd_new(MPD_HOST, MPD_PORT, MPD_PASSWORD);
	
	mpd_set_connection_timeout(conn, Config.mpd_connection_timeout);
	
	if (mpd_connect(conn) != MPD_OK)
	{
		printf("Cannot connect to mpd.\n");
		return -1;
	}
	mpd_send_password(conn);
	
	setlocale(LC_ALL,"");
	initscr();
	noecho();
	cbreak();
	curs_set(0);
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
	mBrowser = new Menu(mPlaylist->EmptyClone());
	mTagEditor = new Menu(mPlaylist->EmptyClone());
	mSearcher = new Menu(mPlaylist->EmptyClone());
	
	int lib_artist_width = COLS/3-1;
	int lib_albums_width = COLS/3;
	int lib_albums_start_x = lib_artist_width+1;
	int lib_songs_width = COLS-COLS/3*2-1;
	int lib_songs_start_x = lib_artist_width+lib_albums_width+2;
	
	mLibArtists = new Menu(0, main_start_y, lib_artist_width, main_height, "Artists", Config.main_color, brNone);
	mLibAlbums = new Menu(lib_albums_start_x, main_start_y, lib_albums_width, main_height, "Albums", Config.main_color, brNone);
	mLibSongs = new Menu(lib_songs_start_x, main_start_y, lib_songs_width, main_height, "Songs", Config.main_color, brNone);
	
	sHelp = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	sLyrics = new Scrollpad(sHelp->EmptyClone());
	
	sHelp->Add("   [b]Keys - Movement\n -----------------------------------------[/b]\n");
	sHelp->Add("\tUp        : Move Cursor up\n");
	sHelp->Add("\tDown      : Move Cursor down\n");
	sHelp->Add("\tPage up   : Page up\n");
	sHelp->Add("\tPage down : Page down\n");
	sHelp->Add("\tHome      : Home\n");
	sHelp->Add("\tEnd       : End\n\n");
	
	sHelp->Add("\t1         : Help screen\n");
	sHelp->Add("\t2         : Playlist screen\n");
	sHelp->Add("\t3         : Browse screen\n");
	sHelp->Add("\t4         : Search engine\n");
	sHelp->Add("\t5         : Media library\n\n\n");
	
	sHelp->Add("   [b]Keys - Global\n -----------------------------------------[/b]\n");
	sHelp->Add("\ts         : Stop\n");
	sHelp->Add("\tP         : Pause\n");
	sHelp->Add("\t>         : Next track\n");
	sHelp->Add("\t<         : Previous track\n");
	sHelp->Add("\tf         : Seek forward\n");
	sHelp->Add("\tb         : Seek backward\n");
	sHelp->Add("\t- Left    : Decrease volume\n");
	sHelp->Add("\t+ Right   : Increase volume\n\n");
	
	sHelp->Add("\tr         : Toggle repeat mode\n");
	sHelp->Add("\tz         : Toggle random mode\n");
	sHelp->Add("\tx         : Toggle crossfade mode\n");
	sHelp->Add("\tZ         : Shuffle playlist\n");
	sHelp->Add("\tU u       : Start a music database update\n\n");
	
	sHelp->Add("\t/         : Forward find\n");
	sHelp->Add("\t?         : Backward find\n");
	sHelp->Add("\t.         : Go to next/previous found position\n");
	sHelp->Add(tag_screen_keydesc);
	sHelp->Add("\tg         : Go to chosen position in current song\n");
	sHelp->Add("\tl         : Show/hide song's lyrics\n\n");
	
	sHelp->Add("\tQ q       :  Quit\n\n\n");
	
	sHelp->Add("   [b]Keys - Playlist screen\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Play\n");
	sHelp->Add("\tDelete    : Delete song from playlist\n");
	sHelp->Add("\tc         : Clear whole playlist\n");
	sHelp->Add("\tC         : Clear playlist but hold currently playing song\n");
	sHelp->Add("\tm         : Move song up\n");
	sHelp->Add("\tn         : Move song down\n");
	sHelp->Add("\tS         : Save playlist\n");
	sHelp->Add("\to         : Go to currently playing position\n\n\n");
	
	sHelp->Add("   [b]Keys - Browse screen\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Enter directory/Add to playlist and play song\n");
	sHelp->Add("\tSpace     : Add song to playlist\n");
	sHelp->Add("\tDelete    : Delete playlist\n\n\n");
	
	sHelp->Add("   [b]Keys - Search engine\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Change option/Add to playlist and play song\n");
	sHelp->Add("\tSpace     : Add song to playlist\n\n\n");
	
	sHelp->Add("   [b]Keys - Media library\n -----------------------------------------[/b]\n");
	sHelp->Add("\tLeft      : Previous column\n");
	sHelp->Add("\tRight     : Next column\n");
	sHelp->Add("\tEnter     : Add to playlist and play song/album/artist's songs\n");
	sHelp->Add("\tSpace     : Add to playlist song/album/artist's songs\n\n\n");
	
#	ifdef HAVE_TAGLIB_H
	sHelp->Add("   [b]Keys - Tag editor\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Change option\n");
#	else
	sHelp->Add("   [b]Keys - Tag info\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Return\n");
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
	
	mpd_signal_connect_error(conn, (ErrorCallback)NcmpcppErrorCallback, NULL);
	mpd_signal_connect_status_changed(conn, (StatusChangedCallback)NcmpcppStatusChanged, NULL);
	
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
	
	while (!main_exit)
	{
		TraceMpdStatus();
		
		block_playlist_update = 0;
		messages_allowed = 1;
		
		if (Config.header_visibility)
		{
			string title;
			int max_allowed_title_length = wHeader->GetWidth()-volume_state.length();
			
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
					title = "Lyrics";
					break;
			}
		
			if (title_allowed)
			{
				wHeader->Bold(1);
				wHeader->WriteXY(0, 0, title, 1);
				wHeader->Bold(0);
				
				if (current_screen == csPlaylist && !playlist_stats.empty())
				{
					int max_length = wHeader->GetWidth()-volume_state.length()-title.length();
					if (playlist_stats.length() > max_length)
						wHeader->WriteXY(title.length(), 0, playlist_stats.substr(0, max_length));
					else
						wHeader->WriteXY(title.length(), 0, playlist_stats);
				}
				
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
				wHeader->WriteXY(0, 0, "[b]1:[/b]Help  [b]2:[/b]Playlist  [b]3:[/b]Browse  [b]4:[/b]Search  [b]5:[/b]Library", 1);
		
			wHeader->SetColor(Config.volume_color);
			wHeader->WriteXY(max_allowed_title_length, 0, volume_state);
			wHeader->SetColor(Config.header_color);
		}
		
		if (current_screen == csLibrary && !block_library_update)
		{
			MpdData *data;
			
			if (wCurrent == mLibAlbums && mLibAlbums->Empty())
			{
				mLibAlbums->HighlightColor(Config.main_color);
				mLibArtists->HighlightColor(Config.library_active_column_color);
				wCurrent = mLibArtists;
			}
			
			if (wCurrent == mLibArtists)
			{
				mLibAlbums->Clear();
				data = mpd_database_get_albums(conn, (char *) mLibArtists->GetCurrentOption().c_str());
				FOR_EACH_MPD_DATA(data)
						mLibAlbums->AddOption(data->tag);
				mpd_data_free(data);
				
				if (mLibAlbums->Empty())
				{
					mLibAlbums->WriteXY(0, 0, "No albums found.");
					vSongs.clear();
					mLibSongs->Clear();
					mpd_database_search_start(conn, 1);
					mpd_database_search_add_constraint(conn, MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption().c_str());
					mpd_database_search_add_constraint(conn, MPD_TAG_ITEM_ALBUM, mLibAlbums->GetCurrentOption().c_str());
					data = mpd_database_search_commit(conn);
					FOR_EACH_MPD_DATA(data)
							vSongs.push_back(data->song);
					mpd_data_free(data);
				}
			}
			
			vSongs.clear();
			mLibSongs->Clear(0);
			mpd_database_search_start(conn, 1);
			mpd_database_search_add_constraint(conn, MPD_TAG_ITEM_ARTIST, mLibArtists->GetCurrentOption().c_str());
			mpd_database_search_add_constraint(conn, MPD_TAG_ITEM_ALBUM, mLibAlbums->GetCurrentOption().c_str());
			data = mpd_database_search_commit(conn);
			FOR_EACH_MPD_DATA(data)
					vSongs.push_back(data->song);
			mpd_data_free(data);
				
			sort(vSongs.begin(), vSongs.end(), SortSongsByTrack);
				
			bool bold = 0;
			for (vector<Song>::const_iterator it = vSongs.begin(); it != vSongs.end(); it++)
			{
				for (vector<Song *>::const_iterator j = vPlaylist.begin(); j != vPlaylist.end(); j++)
				{
					if (it->GetHash() == (*j)->GetHash())
					{
						bold = 1;
						break;
					}
				}
				bold ? mLibSongs->AddBoldOption(DisplaySong(*it, Config.song_library_format)) : mLibSongs->AddOption(DisplaySong(*it, Config.song_library_format));
				bold = 0;
			}
			mLibAlbums->Refresh();
			mLibSongs->Hide();
			mLibSongs->Display();

			block_library_update = 1;
		}
		
		wCurrent->Refresh(redraw_me);
		redraw_me = 0;
		
		wCurrent->ReadKey(input);
		if (input == ERR)
			continue;
		
		if (current_screen == csLibrary)
			block_library_update = 0;
		
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
				if (input == KEY_UP || input == KEY_DOWN || input == KEY_PPAGE || input == KEY_NPAGE || input == KEY_HOME || input == KEY_END)
				{
					if (wCurrent == mLibArtists)
					{	mLibAlbums->Reset();
						mLibSongs->Reset();
					}
					if (wCurrent == mLibAlbums)
						mLibSongs->Reset();
				}
				break;
			}
		}
		
		switch (input)
		{
			case KEY_UP: wCurrent->Go(UP); continue;
			case KEY_DOWN: wCurrent->Go(DOWN); continue;
			case KEY_PPAGE: wCurrent->Go(PAGE_UP); continue;
			case KEY_NPAGE: wCurrent->Go(PAGE_DOWN); continue;
			case KEY_HOME: wCurrent->Go(HOME); continue;
			case KEY_END: wCurrent->Go(END); continue;
			case KEY_RESIZE:
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
					mLibArtists->Hide();
					mLibAlbums->Hide();
					mLibSongs->Hide();
					REFRESH_MEDIA_LIBRARY_SCREEN;
				}
				
				header_update_status = 1;
				int mpd_state = mpd_player_get_state(conn);
				if (mpd_state == MPD_PLAYER_PLAY || mpd_state == MPD_PLAYER_PAUSE)
					NcmpcppStatusChanged(conn, MPD_CST_ELAPSED_TIME); // restore status
				else
					NcmpcppStatusChanged(conn, MPD_CST_STATE);
				
				break;
			}
			case ENTER:
			{
				switch (current_screen)
				{
					case csPlaylist:
					{
						if (!mPlaylist->Empty())
							mpd_player_play_id(conn, vPlaylist[mPlaylist->GetChoice()-1]->GetID());
						break;
					}
					case csBrowser:
					{
						int ci = mBrowser->GetChoice()-1;
						switch (vBrowser[ci].type)
						{
							case MPD_DATA_TYPE_DIRECTORY:
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
									if (browsed_dir != "/")
										GetDirectory(browsed_dir + "/" + vBrowser[ci].name);
									else
										GetDirectory(vBrowser[ci].name);
								break;
							}
							case MPD_DATA_TYPE_SONG:
							{
								string &file = vBrowser[ci].name;
								mpd_Song *test = mpd_database_get_fileinfo(conn, (char *) file.c_str());
								if (test)
								{
									mpd_playlist_add(conn, (char *) file.c_str());
									Song s = test;
									if (s.GetHash() == vPlaylist.back()->GetHash())
										mpd_player_play_id(conn, vPlaylist.back()->GetID());
									ShowMessage("Added to playlist: " +  OmitBBCodes(DisplaySong(s)));
									mBrowser->Refresh();
								}
								mpd_freeSong(test);
								break;
							}
							case MPD_DATA_TYPE_PLAYLIST:
							{
								int howmany = 0;
								ShowMessage("Loading and playing playlist " + vBrowser[ci].name + "...");
								MpdData *list = mpd_database_get_playlist_content(conn, (char *) vBrowser[ci].name.c_str());
								Song tmp;
								if (list)
									tmp = list->song;
								FOR_EACH_MPD_DATA(list)
								{
									howmany++;
									mpd_playlist_queue_add(conn, list->song->file);
								}
								mpd_data_free(list);
								mpd_playlist_queue_commit(conn);
								int new_id;
								try
								{
									new_id = vPlaylist.at(mPlaylist->MaxChoice()-howmany)->GetID();
								}
								catch (std::out_of_range)
								{
									new_id = -1;
								}
								if (new_id >= 0)
									if (vPlaylist[mPlaylist->MaxChoice()-howmany]->GetHash() == tmp.GetHash())
										mpd_player_play_id(conn, new_id);
								break;
							}
						}
						break;
					}
					case csTagEditor:
					{
#						ifdef HAVE_TAGLIB_H
						int id = mTagEditor->GetRealChoice();
						int option = mTagEditor->GetChoice();
						BLOCK_STATUSBAR_UPDATE;
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
									s.GetEmptyFields(1);
									f.tag()->setTitle(NCMPCPP_TO_WSTRING(s.GetTitle()));
									f.tag()->setArtist(NCMPCPP_TO_WSTRING(s.GetArtist()));
									f.tag()->setAlbum(NCMPCPP_TO_WSTRING(s.GetAlbum()));
									f.tag()->setYear(atoi(s.GetYear().c_str()));
									f.tag()->setTrack(atoi(s.GetTrack().c_str()));
									f.tag()->setGenre(NCMPCPP_TO_WSTRING(s.GetGenre()));
									f.tag()->setComment(NCMPCPP_TO_WSTRING(s.GetComment()));
									s.GetEmptyFields(0);
									f.save();
									mpd_database_update_dir(conn, (char *) s.GetDirectory().c_str());
									if (prev_screen == csSearcher)
									{
										int upid = mSearcher->GetChoice()-search_engine_static_option-1;
										vSearched[upid] = s;
										mSearcher->UpdateOption(upid+1, DisplaySong(s));
									}
								}
								else
									ShowMessage("Error writing tags!");
							}
							case 9:
							{
#								endif // HAVE_TAGLIB_H
								wCurrent->Clear();
								wCurrent = wPrev;
								current_screen = prev_screen;
								redraw_me = 1;
								if (current_screen == csLibrary)
								{
#									ifdef HAVE_TAGLIB_H
									if (id == 8)
									{
										mLibSongs->HighlightColor(Config.main_color);
										mLibArtists->HighlightColor(Config.library_active_column_color);
										wCurrent = mLibArtists;
									}
									else
										wCurrent = wPrev;
#									else
									wCurrent = wPrev;
#									endif
									REFRESH_MEDIA_LIBRARY_SCREEN;
								}
#								ifdef HAVE_TAGLIB_H
								break;
							}
						}
						UNBLOCK_STATUSBAR_UPDATE;
#						endif // HAVE_TAGLIB_H
						break;
					}
					case csSearcher:
					{
						int id = mSearcher->GetChoice();
						int option = mSearcher->GetChoice();
						BLOCK_STATUSBAR_UPDATE;
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
									
									for (vector<Song>::const_iterator it = vSearched.begin(); it != vSearched.end(); it++)
									{	
										for (vector<Song *>::const_iterator j = vPlaylist.begin(); j != vPlaylist.end(); j++)
										{
											if ((*j)->GetHash() == it->GetHash())
											{
												bold = 1;
												break;
											}
										}
										bold ? mSearcher->AddBoldOption(DisplaySong(*it)) : mSearcher->AddOption(DisplaySong(*it));
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
								vSearched.clear();
								PrepareSearchEngine(searched_song);
								ShowMessage("Search state reset");
								break;
							}
							default:
							{
								int ci = mSearcher->GetRealChoice()-1;
								const string &file = vSearched[ci-1].GetFile();
								mpd_Song *test = mpd_database_get_fileinfo(conn, (char *) file.c_str());
								if (test)
								{
									mpd_playlist_add(conn, (char *) file.c_str());
									Song s = test;
									if (s.GetHash() == vPlaylist.back()->GetHash())
										mpd_player_play_id(conn, vPlaylist.back()->GetID());
									ShowMessage("Added to playlist: " +  OmitBBCodes(DisplaySong(s)));
								}
								mpd_freeSong(test);
								break;
							}
						}
						UNBLOCK_STATUSBAR_UPDATE;
						break;
					}
					case csLibrary:
					{
						Start_Point_For_KEY_SPACE: // same code for KEY_SPACE, but without playing.
						
						MpdData *data;
						
						if (wCurrent == mLibArtists)
						{
							const string &artist = mLibArtists->GetCurrentOption();
							ShowMessage("Adding all songs artist's: " + artist);
							mpd_database_search_start(conn, 1);
							mpd_database_search_add_constraint(conn, MPD_TAG_ITEM_ARTIST, (char *) artist.c_str());
							data = mpd_database_search_commit(conn);
							int howmany = 0;
							Song tmp;
							if (data)
								tmp = data->song;
							FOR_EACH_MPD_DATA(data)
							{
								howmany++;
								mpd_playlist_queue_add(conn, data->song->file);
							}
							mpd_data_free(data);
							mpd_playlist_queue_commit(conn);
							if (input == ENTER)
							{
								int new_id;
								try
								{
									new_id = vPlaylist.at(mPlaylist->MaxChoice()-howmany)->GetID();
								}
								catch (std::out_of_range)
								{
									new_id = -1;
								}
								if (new_id >= 0)
									if (vPlaylist[mPlaylist->MaxChoice()-howmany]->GetHash() == tmp.GetHash())
										mpd_player_play_id(conn, new_id);
							}
						}
						
						if (wCurrent == mLibAlbums)
						{
							int howmany = 0;
							ShowMessage("Adding songs from album: " + mLibAlbums->GetCurrentOption());
							for (vector<Song>::const_iterator it = vSongs.begin(); it != vSongs.end(); it++, howmany++)
								mpd_playlist_queue_add(conn, (char *) it->GetFile().c_str());
							mpd_playlist_queue_commit(conn);
							if (input == ENTER)
							{
								int new_id;
								try
								{
									new_id = vPlaylist.at(mPlaylist->MaxChoice()-howmany)->GetID();
								}
								catch (std::out_of_range)
								{
									new_id = -1;
								}
								if (new_id >= 0)
									if (vPlaylist[mPlaylist->MaxChoice()-howmany]->GetHash() == vSongs.begin()->GetHash())
										mpd_player_play_id(conn, new_id);
							}
						}
						
						if (wCurrent == mLibSongs)
						{
							if (!vSongs.empty())
							{
								Song &s = vSongs[mLibSongs->GetChoice()-1];
								ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
								mpd_playlist_add(conn, (char *) s.GetFile().c_str());
								if (input == ENTER && s.GetHash() == vPlaylist.back()->GetHash())
									mpd_player_play_id(conn, vPlaylist.back()->GetID());
							}
						}
						
						if (input == KEY_SPACE)
							wCurrent->Go(DOWN);
						
						break;
					}
				}
				break;
			}
			case KEY_SPACE:
			{
				if (current_screen == csBrowser)
				{
					int ci = mBrowser->GetChoice()-1;
					switch (vBrowser[ci].type)
					{
						case MPD_DATA_TYPE_DIRECTORY:
						{
							string getdir = browsed_dir == "/" ? vBrowser[ci].name : browsed_dir + "/" + vBrowser[ci].name;
							MpdData *queue = mpd_database_get_directory_recursive(conn, (char *) getdir.c_str());
							
							FOR_EACH_MPD_DATA(queue)
								mpd_playlist_queue_add(conn, queue->song->file);
							mpd_data_free(queue);
							ShowMessage("Added folder: " + getdir);
							mpd_playlist_queue_commit(conn);
							break;
						}
						case MPD_DATA_TYPE_SONG:
						{
							mpd_Song *test = mpd_database_get_fileinfo(conn, (char *) vBrowser[ci].name.c_str());
							if (test)
							{
								mpd_playlist_queue_add(conn, (char *) vBrowser[ci].name.c_str());
								Song s = test;
								ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
								mpd_playlist_queue_commit(conn);
							}
							mpd_freeSong(test);
							break;
						}
						case MPD_DATA_TYPE_PLAYLIST:
						{
							ShowMessage("Loading playlist " + vBrowser[ci].name + "...");
							MpdData *list = mpd_database_get_playlist_content(conn, (char *) vBrowser[ci].name.c_str());
							FOR_EACH_MPD_DATA(list)
								mpd_playlist_queue_add(conn, list->song->file);
							mpd_data_free(list);
							mpd_playlist_queue_commit(conn);
							break;
						}
					}
					mBrowser->Go(DOWN);
				}
				if (current_screen == csSearcher && !vSearched.empty())
				{
					int id = mSearcher->GetChoice()-search_engine_static_option-1;
					if (id < 0)
						break;
						
					mpd_playlist_queue_add(conn, (char *)vSearched[id].GetFile().c_str());
					Song &s = vSearched[id];
					ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
					mpd_playlist_queue_commit(conn);
					mSearcher->Go(DOWN);
				}
				if (current_screen == csLibrary)
					goto Start_Point_For_KEY_SPACE; // sorry, but that's stupid to copy the same code here.
				break;
			}
			case KEY_RIGHT:
			{
				if (current_screen == csLibrary)
				{
					if (wCurrent == mLibArtists)
					{
						mLibArtists->HighlightColor(Config.main_color);
						wCurrent->Refresh();
						wCurrent = mLibAlbums;
						mLibAlbums->HighlightColor(Config.library_active_column_color);
						if (!mLibAlbums->Empty())
							break;
					}
					if (wCurrent == mLibAlbums)
					{
						mLibAlbums->HighlightColor(Config.main_color);
						wCurrent->Refresh();
						wCurrent = mLibSongs;
						mLibSongs->HighlightColor(Config.library_active_column_color);
						break;
					}
					break;
				}
			}
			case '+': // volume up
			{
				mpd_status_set_volume(conn, mpd_status_get_volume(conn)+1);
				break;
			}
			case KEY_LEFT:
			{
				if (current_screen == csLibrary)
				{
					if (wCurrent == mLibSongs)
					{
						mLibSongs->HighlightColor(Config.main_color);
						wCurrent->Refresh();
						wCurrent = mLibAlbums;
						mLibAlbums->HighlightColor(Config.library_active_column_color);
						if (!mLibAlbums->Empty())
							break;
					}
					if (wCurrent == mLibAlbums)
					{
						mLibAlbums->HighlightColor(Config.main_color);
						wCurrent->Refresh();
						wCurrent = mLibArtists;
						mLibArtists->HighlightColor(Config.library_active_column_color);
						break;
					}
					break;
				}
			}
			case '-': //volume down
			{
				mpd_status_set_volume(conn, mpd_status_get_volume(conn)-1);
				break;
			}
			case KEY_DC: // delete position from list
			{
				if (!mPlaylist->Empty() && current_screen == csPlaylist)
				{
					block_playlist_update = 1;
					dont_change_now_playing = 1;
					mPlaylist->Timeout(50);
					int id = mPlaylist->GetChoice()-1;
					
					while (!vPlaylist.empty() && input == KEY_DC)
					{
						TraceMpdStatus();
						timer = time(NULL);
						
						if (input == KEY_DC)
						{
							id = mPlaylist->GetChoice()-1;
							
							mpd_playlist_queue_delete_pos(conn, id);
							delete vPlaylist[id];
							vPlaylist.erase(vPlaylist.begin()+id);
							mPlaylist->DeleteOption(id+1);
							if (now_playing > id)
								now_playing--;
							mPlaylist->Refresh();
						}
						mPlaylist->ReadKey(input);
					}
					mpd_playlist_queue_commit(conn);
					mPlaylist->Timeout(ncmpcpp_window_timeout);
					dont_change_now_playing = 0;
					block_playlist_update = 0;
				}
				if (current_screen == csBrowser)
				{
					BLOCK_STATUSBAR_UPDATE;
					int id = mBrowser->GetChoice()-1;
					if (vBrowser[id].type == MPD_DATA_TYPE_PLAYLIST)
					{
						block_statusbar_update = 1;
						wFooter->WriteXY(0, Config.statusbar_visibility, "Delete playlist " + vBrowser[id].name + " ? [y/n] ", 1);
						curs_set(1);
						int in = 0;
						do
						{
							TraceMpdStatus();
							wFooter->ReadKey(in);
						}
						while (in != 'y' && in != 'n');
						block_statusbar_update = 0;
						if (in == 'y')
						{
							mpd_database_delete_playlist(conn, (char *) vBrowser[id].name.c_str());
							ShowMessage("Playlist " + vBrowser[id].name + " deleted!");
							GetDirectory("/");
						}
						else
							ShowMessage("Aborted!");
						curs_set(0);
						UNBLOCK_STATUSBAR_UPDATE;
					}
				}
				break;
			}
			case '<':
			{
				mpd_player_prev(conn);
				break;
			}
			case '>':
			{
				mpd_player_next(conn);
				break;
			}
			case 'P': // pause
			{
				mpd_player_pause(conn);
				break;
			}
			case 'S': // save playlist
			{
				string playlist_name;
				BLOCK_STATUSBAR_UPDATE;
				wFooter->WriteXY(0, Config.statusbar_visibility, "Save playlist as: ", 1);
				playlist_name = wFooter->GetString("", TraceMpdStatus);
				UNBLOCK_STATUSBAR_UPDATE;
				if (playlist_name.find("/") != string::npos)
				{
					ShowMessage("Playlist name cannot contain slashes!");
					break;
				}
				if (!playlist_name.empty())
				{
					switch (mpd_database_save_playlist(conn, (char *) playlist_name.c_str()))
					{
						case MPD_OK:
							ShowMessage("Playlist saved as: " + playlist_name);
							break;
						case MPD_DATABASE_PLAYLIST_EXIST:
							ShowMessage("Playlist already exists!");
							break;
					}
				}
				if (browsed_dir == "/" && !vBrowser.empty())
					GetDirectory(browsed_dir);
				break;
			}
			case 's': // stop
			{
				mpd_player_stop(conn);
				break;
			}
			case 'm': // move song up
			{
				block_playlist_update = 1;
				int pos = mPlaylist->GetChoice()-1;
				if (pos > 0 && !mPlaylist->Empty() && current_screen == csPlaylist)
				{
					std::swap<Song *>(vPlaylist[pos], vPlaylist[pos-1]);
					if (pos == now_playing)
					{
						now_playing--;
						mPlaylist->BoldOption(pos, 1);
						mPlaylist->BoldOption(pos+1, 0);
					}
					else
					{
						if (pos-1 == now_playing)
						{
							now_playing++;
							mPlaylist->BoldOption(pos, 0);
							mPlaylist->BoldOption(pos+1, 1);
						}
					}
					mPlaylist->UpdateOption(pos, DisplaySong(*vPlaylist[pos-1]));
					mPlaylist->UpdateOption(pos+1, DisplaySong(*vPlaylist[pos]));
					mpd_playlist_move_pos(conn, pos, pos-1);
					mPlaylist->Go(UP);
				}
				break;
			}
			case 'n': //move song down
			{
				block_playlist_update = 1;
				int pos = mPlaylist->GetChoice()-1;
				if (pos+1 < mpd_playlist_get_playlist_length(conn) && !mPlaylist->Empty() && current_screen == csPlaylist)
				{
					std::swap<Song *>(vPlaylist[pos+1], vPlaylist[pos]);
					if (pos == now_playing)
					{
						now_playing++;
						mPlaylist->BoldOption(pos+1, 0);
						mPlaylist->BoldOption(pos+2, 1);
					}
					else
					{
						if (pos+1 == now_playing)
						{
							now_playing--;
							mPlaylist->BoldOption(pos+1, 1);
							mPlaylist->BoldOption(pos+2, 0);
						}
					}
					mPlaylist->UpdateOption(pos+2, DisplaySong(*vPlaylist[pos+1]));
					mPlaylist->UpdateOption(pos+1, DisplaySong(*vPlaylist[pos]));
					mpd_playlist_move_pos(conn, pos, pos+1);
					mPlaylist->Go(DOWN);
				}
				break;
			}
			case 'f': case 'b': // seek through song
			{
				if (now_playing < 0)
					break;
				
				block_progressbar_update = 1;
				BLOCK_STATUSBAR_UPDATE;
				
				int songpos, in;
				
				songpos = mpd_status_get_elapsed_song_time(conn);
				Song &s = *vPlaylist[now_playing];
				
				while (1)
				{
					TraceMpdStatus();
					timer = time(NULL);
					mPlaylist->ReadKey(in);
					if (in == 'f' || in == 'b')
					{
						if (songpos < s.GetTotalLength() && in == 'f')
							songpos++;
						if (songpos < s.GetTotalLength() && songpos > 0 && in == 'b')
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
				mpd_player_seek(conn, songpos);
				
				block_progressbar_update = 0;
				UNBLOCK_STATUSBAR_UPDATE;
				
				break;
			}
			case 'U': case 'u': // update database
			{
				if (current_screen == csBrowser)
					mpd_database_update_dir(conn, (char *) browsed_dir.c_str());
				else
					mpd_database_update_dir(conn, "/");
				break;
			}
			case 'O': case 'o': // go to playing song
			{
				if (current_screen == csPlaylist && now_playing >= 0)
					mPlaylist->Highlight(now_playing+1);
				break;
			}
			case 'r': // switch repeat state
			{
				mpd_player_set_repeat(conn, !mpd_player_get_repeat(conn));
				break;
			}
			case 'Z':
			{
				mpd_playlist_shuffle(conn);
				break;
			}
			case 'z': //switch random state
			{
				mpd_player_set_random(conn, !mpd_player_get_random(conn));
				break;
			}
			case 'x': // switch crossfade state
			{
				mpd_status_set_crossfade(conn, (mpd_status_get_crossfade(conn) ? 0 : Config.crossfade_time));
				break;
			}
			case 'E': case 'e': // edit song's tags
			{
				int id = wCurrent->GetChoice()-1;
				switch (current_screen)
				{
					case csPlaylist:
					{
						if (!mPlaylist->Empty())
						{
							if (GetSongInfo(*vPlaylist[id]))
							{
								wCurrent = mTagEditor;
								wPrev = mPlaylist;
								current_screen = csTagEditor;
								prev_screen = csPlaylist;
							}
							else
								ShowMessage("Cannot read file!");
						}
						break;
					}
					case csBrowser:
					{
						if (vBrowser[id].type == MPD_DATA_TYPE_SONG)
						{
							Song edited = mpd_database_get_fileinfo(conn, vBrowser[id].name.c_str());
							if (GetSongInfo(edited))
							{
								wCurrent = mTagEditor;
								wPrev = mBrowser;
								current_screen = csTagEditor;
								prev_screen = csBrowser;
							}
							else
								ShowMessage("Cannot read file!");
						}
						break;
					}
					case csSearcher:
					{
						if (id >= search_engine_static_option && !vSearched.empty())
						{
							if (GetSongInfo(vSearched[id-search_engine_static_option]))
							{
								wCurrent = mTagEditor;
								wPrev = mSearcher;
								current_screen = csTagEditor;
								prev_screen = csSearcher;
							}
							else
								ShowMessage("Cannot read file!");
						}
						break;
					}
					case csLibrary:
					{
						if (!vSongs.empty() && wCurrent == mLibSongs)
						{
							if (GetSongInfo(vSongs[id]))
							{
								wPrev = wCurrent;
								wCurrent = mTagEditor;
								current_screen = csTagEditor;
								prev_screen = csLibrary;
							}
							else
								ShowMessage("Cannot read file!");
						}
					}
					default:
						break;
				}
				break;
			}
			case 'g': // go to position in currently playing song
			{
				if (now_playing < 0)
					break;
				int newpos = 0;
				string position;
				BLOCK_STATUSBAR_UPDATE;
				wFooter->WriteXY(0, Config.statusbar_visibility, "Position to go (in %): ", 1);
				position = wFooter->GetString(3, TraceMpdStatus);
				newpos = atoi(position.c_str());
				if (newpos > 0 && newpos < 100 && !position.empty())
					mpd_player_seek(conn, vPlaylist[now_playing]->GetTotalLength()*newpos/100.0);
				UNBLOCK_STATUSBAR_UPDATE;
				break;
			}
			case 'C': // clear playlist but holds currently playing song
			{
				if (now_playing < 0)
				{
					ShowMessage("Nothing is playing now!");
					break;
				}
				for (vector<Song *>::iterator it = vPlaylist.begin(); it != vPlaylist.begin()+now_playing; it++)
					mpd_playlist_queue_delete_id(conn, (*it)->GetID());
				for (vector<Song *>::iterator it = vPlaylist.begin()+now_playing+1; it != vPlaylist.end(); it++)
					mpd_playlist_queue_delete_id(conn, (*it)->GetID());
				ShowMessage("Deleting all songs except now playing one...");
				mpd_playlist_queue_commit(conn);
				ShowMessage("Songs deleted!");
				break;
			}
			case 'c': // clear playlist
			{
				ShowMessage("Clearing playlist...");
				mpd_playlist_clear(conn);
				ShowMessage("Cleared playlist!");
				break;
			}
			case '/': case '?': // find forward/backward
			{
				if (current_screen != csHelp && current_screen != csSearcher || (current_screen == csSearcher && !vSearched.empty()))
				{
					string how = input == '/' ? "forward" : "backward";
					found_pos = 0;
					vFoundPositions.clear();
					Menu *mCurrent = static_cast<Menu *>(wCurrent);
					BLOCK_STATUSBAR_UPDATE;
					wFooter->WriteXY(0, Config.statusbar_visibility, "Find " + how + ": ", 1);
					string findme = wFooter->GetString("", TraceMpdStatus);
					UNBLOCK_STATUSBAR_UPDATE;
					timer = time(NULL);
					if (findme.empty())
						break;
					transform(findme.begin(), findme.end(), findme.begin(), tolower);
					
					if (input == '/') // forward
					{
						for (int i = mCurrent->GetChoice(); i <= mCurrent->MaxChoice(); i++)
						{
							string name = mCurrent->GetOption(i);
							transform(name.begin(), name.end(), name.begin(), tolower);
							if (name.find(findme) != string::npos && !mCurrent->IsStatic(i))
								vFoundPositions.push_back(i);
						}
					}
					else // backward
					{
						for (int i = mCurrent->GetChoice(); i > 0; i--)
						{
							string name = mCurrent->GetOption(i);
							transform(name.begin(), name.end(), name.begin(), tolower);
							if (name.find(findme) != string::npos && !mCurrent->IsStatic(i))
								vFoundPositions.push_back(i);
						}
					}
					
					if (vFoundPositions.empty())
						ShowMessage("Unable to find \"" + findme + "\"");
					else
					{
						mCurrent->Highlight(vFoundPositions.front());
						mCurrent->Highlighting(1);
					}
				}
				break;
			}
			case '.': // go to next/previous found position
			{
				if (!vFoundPositions.empty())
				{
					Menu *mCurrent = static_cast<Menu *>(wCurrent);
					try
					{
						mCurrent->Highlight(vFoundPositions.at(++found_pos));
					}
					catch (std::out_of_range)
					{
						mCurrent->Highlight(vFoundPositions.front());
						found_pos = 0;
					}
				}
				break;
			}
			case 'l': // show lyrics
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
					break;
				}
				if ((wCurrent == mPlaylist && !vPlaylist.empty())
				||  (wCurrent == mBrowser && vBrowser[mBrowser->GetChoice()-1].type == MPD_DATA_TYPE_SONG)
				||  (wCurrent == mSearcher && !vSearched.empty() && mSearcher->GetChoice() > search_engine_static_option)
				||  (wCurrent == mLibSongs))
				{
					Song s;
					switch (current_screen)
					{
						case csPlaylist:
							s = *vPlaylist[mPlaylist->GetChoice()-1];
							break;
						case csBrowser:
							s = mpd_database_get_fileinfo(conn, vBrowser[mBrowser->GetChoice()-1].name.c_str());
							break;
						case csSearcher:
							s = vSearched[mSearcher->GetChoice()-search_engine_static_option-1];
							break;
						case csLibrary:
							s = vSongs[mLibSongs->GetChoice()-1];
							break;
					}
					
					if (s.GetArtist() != UNKNOWN_ARTIST && s.GetTitle() != UNKNOWN_TITLE)
					{
						wPrev = wCurrent;
						prev_screen = current_screen;
						wCurrent = sLyrics;
						wCurrent->Hide();
						wCurrent->Clear();
						current_screen = csLyrics;
						
						sLyrics->WriteXY(0, 0, "Fetching lyrics...");
						sLyrics->Refresh();
						sLyrics->Add("[b]" + s.GetArtist() + " - " + s.GetTitle() + "[/b]\n\n");
						sLyrics->Add(GetLyrics(s.GetArtist(), s.GetTitle()));
						sLyrics->Timeout(ncmpcpp_window_timeout);
					}
				}
				break;
			}
			case '1': // help screen
			{
				if (wCurrent != sHelp)
				{
					wCurrent = sHelp;
					wCurrent->Hide();
					current_screen = csHelp;
				}
				break;
			}
			case '2': // playlist screen
			{
				if (wCurrent != mPlaylist && current_screen != csTagEditor)
				{
					found_pos = 0;
					vFoundPositions.clear();
					wCurrent = mPlaylist;
					wCurrent->Hide();
					current_screen = csPlaylist;
					redraw_me = 1;
				}
				break;
			}
			case '3': // browse screen
			{
				if (browsed_dir.empty())
					browsed_dir = "/";
				
				if (mBrowser->Empty())
					GetDirectory(browsed_dir);
				else
				{
					bool bold = 0;
					for (int i = 0; i < vBrowser.size(); i++)
					{
						if (vBrowser[i].type == MPD_DATA_TYPE_SONG)
						{
							for (vector<Song *>::const_iterator it = vPlaylist.begin(); it != vPlaylist.end(); it++)
							{
								if ((*it)->GetHash() == vBrowser[i].hash)
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
				break;
			}
			case '4': // search screen
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
						for (vector<Song>::const_iterator it = vSearched.begin(); it != vSearched.end(); it++, i++)
						{
							for (vector<Song *>::const_iterator j = vPlaylist.begin(); j != vPlaylist.end(); j++)
							{
								if ((*j)->GetHash() == it->GetHash())
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
				break;
			}
			case '5': // artist library
			{
				if (current_screen != csLibrary)
				{
					found_pos = 0;
					vFoundPositions.clear();
					
					if (mLibArtists->Empty())
					{
						MpdData *data = mpd_database_get_artists(conn);
				
						FOR_EACH_MPD_DATA(data)
								vArtists.push_back(data->tag);
						mpd_data_free(data);
				
						sort(vArtists.begin(), vArtists.end(), CaseInsensitiveComparison);
				
						for (vector<string>::const_iterator it = vArtists.begin(); it != vArtists.end(); it++)
							mLibArtists->AddOption(*it);
					}
					
					mLibArtists->HighlightColor(Config.library_active_column_color);
					mLibAlbums->HighlightColor(Config.main_color);
					mLibSongs->HighlightColor(Config.main_color);
					
					wCurrent->Hide();
					
					REFRESH_MEDIA_LIBRARY_SCREEN;
				
					wCurrent = mLibArtists;
					current_screen = csLibrary;
					redraw_me = 1;
				}
				break;
			}
			case 'q': case 'Q':
				main_exit = 1;
			default: continue;
		}
	}
	
	mpd_disconnect(conn);
	mpd_free(conn);
	curs_set(1);
	endwin();
	printf("\n");
	return 0;
}

