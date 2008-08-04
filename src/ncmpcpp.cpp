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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ncmpcpp.h"
#include "status_checker.h"
#include "helpers.h"
#include "settings.h"
#include "song.h"

#define FOR_EACH_MPD_DATA(x) for (; (x); (x) = mpd_data_get_next(x))

char *MPD_HOST = getenv("MPD_HOST");
int MPD_PORT = getenv("MPD_PORT") ? atoi(getenv("MPD_PORT")) : 6600;
char *MPD_PASSWORD = getenv("MPD_PASSWORD");

ncmpcpp_config Config;

vector<Song> vPlaylist;
vector<Song> vSearched;
vector<MpdDataType> vFileType;
vector<string> vNameList;

Window *mCurrent = 0;
Window *mPrev = 0;
Menu *mPlaylist;
Menu *mBrowser;
Menu *mTagEditor;
Menu *mSearcher;
Scrollpad *sHelp;

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

int block_statusbar_update_delay = -1;

long long current_playlist_id = -1;

string browsed_dir = "/";
string browsed_subdir;
string player_state;
string volume_state;
string switch_state;

string mpd_repeat;
string mpd_random;
string mpd_crossfade;
string mpd_db_updating;

CurrScreen current_screen;
CurrScreen prev_screen;

Song edited_song;
Song searched_song;

bool main_exit = 0;
bool messages_allowed = 0;
bool title_allowed = 0;

bool header_update_status = 0;

bool block_progressbar_update = 0;
bool block_statusbar_update = 0;
bool allow_statusbar_unblock = 1;
bool block_playlist_update = 0;

bool search_case_sensitive = 1;
bool search_mode_match = 1;

extern string EMPTY;
extern string UNKNOWN_ARTIST;
extern string UNKNOWN_TITLE;
extern string UNKNOWN_ALBUM;

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
	
	mPlaylist = new Menu(0, 2, COLS, LINES-4, "", Config.main_color, brNone);
	mBrowser = new Menu(mPlaylist->EmptyClone());
	mTagEditor = new Menu(mPlaylist->EmptyClone());
	mSearcher = new Menu(mPlaylist->EmptyClone());
	
	sHelp = new Scrollpad(0, 2, COLS, LINES-4, "", Config.main_color, brNone);
	
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
	sHelp->Add("\t4         : Search engine\n\n\n");
	
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
	
	sHelp->Add("\tg         : Go to chosen position in current song\n\n");
	
	sHelp->Add("\tQ q       :  Quit\n\n\n");
	
	sHelp->Add("   [b]Keys - Playlist screen\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Play\n");
	sHelp->Add("\tDelete    : Delete song from playlist\n");
	sHelp->Add("\tc         : Clear playlist\n");
	sHelp->Add("\tm         : Move song up\n");
	sHelp->Add("\tn         : Move song down\n");
	sHelp->Add("\tS         : Save playlist\n");
	sHelp->Add("\tE e       : Edit song's tags\n");
	sHelp->Add("\to         : Go to currently playing position\n\n\n");
	
	sHelp->Add("   [b]Keys - Browse screen\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Enter directory/Select and play song\n");
	sHelp->Add("\tSpace     : Add song to playlist\n");
	sHelp->Add("\tDelete    : Delete playlist\n");
	sHelp->Add("\tE e       : Edit song's tags\n\n\n");
	
	sHelp->Add("   [b]Keys - Search engine\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Change option/Select and play song\n");
	sHelp->Add("\tSpace     : Add song to playlist\n");
	sHelp->Add("\tE e       : Edit song's tags\n\n\n");
	
	sHelp->Add("   [b]Keys - Tag Editor\n -----------------------------------------[/b]\n");
	sHelp->Add("\tEnter     : Change option\n");
	
	wHeader = new Window(0, 0, COLS, 2, "", Config.header_color, brNone);
	wFooter = new Window(0, LINES-2, COLS, 2, "", Config.statusbar_color, brNone);
	
	wHeader->Display();
	wFooter->Display();
	
	mpd_signal_connect_error(conn, (ErrorCallback)NcmpcppErrorCallback, NULL);
	mpd_signal_connect_status_changed(conn, (StatusChangedCallback)NcmpcppStatusChanged, NULL);
	
	mvwhline(wHeader->RawWin(), 1, 0, 0, wHeader->GetWidth());
	wHeader->Refresh();
	wFooter->SetColor(Config.progressbar_color);
	mvwhline(wFooter->RawWin(), 0, 0, 0, wFooter->GetWidth());
	wFooter->SetColor(Config.statusbar_color);
	wFooter->Refresh();
	
	mCurrent = mPlaylist;
	current_screen = csPlaylist;
	
	mCurrent->Display();
	
	int input;
	timer = time(NULL);
	
	sHelp->Timeout(ncmpcpp_window_timeout);
	mPlaylist->Timeout(ncmpcpp_window_timeout);
	mBrowser->Timeout(ncmpcpp_window_timeout);
	mTagEditor->Timeout(ncmpcpp_window_timeout);
	mSearcher->Timeout(ncmpcpp_window_timeout);
	wFooter->Timeout(ncmpcpp_window_timeout);
	
	while (!main_exit)
	{
		TraceMpdStatus();
		
		string title;
		int max_allowed_title_length = wHeader->GetWidth()-volume_state.length();
		
		messages_allowed = 1;
		
		switch (current_screen)
		{
			case csHelp:
				title = "Help";
				break;
			case csPlaylist:
				title = "Playlist";
				break;
			case csBrowser:
				title = "Browse: ";
				break;
			case csTagEditor:
				title = "Tag editor";
				break;
			case csSearcher:
				title = "Search engine";
		}
		
		if (title_allowed)
		{
			wHeader->Bold(1);
			wHeader->WriteXY(0, 0, title, 1);
			wHeader->Bold(0);
		}
		else
			wHeader->WriteXY(0, 0, "[b]1:[/b]Help  [b]2:[/b]Playlist  [b]3:[/b]Browse  [b]4:[/b]Search", 1);
		
		wHeader->WriteXY(max_allowed_title_length, 0, volume_state);
		
		if (current_screen == csBrowser)
		{
			int max_length_without_scroll = wHeader->GetWidth()-volume_state.length()-title.length();
			ncmpcpp_string_t wbrowseddir = NCMPCPP_TO_WSTRING(browsed_dir);
			wHeader->Bold(1);
			if (browsed_dir.length() > max_length_without_scroll)
			{
#				ifdef UTF8_ENABLED
				wbrowseddir += L" ** ";
#				else
				wbrowseddir += " ** ";
#				endif
				const int scrollsize = max_length_without_scroll;
				ncmpcpp_string_t part = wbrowseddir.substr(browsed_dir_scroll_begin++, scrollsize);
				if (part.length() < scrollsize)
					part += wbrowseddir.substr(0, scrollsize-part.length());
				wHeader->WriteXY(8, 0, part);
				if (browsed_dir_scroll_begin >= wbrowseddir.length())
					browsed_dir_scroll_begin = 0;
			}
			else
				wHeader->WriteXY(8, 0, browsed_dir);
			wHeader->Bold(0);
		}
		
		mCurrent->Refresh();
		
		mCurrent->ReadKey(input);
		if (input == ERR)
			continue;
		
		title_allowed = 1;
		timer = time(NULL);
		
		if (current_screen == csPlaylist)
		{
			mPlaylist->Highlighting(1);
			mPlaylist->Refresh();
		}
		if (current_screen == csBrowser)
			browsed_dir_scroll_begin--;
		
		switch (input)
		{
			case KEY_UP: mCurrent->Go(UP); continue;
			case KEY_DOWN: mCurrent->Go(DOWN); continue;
			case KEY_PPAGE: mCurrent->Go(PAGE_UP); continue;
			case KEY_NPAGE: mCurrent->Go(PAGE_DOWN); continue;
			case KEY_HOME: mCurrent->Go(HOME); continue;
			case KEY_END: mCurrent->Go(END); continue;
			case KEY_RESIZE:
			{
				int in;
				
				while (1)
				{
					mCurrent->ReadKey(in);
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
				
				sHelp->Resize(COLS, LINES-4);
				sHelp->Timeout(ncmpcpp_window_timeout);
				mPlaylist->Resize(COLS, LINES-4);
				mBrowser->Resize(COLS, LINES-4);
				mTagEditor->Resize(COLS, LINES-4);
				mSearcher->Resize(COLS, LINES-4);
				
				wHeader->Resize(COLS, wHeader->GetHeight());
				wFooter->MoveTo(0, LINES-2);
				wFooter->Resize(COLS, wFooter->GetHeight());
				
				if (mCurrent != sHelp)
					mCurrent->Hide();
				mCurrent->Display();
				
				wHeader->DisableBB();
				wHeader->Bold(1);
				mvwhline(wHeader->RawWin(), 1, 0, 0, wHeader->GetWidth());
				if (!switch_state.empty())
					wHeader->WriteXY(wHeader->GetWidth()-switch_state.length()-3, 1, "[" + switch_state + "]");
				wHeader->Refresh();
				wHeader->Bold(0);
				wHeader->EnableBB();
				
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
							mpd_player_play_id(conn, vPlaylist[mPlaylist->GetChoice()-1].GetID());
						break;
					}
					case csBrowser:
					{
						int ci = mBrowser->GetChoice()-1;
						switch (vFileType[ci])
						{
							case MPD_DATA_TYPE_DIRECTORY:
							{
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
										GetDirectory(browsed_dir + "/" + vNameList[ci]);
									else
										GetDirectory(vNameList[ci]);
								break;
							}
							case MPD_DATA_TYPE_SONG:
							{
								char *file = (char *)vNameList[ci].c_str();
								mpd_Song *test = mpd_database_get_fileinfo(conn, file);
								if (test)
								{
									mpd_playlist_add(conn, file);
									Song &s = vPlaylist.back();
									mpd_player_play_id(conn, s.GetID());
									ShowMessage("Added to playlist: " +  OmitBBCodes(DisplaySong(s)));
									mBrowser->Refresh();
								}
								break;
							}
							case MPD_DATA_TYPE_PLAYLIST:
							{
								int howmany = 0;
								ShowMessage("Loading and playing playlist " + vNameList[ci] + "...");
								MpdData *list = mpd_database_get_playlist_content(conn, (char *) vNameList[ci].c_str());
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
									new_id = vPlaylist.at(mPlaylist->MaxChoice()-howmany).GetID();
								}
								catch (std::out_of_range)
								{
									new_id = -1;
								}
								if (new_id >= 0)
									mpd_player_play_id(conn, new_id);
								break;
							}
						}
						break;
					}
					case csTagEditor:
					{
						int id = mTagEditor->GetRealChoice();
						int option = mTagEditor->GetChoice();
						block_statusbar_update = 1;
						allow_statusbar_unblock = 0;
						Song &s = edited_song;
					
						switch (id)
						{
							case 1:
							{
								wFooter->WriteXY(0, 1, "[b]New title:[/b] ", 1);
								if (s.GetTitle() == UNKNOWN_TITLE)
									s.SetTitle(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetTitle(wFooter->GetString(s.GetTitle(), TraceMpdStatus));
								mTagEditor->UpdateOption(option, "[b]Title:[/b] " + s.GetTitle());
								break;
							}
							case 2:
							{
								wFooter->WriteXY(0, 1, "[b]New artist:[/b] ", 1);
								if (s.GetArtist() == UNKNOWN_ARTIST)
									s.SetArtist(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetArtist(wFooter->GetString(s.GetArtist(), TraceMpdStatus));
								mTagEditor->UpdateOption(option, "[b]Artist:[/b] " + s.GetArtist());
								break;
							}
							case 3:
							{
								wFooter->WriteXY(0, 1, "[b]New album:[/b] ", 1);
								if (s.GetAlbum() == UNKNOWN_ALBUM)
									s.SetAlbum(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetAlbum(wFooter->GetString(s.GetAlbum(), TraceMpdStatus));
								mTagEditor->UpdateOption(option, "[b]Album:[/b] " + s.GetAlbum());
								break;
							}
							case 4:
							{
								wFooter->WriteXY(0, 1, "[b]New year:[/b] ", 1);
								if (s.GetYear() == EMPTY)
									s.SetDate(wFooter->GetString(4, TraceMpdStatus));
								else
									s.SetDate(wFooter->GetString(s.GetYear(), 4, TraceMpdStatus));
								mTagEditor->UpdateOption(option, "[b]Year:[/b] " + s.GetYear());
								break;
							}
							case 5:
							{
								wFooter->WriteXY(0, 1, "[b]New track:[/b] ", 1);
								if (s.GetTrack() == EMPTY)
									s.SetTrack(wFooter->GetString(3, TraceMpdStatus));
								else
									s.SetTrack(wFooter->GetString(s.GetTrack(), 3, TraceMpdStatus));
								mTagEditor->UpdateOption(option, "[b]Track:[/b] " + s.GetTrack());
								break;
							}
							case 6:
							{
								wFooter->WriteXY(0, 1, "[b]New genre:[/b] ", 1);
								if (s.GetGenre() == EMPTY)
									s.SetGenre(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetGenre(wFooter->GetString(s.GetGenre(), TraceMpdStatus));
								mTagEditor->UpdateOption(option, "[b]Genre:[/b] " + s.GetGenre());
								break;
							}
							case 7:
							{
								wFooter->WriteXY(0, 1, "[b]New comment:[/b] ", 1);
								if (s.GetComment() == EMPTY)
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
								mCurrent->Clear();
								mCurrent = mPrev;
								current_screen = prev_screen;
								break;
							}
						}
						allow_statusbar_unblock = 1;
						if (block_statusbar_update_delay < 0)
							block_statusbar_update = 0;
						break;
					}
					case csSearcher:
					{
						int id = mSearcher->GetChoice();
						int option = mSearcher->GetChoice();
						block_statusbar_update = 1;
						allow_statusbar_unblock = 0;
						Song &s = searched_song;
					
						switch (id)
						{
							case 1:
							{
								wFooter->WriteXY(0, 1, "[b]Filename:[/b] ", 1);
								if (s.GetShortFilename() == EMPTY)
									s.SetShortFilename(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetShortFilename(wFooter->GetString(s.GetShortFilename(), TraceMpdStatus));
								mSearcher->UpdateOption(option, "[b]Filename:[/b] " + s.GetShortFilename());
								break;
							}
							case 2:
							{
								wFooter->WriteXY(0, 1, "[b]Title:[/b] ", 1);
								if (s.GetTitle() == UNKNOWN_TITLE)
									s.SetTitle(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetTitle(wFooter->GetString(s.GetTitle(), TraceMpdStatus));
								mSearcher->UpdateOption(option, "[b]Title:[/b] " + s.GetTitle());
								break;
							}
							case 3:
							{
								wFooter->WriteXY(0, 1, "[b]Artist:[/b] ", 1);
								if (s.GetArtist() == UNKNOWN_ARTIST)
									s.SetArtist(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetArtist(wFooter->GetString(s.GetArtist(), TraceMpdStatus));
								mSearcher->UpdateOption(option, "[b]Artist:[/b] " + s.GetArtist());
								break;
							}
							case 4:
							{
								wFooter->WriteXY(0, 1, "[b]Album:[/b] ", 1);
								if (s.GetAlbum() == UNKNOWN_ALBUM)
									s.SetAlbum(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetAlbum(wFooter->GetString(s.GetAlbum(), TraceMpdStatus));
								mSearcher->UpdateOption(option, "[b]Album:[/b] " + s.GetAlbum());
								break;
							}
							case 5:
							{
								wFooter->WriteXY(0, 1, "[b]Year:[/b] ", 1);
								if (s.GetYear() == EMPTY)
									s.SetDate(wFooter->GetString(4, TraceMpdStatus));
								else
									s.SetDate(wFooter->GetString(s.GetYear(), 4, TraceMpdStatus));
								mSearcher->UpdateOption(option, "[b]Year:[/b] " + s.GetYear());
								break;
							}
							case 6:
							{
								wFooter->WriteXY(0, 1, "[b]Track:[/b] ", 1);
								if (s.GetTrack() == EMPTY)
									s.SetTrack(wFooter->GetString(3, TraceMpdStatus));
								else
									s.SetTrack(wFooter->GetString(s.GetTrack(), 3, TraceMpdStatus));
								mSearcher->UpdateOption(option, "[b]Track:[/b] " + s.GetTrack());
								break;
							}
							case 7:
							{
								wFooter->WriteXY(0, 1, "[b]Genre:[/b] ", 1);
								if (s.GetGenre() == EMPTY)
									s.SetGenre(wFooter->GetString("", TraceMpdStatus));
								else
									s.SetGenre(wFooter->GetString(s.GetGenre(), TraceMpdStatus));
								mSearcher->UpdateOption(option, "[b]Genre:[/b] " + s.GetGenre());
								break;
							}
							case 8:
							{
								wFooter->WriteXY(0, 1, "[b]Comment:[/b] ", 1);
								if (s.GetComment() == EMPTY)
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
										for (vector<Song>::const_iterator j = vPlaylist.begin(); j != vPlaylist.end(); j++)
											if (j->GetFile() == it->GetFile())
												bold = 1;
										if (bold)
											mSearcher->AddBoldOption(DisplaySong(*it));
										else
											mSearcher->AddOption(DisplaySong(*it));
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
								vSearched.clear();
								PrepareSearchEngine(searched_song);
								ShowMessage("Search state reset");
								break;
							}
							default:
							{
								int ci = mSearcher->GetRealChoice()-1;
								char *file = (char *)vSearched[ci-1].GetFile().c_str();
								mpd_Song *test = mpd_database_get_fileinfo(conn, file);
								if (test)
								{
									mpd_playlist_add(conn, file);
									Song &s = vPlaylist.back();
									mpd_player_play_id(conn, s.GetID());
									ShowMessage("Added to playlist: " +  OmitBBCodes(DisplaySong(s)));
									mSearcher->Refresh();
								}
								break;
							}
						}
						allow_statusbar_unblock = 1;
						if (block_statusbar_update_delay < 0)
							block_statusbar_update = 0;
						
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
					switch (vFileType[ci])
					{
						case MPD_DATA_TYPE_DIRECTORY:
						{
							string getdir = browsed_dir == "/" ? vNameList[ci] : browsed_dir + "/" + vNameList[ci];
							MpdData *queue = mpd_database_get_directory_recursive(conn, (char *) getdir.c_str());
							
							FOR_EACH_MPD_DATA(queue)
								mpd_playlist_queue_add(conn, queue->song->file);
							mpd_data_free(queue);
							mpd_playlist_queue_commit(conn);
							ShowMessage("Added folder: " + getdir);
							break;
						}
						case MPD_DATA_TYPE_SONG:
						{
							if (mpd_playlist_add(conn, (char *) vNameList[ci].c_str()) == MPD_OK);
							{
								Song &s = vPlaylist.back();
								ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
							}
							break;
						}
						case MPD_DATA_TYPE_PLAYLIST:
						{
							ShowMessage("Loading playlist " + vNameList[ci] + "...");
							MpdData *list = mpd_database_get_playlist_content(conn, (char *) vNameList[ci].c_str());
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
					try
					{
						
						if (mpd_playlist_add(conn, (char *)vSearched.at(id).GetFile().c_str()) == MPD_OK);
						{
							Song &s = vPlaylist.back();
							ShowMessage("Added to playlist: " + OmitBBCodes(DisplaySong(s)));
							mSearcher->Go(DOWN);
						}
					}
					catch (std::out_of_range)
					{
						ShowMessage("Error adding file!");
					}
				}
				break;
			}
			case KEY_RIGHT: case '+': // volume up
			{
				mpd_status_set_volume(conn, mpd_status_get_volume(conn)+1);
				break;
			}
			case KEY_LEFT: case '-': //volume down
			{
				mpd_status_set_volume(conn, mpd_status_get_volume(conn)-1);
				break;
			}
			case KEY_DC: // delete position from list
			{
				if (!mPlaylist->Empty() && current_screen == csPlaylist)
				{
					block_playlist_update = 1;
					mPlaylist->Timeout(50);
					int id = mPlaylist->GetChoice()-1;
					
					while (!vPlaylist.empty() && input != ERR)
					{
						TraceMpdStatus();
						timer = time(NULL);
						
						if (input == KEY_DC)
						{
							id = mPlaylist->GetChoice()-1;
							
							mpd_playlist_queue_delete_pos(conn, id);
							for (vector<Song>::iterator it = vPlaylist.begin()+id; it != vPlaylist.end(); it++)
								it->SetPosition(it->GetPosition()-1);
							if (now_playing > id)
								now_playing--;
							vPlaylist.erase(vPlaylist.begin()+id);
							mPlaylist->DeleteOption(id+1);
							mPlaylist->Refresh();
						}
						mPlaylist->ReadKey(input);
					}
					
					mpd_playlist_queue_commit(conn);
					mPlaylist->Timeout(ncmpcpp_window_timeout);
					block_playlist_update = 0;
				}
				if (current_screen == csBrowser)
				{
					block_statusbar_update = 1;
					allow_statusbar_unblock = 0;
					
					int id = mBrowser->GetChoice()-1;
					if (vFileType[id] == MPD_DATA_TYPE_PLAYLIST)
					{
						block_statusbar_update = 1;
						wFooter->WriteXY(0, 1, "Delete playlist " + vNameList[id] + " ? [y/n] ", 1);
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
							mpd_database_delete_playlist(conn, (char *) vNameList[id].c_str());
							ShowMessage("Playlist " + vNameList[id] + " deleted!");
							GetDirectory("/");
						}
						else
							ShowMessage("Aborted!");
						curs_set(0);
						allow_statusbar_unblock = 1;
						if (block_statusbar_update_delay < 0)
							block_statusbar_update = 0;
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
				block_statusbar_update = 1;
				allow_statusbar_unblock = 0;
				wFooter->WriteXY(0, 1, "Save playlist as: ", 1);
				playlist_name = wFooter->GetString("", TraceMpdStatus);
				allow_statusbar_unblock = 1;
				if (block_statusbar_update_delay < 0)
					block_statusbar_update = 0;
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
							ShowMessage("Playlist already exist!");
							break;
					}
				}
				if (browsed_dir == "/" && !vNameList.empty())
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
				int pos = mPlaylist->GetChoice()-1;
				if (pos > 0 && !mPlaylist->Empty() && current_screen == csPlaylist)
				{
					mpd_playlist_move_pos(conn, pos, pos-1);
					mPlaylist->Go(UP);
				}
				break;
			}
			case 'n': //move song down
			{
				int pos = mPlaylist->GetChoice()-1;
				if (pos+1 < mpd_playlist_get_playlist_length(conn) && !mPlaylist->Empty() && current_screen == csPlaylist)
				{
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
				block_statusbar_update = 1;
				
				int songpos, in;
				
				songpos = mpd_status_get_elapsed_song_time(conn);
				Song &s = vPlaylist[now_playing];
				
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
				block_statusbar_update = 0;
				
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
				int id = mCurrent->GetChoice()-1;
				switch (current_screen)
				{
					case csPlaylist:
					{
						if (GetSongInfo(vPlaylist[id]))
						{
							mCurrent = mTagEditor;
							mPrev = mPlaylist;
							current_screen = csTagEditor;
							prev_screen = csPlaylist;
						}
						else
							ShowMessage("Cannot read file!");
						break;
					}
					case csBrowser:
					{
						if (vFileType[id] == MPD_DATA_TYPE_SONG)
						{
							Song edited = mpd_database_get_fileinfo(conn, vNameList[id].c_str());
							if (GetSongInfo(edited))
							{
								mCurrent = mTagEditor;
								mPrev = mBrowser;
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
								mCurrent = mTagEditor;
								mPrev = mSearcher;
								current_screen = csTagEditor;
								prev_screen = csSearcher;
							}
							else
								ShowMessage("Cannot read file!");
						}
						break;
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
				block_statusbar_update = 1;
				allow_statusbar_unblock = 0;
				wFooter->WriteXY(0, 1, "Position to go (in %): ", 1);
				position = wFooter->GetString(3, TraceMpdStatus);
				newpos = atoi(position.c_str());
				if (newpos > 0 && newpos < 100 && !position.empty())
					mpd_player_seek(conn, vPlaylist[now_playing].GetTotalLength()*newpos/100.0);
				allow_statusbar_unblock = 1;
				if (block_statusbar_update_delay < 0)
					block_statusbar_update = 0;
				break;
			}
			case 'c': // clear playlist
			{
				mpd_playlist_clear(conn);
				ShowMessage("Cleared playlist!");
				break;
			}
			case '1': // help screen
			{
				if (mCurrent != sHelp)
				{
					mCurrent->Hide();
					mCurrent = sHelp;
					current_screen = csHelp;
				}
				break;
			}
			case '2': // playlist screen
			{
				if (mCurrent != mPlaylist && current_screen != csTagEditor)
				{
					mCurrent->Hide();
					mCurrent = mPlaylist;
					current_screen = csPlaylist;
				}
				break;
			}
			case '3': // browse screen
			{
				if (browsed_dir.empty())
					browsed_dir = "/";
				
				if (mCurrent != mBrowser && current_screen != csTagEditor)
				{
					mCurrent->Hide();
					mCurrent = mBrowser;
					current_screen = csBrowser;
				}
				if (mBrowser->Empty())
					GetDirectory(browsed_dir);
				break;
			}
			case '4': // search screen
			{
				if (current_screen != csTagEditor && current_screen != csSearcher)
				{
					mCurrent->Hide();
					if (vSearched.empty())
						PrepareSearchEngine(searched_song);
					mCurrent = mSearcher;
					current_screen = csSearcher;
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

