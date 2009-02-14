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

#include "tag_editor.h"

#ifdef HAVE_TAGLIB_H

#include <fstream>
#include <stdexcept>

#include "id3v2tag.h"
#include "textidentificationframe.h"
#include "mpegfile.h"

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "media_library.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "status_checker.h"

using namespace Global;
using namespace MPD;

Menu<Buffer> *Global::mTagEditor;

Window *Global::wTagEditorActiveCol;
Menu<string_pair> *Global::mEditorAlbums;
Menu<string_pair> *Global::mEditorDirs;
Menu<string_pair> *Global::mEditorLeftCol;
Menu<string> *Global::mEditorTagTypes;
Menu<Song> *Global::mEditorTags;

void TinyTagEditor::Init()
{
	mTagEditor = new Menu<Buffer>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mTagEditor->HighlightColor(Config.main_highlight_color);
	mTagEditor->SetTimeout(ncmpcpp_window_timeout);
	mTagEditor->SetItemDisplayer(Display::Generic);
}

void TinyTagEditor::EnterPressed(Song &s)
{
	size_t option = mTagEditor->Choice();
	LockStatusbar();
	
	if (option >= 8 && option <= 20)
		mTagEditor->at(option).Clear();
	
	switch (option-7)
	{
		case 1:
		{
			Statusbar() << fmtBold << "Title: " << fmtBoldEnd;
			s.SetTitle(wFooter->GetString(s.GetTitle()));
			mTagEditor->at(option) << fmtBold << "Title:" << fmtBoldEnd << ' ' << ShowTag(s.GetTitle());
			break;
		}
		case 2:
		{
			Statusbar() << fmtBold << "Artist: " << fmtBoldEnd;
			s.SetArtist(wFooter->GetString(s.GetArtist()));
			mTagEditor->at(option) << fmtBold << "Artist:" << fmtBoldEnd << ' ' << ShowTag(s.GetArtist());
			break;
		}
		case 3:
		{
			Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
			s.SetAlbum(wFooter->GetString(s.GetAlbum()));
			mTagEditor->at(option) << fmtBold << "Album:" << fmtBoldEnd << ' ' << ShowTag(s.GetAlbum());
			break;
		}
		case 4:
		{
			Statusbar() << fmtBold << "Year: " << fmtBoldEnd;
			s.SetYear(wFooter->GetString(s.GetYear(), 4));
			mTagEditor->at(option) << fmtBold << "Year:" << fmtBoldEnd << ' ' << ShowTag(s.GetYear());
			break;
		}
		case 5:
		{
			Statusbar() << fmtBold << "Track: " << fmtBoldEnd;
			s.SetTrack(wFooter->GetString(s.GetTrack(), 3));
			mTagEditor->at(option) << fmtBold << "Track:" << fmtBoldEnd << ' ' << ShowTag(s.GetTrack());
			break;
		}
		case 6:
		{
			Statusbar() << fmtBold << "Genre: " << fmtBoldEnd;
			s.SetGenre(wFooter->GetString(s.GetGenre()));
			mTagEditor->at(option) << fmtBold << "Genre:" << fmtBoldEnd << ' ' << ShowTag(s.GetGenre());
			break;
		}
		case 7:
		{
			Statusbar() << fmtBold << "Composer: " << fmtBoldEnd;
			s.SetComposer(wFooter->GetString(s.GetComposer()));
			mTagEditor->at(option) << fmtBold << "Composer:" << fmtBoldEnd << ' ' << ShowTag(s.GetComposer());
			break;
		}
		case 8:
		{
			Statusbar() << fmtBold << "Performer: " << fmtBoldEnd;
			s.SetPerformer(wFooter->GetString(s.GetPerformer()));
			mTagEditor->at(option) << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(s.GetPerformer());
			break;
		}
		case 9:
		{
			Statusbar() << fmtBold << "Disc: " << fmtBoldEnd;
			s.SetDisc(wFooter->GetString(s.GetDisc()));
			mTagEditor->at(option) << fmtBold << "Disc:" << fmtBoldEnd << ' ' << ShowTag(s.GetDisc());
			break;
		}
		case 10:
		{
			Statusbar() << fmtBold << "Comment: " << fmtBoldEnd;
			s.SetComment(wFooter->GetString(s.GetComment()));
			mTagEditor->at(option) << fmtBold << "Comment:" << fmtBoldEnd << ' ' << ShowTag(s.GetComment());
			break;
		}
		case 12:
		{
			Statusbar() << fmtBold << "Filename: " << fmtBoldEnd;
			string filename = s.GetNewName().empty() ? s.GetName() : s.GetNewName();
			size_t dot = filename.rfind(".");
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
					Mpd->UpdateDirectory(locale_to_utf_cpy(s.GetDirectory()));
					if (prev_screen == csSearcher)
						*mySearcher->Main()->Current().second = s;
				}
				else
				{
					if (wPrev == myPlaylist->Main())
						myPlaylist->Main()->Current() = s;
					else if (wPrev == myBrowser->Main())
						*myBrowser->Main()->Current().song = s;
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
			redraw_header = 1;
			if (current_screen == csLibrary)
			{
				myLibrary->Refresh();
			}
			else if (current_screen == csPlaylistEditor)
			{
				myPlaylistEditor->Refresh();
			}
			else if (current_screen == csTagEditor)
			{
				TagEditor::Refresh();
			}
			break;
		}
	}
	UnlockStatusbar();
}

namespace TagEditor
{
	const size_t middle_col_width = 26;
	size_t left_col_width;
	size_t middle_col_startx;
	size_t right_col_width;
	size_t right_col_startx;
}

void TagEditor::Init()
{
	left_col_width = COLS/2-middle_col_width/2;
	middle_col_startx = left_col_width+1;
	right_col_width = COLS-left_col_width-middle_col_width-2;
	right_col_startx = left_col_width+middle_col_width+2;
	
	mEditorAlbums = new Menu<string_pair>(0, main_start_y, left_col_width, main_height, "Albums", Config.main_color, brNone);
	mEditorAlbums->HighlightColor(Config.active_column_color);
	mEditorAlbums->SetTimeout(ncmpcpp_window_timeout);
	mEditorAlbums->SetItemDisplayer(Display::StringPairs);
	
	mEditorDirs = new Menu<string_pair>(0, main_start_y, left_col_width, main_height, "Directories", Config.main_color, brNone);
	mEditorDirs->HighlightColor(Config.active_column_color);
	mEditorDirs->SetTimeout(ncmpcpp_window_timeout);
	mEditorDirs->SetItemDisplayer(Display::StringPairs);
	mEditorLeftCol = Config.albums_in_tag_editor ? mEditorAlbums : mEditorDirs;
	
	mEditorTagTypes = new Menu<string>(middle_col_startx, main_start_y, middle_col_width, main_height, "Tag types", Config.main_color, brNone);
	mEditorTagTypes->HighlightColor(Config.main_highlight_color);
	mEditorTagTypes->SetTimeout(ncmpcpp_window_timeout);
	mEditorTagTypes->SetItemDisplayer(Display::Generic);
	
	mEditorTags = new Menu<Song>(right_col_startx, main_start_y, right_col_width, main_height, "Tags", Config.main_color, brNone);
	mEditorTags->HighlightColor(Config.main_highlight_color);
	mEditorTags->SetTimeout(ncmpcpp_window_timeout);
	mEditorTags->SetSelectPrefix(&Config.selected_item_prefix);
	mEditorTags->SetSelectSuffix(&Config.selected_item_suffix);
	mEditorTags->SetItemDisplayer(Display::Tags);
	mEditorTags->SetItemDisplayerUserData(mEditorTagTypes);
	
	wTagEditorActiveCol = mEditorLeftCol;
}

void TagEditor::Resize()
{
	left_col_width = COLS/2-middle_col_width/2;
	middle_col_startx = left_col_width+1;
	right_col_width = COLS-left_col_width-middle_col_width-2;
	right_col_startx = left_col_width+middle_col_width+2;
	
	mEditorAlbums->Resize(left_col_width, main_height);
	mEditorDirs->Resize(left_col_width, main_height);
	mEditorTagTypes->Resize(middle_col_width, main_height);
	mEditorTags->Resize(right_col_width, main_height);
	
	mEditorTagTypes->MoveTo(middle_col_startx, main_start_y);
	mEditorTags->MoveTo(right_col_startx, main_start_y);
}

void TagEditor::Refresh()
{
	mEditorLeftCol->Display();
	mvvline(main_start_y, middle_col_startx-1, 0, main_height);
	mEditorTagTypes->Display();
	mvvline(main_start_y, right_col_startx-1, 0, main_height);
	mEditorTags->Display();
}

void TagEditor::SwitchTo()
{
	if (current_screen != csTagEditor && current_screen != csTinyTagEditor)
	{
		CLEAR_FIND_HISTORY;
		
		myPlaylist->Main()->Hide(); // hack, should be wCurrent, but it doesn't always have 100% width
		
//		redraw_screen = 1;
		redraw_header = 1;
		TagEditor::Refresh();
		
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
			mEditorTagTypes->AddSeparator();
			mEditorTagTypes->AddOption("Capitalize First Letters");
			mEditorTagTypes->AddOption("lower all letters");
		}
		
		wCurrent = wTagEditorActiveCol;
		current_screen = csTagEditor;
	}
}

void TagEditor::Update()
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
				{
					l[0]->Localize();
					maplist[l[0]->toString(Config.tag_editor_album_format)] = *it;
				}
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
				size_t slash = editor_browsed_dir.rfind("/");
				string parent = slash != string::npos ? editor_browsed_dir.substr(0, slash) : "/";
				mEditorDirs->AddOption(make_pair("[..]", parent));
			}
			else
			{
				mEditorDirs->AddOption(make_pair(".", "/"));
			}
			for (TagList::const_iterator it = list.begin(); it != list.end(); it++)
			{
				size_t slash = it->rfind("/");
				string to_display = slash != string::npos ? it->substr(slash+1) : *it;
				utf_to_locale(to_display);
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
			{
				(*it)->Localize();
				mEditorTags->AddOption(**it);
			}
		}
		else
		{
			Mpd->GetSongs(mEditorDirs->Current().second, list);
			sort(list.begin(), list.end(), CaseInsensitiveSorting());
			for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
			{
				(*it)->Localize();
				mEditorTags->AddOption(**it);
			}
		}
		FreeSongList(list);
		mEditorTags->Window::Clear();
		mEditorTags->Refresh();
	}
	
	if (/*redraw_screen && */wCurrent == mEditorTagTypes && mEditorTagTypes->Choice() < 13)
	{
		mEditorTags->Refresh();
//		redraw_screen = 0;
	}
	else if (mEditorTagTypes->Choice() >= 13)
		mEditorTags->Window::Clear();
}

void TagEditor::EnterPressed()
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
		return;
	}
	
	if (mEditorTags->Empty()) // we need songs to deal with, don't we?
		return;
	
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
	
	size_t id = mEditorTagTypes->RealChoice();
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
				TagEditor::Refresh();
			}
			else if (wCurrent == mEditorTags)
			{
				Song &s = mEditorTags->Current();
				string old_name = s.GetNewName().empty() ? s.GetName() : s.GetNewName();
				size_t last_dot = old_name.rfind(".");
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
			return;
		}
		case 11: // reset
		{
			mEditorTags->Clear(0);
			ShowMessage("Changes reset");
			return;
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
			return;
		}
		case 13: // capitalize first letters
		{
			ShowMessage("Processing...");
			for (SongList::iterator it = list.begin(); it != list.end(); it++)
				CapitalizeFirstLetters(**it);
			ShowMessage("Done!");
			break;
		}
		case 14: // lower all letters
		{
			ShowMessage("Processing...");
			for (SongList::iterator it = list.begin(); it != list.end(); it++)
				LowerAllLetters(**it);
			ShowMessage("Done!");
			break;
		}
		default:
			break;
	}
	
	if (wCurrent == mEditorTagTypes && id != 0 && id != 4 && set != NULL)
	{
		LockStatusbar();
		Statusbar() << fmtBold << mEditorTagTypes->Current() << fmtBoldEnd << ": ";
		string new_tag = wFooter->GetString((mEditorTags->Current().*get)());
		UnlockStatusbar();
		for (SongList::iterator it = list.begin(); it != list.end(); it++)
			(**it.*set)(new_tag);
	}
	else if (wCurrent == mEditorTags && set != NULL)
	{
		LockStatusbar();
		Statusbar() << fmtBold << mEditorTagTypes->Current() << fmtBoldEnd << ": ";
		string new_tag = wFooter->GetString((mEditorTags->Current().*get)());
		UnlockStatusbar();
		if (new_tag != (mEditorTags->Current().*get)())
			(mEditorTags->Current().*set)(new_tag);
		mEditorTags->Scroll(wDown);
	}
}

namespace
{
	const string patterns_list_file = config_dir + "patterns.list";
	vector<string> patterns_list;
	
	void GetPatternList()
	{
		if (patterns_list.empty())
		{
			std::ifstream input(patterns_list_file.c_str());
			if (input.is_open())
			{
				string line;
				while (getline(input, line))
				{
					if (!line.empty())
						patterns_list.push_back(line);
				}
				input.close();
			}
		}
	}
	
	void SavePatternList()
	{
		std::ofstream output(patterns_list_file.c_str());
		if (output.is_open())
		{
			for (vector<string>::const_iterator it = patterns_list.begin(); it != patterns_list.end() && it != patterns_list.begin()+30; it++)
				output << *it << std::endl;
			output.close();
		}
	}
	
	SongSetFunction IntoSetFunction(char c)
	{
		switch (c)
		{
			case 'a':
				return &Song::SetArtist;
			case 't':
				return &Song::SetTitle;
			case 'b':
				return &Song::SetAlbum;
			case 'y':
				return &Song::SetYear;
			case 'n':
				return &Song::SetTrack;
			case 'g':
				return &Song::SetGenre;
			case 'c':
				return &Song::SetComposer;
			case 'p':
				return &Song::SetPerformer;
			case 'd':
				return &Song::SetDisc;
			case 'C':
				return &Song::SetComment;
			default:
				return NULL;
		}
	}

	string GenerateFilename(const Song &s, string &pattern)
	{
		string result = s.toString(pattern);
		EscapeUnallowedChars(result);
		return result;
	}

	string ParseFilename(Song &s, string mask, bool preview)
	{
		std::ostringstream result;
		vector<string> separators;
		vector< std::pair<char, string> > tags;
		string file = s.GetName().substr(0, s.GetName().rfind("."));
		
		try
		{
			for (size_t i = mask.find("%"); i != string::npos; i = mask.find("%"))
			{
				tags.push_back(make_pair(mask.at(i+1), ""));
				mask = mask.substr(i+2);
				i = mask.find("%");
				if (!mask.empty())
					separators.push_back(mask.substr(0, i));
			}
			int i = 0;
			for (vector<string>::const_iterator it = separators.begin(); it != separators.end(); it++, i++)
			{
				int j = file.find(*it);
				tags.at(i).second = file.substr(0, j);
				file = file.substr(j+it->length());
			}
			if (!file.empty())
				tags.at(i).second = file;
		}
		catch (std::out_of_range)
		{
			return "Error while parsing filename!";
		}
		
		for (vector< std::pair<char, string> >::iterator it = tags.begin(); it != tags.end(); it++)
		{
			for (string::iterator j = it->second.begin(); j != it->second.end(); j++)
				if (*j == '_')
					*j = ' ';
			
			if (!preview)
			{
				SongSetFunction set = IntoSetFunction(it->first);
				if (set)
					(s.*set)(it->second);
			}
			else
				result << "%" << it->first << ": " << it->second << "\n";
		}
		return result.str();
	}
	
	string tag_capitalize_first_letters(const string &s)
	{
		if (s.empty())
			return "";
		string result = s;
		if (isalpha(result[0]))
			result[0] = toupper(result[0]);
		for (string::iterator it = result.begin()+1; it != result.end(); it++)
		{
			if (isalpha(*it) && !isalpha(*(it-1)))
				*it = toupper(*it);
		}
		return result;
	}
}

SongSetFunction IntoSetFunction(mpd_TagItems tag)
{
	switch (tag)
	{
		case MPD_TAG_ITEM_ARTIST:
			return &Song::SetArtist;
		case MPD_TAG_ITEM_ALBUM:
			return &Song::SetAlbum;
		case MPD_TAG_ITEM_TITLE:
			return &Song::SetTitle;
		case MPD_TAG_ITEM_TRACK:
			return &Song::SetTrack;
		case MPD_TAG_ITEM_GENRE:
			return &Song::SetGenre;
		case MPD_TAG_ITEM_DATE:
			return &Song::SetYear;
		case MPD_TAG_ITEM_COMPOSER:
			return &Song::SetComposer;
		case MPD_TAG_ITEM_PERFORMER:
			return &Song::SetPerformer;
		case MPD_TAG_ITEM_COMMENT:
			return &Song::SetComment;
		case MPD_TAG_ITEM_DISC:
			return &Song::SetDisc;
		case MPD_TAG_ITEM_FILENAME:
			return &Song::SetNewName;
		default:
			return NULL;
	}
}

string FindSharedDir(Menu<Song> *menu)
{
	SongList list;
	for (size_t i = 0; i < menu->Size(); i++)
		list.push_back(&menu->at(i));
	return FindSharedDir(list);
}

string FindSharedDir(const SongList &v)
{
	string result;
	if (!v.empty())
	{
		result = v.front()->GetFile();
		for (SongList::const_iterator it = v.begin()+1; it != v.end(); it++)
		{
			int i = 1;
			while (result.substr(0, i) == (*it)->GetFile().substr(0, i))
				i++;
			result = result.substr(0, i);
		}
		size_t slash = result.rfind("/");
		result = slash != string::npos ? result.substr(0, slash) : "/";
	}
	return result;
}

void ReadTagsFromFile(mpd_Song *s)
{
	TagLib::FileRef f(s->file);
	if (f.isNull())
		return;
	
	TagLib::MPEG::File *mpegf = dynamic_cast<TagLib::MPEG::File *>(f.file());

	s->artist = !f.tag()->artist().isEmpty() ? str_pool_get(f.tag()->artist().toCString(1)) : 0;
	s->title = !f.tag()->title().isEmpty() ? str_pool_get(f.tag()->title().toCString(1)) : 0;
	s->album = !f.tag()->album().isEmpty() ? str_pool_get(f.tag()->album().toCString(1)) : 0;
	s->track = f.tag()->track() ? str_pool_get(IntoStr(f.tag()->track()).c_str()) : 0;
	s->date = f.tag()->year() ? str_pool_get(IntoStr(f.tag()->year()).c_str()) : 0;
	s->genre = !f.tag()->genre().isEmpty() ? str_pool_get(f.tag()->genre().toCString(1)) : 0;
	if (mpegf)
	{
		s->composer = !mpegf->ID3v2Tag()->frameListMap()["TCOM"].isEmpty()
		? (!mpegf->ID3v2Tag()->frameListMap()["TCOM"].front()->toString().isEmpty()
		   ? str_pool_get(mpegf->ID3v2Tag()->frameListMap()["TCOM"].front()->toString().toCString(1))
		   : 0)
		: 0;
		s->performer = !mpegf->ID3v2Tag()->frameListMap()["TOPE"].isEmpty()
		? (!mpegf->ID3v2Tag()->frameListMap()["TOPE"].front()->toString().isEmpty()
		   ? str_pool_get(mpegf->ID3v2Tag()->frameListMap()["TOPE"].front()->toString().toCString(1))
		   : 0)
		: 0;
		s->disc = !mpegf->ID3v2Tag()->frameListMap()["TPOS"].isEmpty()
		? (!mpegf->ID3v2Tag()->frameListMap()["TPOS"].front()->toString().isEmpty()
		   ? str_pool_get(mpegf->ID3v2Tag()->frameListMap()["TPOS"].front()->toString().toCString(1))
		   : 0)
		: 0;
	}
	s->comment = !f.tag()->comment().isEmpty() ? str_pool_get(f.tag()->comment().toCString(1)) : 0;
	s->time = f.audioProperties()->length();
}

bool GetSongTags(Song &s)
{
	string path_to_file;
	if (s.IsFromDB())
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	locale_to_utf(path_to_file);
	
	TagLib::FileRef f(path_to_file.c_str());
	if (f.isNull())
		return false;
	s.SetComment(f.tag()->comment().to8Bit(1));
	
	string ext = s.GetFile();
	ext = ext.substr(ext.rfind(".")+1);
	ToLower(ext);
	
	mTagEditor->Clear();
	mTagEditor->Reset();
	
	mTagEditor->ResizeBuffer(23);
	
	for (size_t i = 0; i < 7; i++)
		mTagEditor->Static(i, 1);
	
	mTagEditor->IntoSeparator(7);
	mTagEditor->IntoSeparator(18);
	mTagEditor->IntoSeparator(20);
	
	if (ext != "mp3")
		for (size_t i = 14; i <= 16; i++)
			mTagEditor->Static(i, 1);
	
	mTagEditor->Highlight(8);
	
	mTagEditor->at(0) << fmtBold << Config.color1 << "Song name: " << fmtBoldEnd << Config.color2 << s.GetName() << clEnd;
	mTagEditor->at(1) << fmtBold << Config.color1 << "Location in DB: " << fmtBoldEnd << Config.color2 << ShowTag(s.GetDirectory()) << clEnd;
	mTagEditor->at(3) << fmtBold << Config.color1 << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << clEnd;
	mTagEditor->at(4) << fmtBold << Config.color1 << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps" << clEnd;
	mTagEditor->at(5) << fmtBold << Config.color1 << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz" << clEnd;
	mTagEditor->at(6) << fmtBold << Config.color1 << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << clDefault;
	
	mTagEditor->at(8) << fmtBold << "Title:" << fmtBoldEnd << ' ' << ShowTag(s.GetTitle());
	mTagEditor->at(9) << fmtBold << "Artist:" << fmtBoldEnd << ' ' << ShowTag(s.GetArtist());
	mTagEditor->at(10) << fmtBold << "Album:" << fmtBoldEnd << ' ' << ShowTag(s.GetAlbum());
	mTagEditor->at(11) << fmtBold << "Year:" << fmtBoldEnd << ' ' << ShowTag(s.GetYear());
	mTagEditor->at(12) << fmtBold << "Track:" << fmtBoldEnd << ' ' << ShowTag(s.GetTrack());
	mTagEditor->at(13) << fmtBold << "Genre:" << fmtBoldEnd << ' ' << ShowTag(s.GetGenre());
	mTagEditor->at(14) << fmtBold << "Composer:" << fmtBoldEnd << ' ' << ShowTag(s.GetComposer());
	mTagEditor->at(15) << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(s.GetPerformer());
	mTagEditor->at(16) << fmtBold << "Disc:" << fmtBoldEnd << ' ' << ShowTag(s.GetDisc());
	mTagEditor->at(17) << fmtBold << "Comment:" << fmtBoldEnd << ' ' << ShowTag(s.GetComment());

	mTagEditor->at(19) << fmtBold << "Filename:" << fmtBoldEnd << ' ' << s.GetName();

	mTagEditor->at(21) << "Save";
	mTagEditor->at(22) << "Cancel";
	return true;
}

bool WriteTags(Song &s)
{
	using namespace TagLib;
	string path_to_file;
	bool file_is_from_db = s.IsFromDB();
	if (file_is_from_db)
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	locale_to_utf(path_to_file);
	FileRef f(path_to_file.c_str());
	if (!f.isNull())
	{
		f.tag()->setTitle(ToWString(s.GetTitle()));
		f.tag()->setArtist(ToWString(s.GetArtist()));
		f.tag()->setAlbum(ToWString(s.GetAlbum()));
		f.tag()->setYear(StrToInt(s.GetYear()));
		f.tag()->setTrack(StrToInt(s.GetTrack()));
		f.tag()->setGenre(ToWString(s.GetGenre()));
		f.tag()->setComment(ToWString(s.GetComment()));
		if (!f.save())
			return false;
		
		string ext = s.GetFile();
		ext = ext.substr(ext.rfind(".")+1);
		ToLower(ext);
		if (ext == "mp3")
		{
			MPEG::File file(path_to_file.c_str());
			ID3v2::Tag *tag = file.ID3v2Tag();
			String::Type encoding = String::UTF8;
			ByteVector Composer("TCOM");
			ByteVector Performer("TOPE");
			ByteVector Disc("TPOS");
			ID3v2::Frame *ComposerFrame = new ID3v2::TextIdentificationFrame(Composer, encoding);
			ID3v2::Frame *PerformerFrame = new ID3v2::TextIdentificationFrame(Performer, encoding);
			ID3v2::Frame *DiscFrame = new ID3v2::TextIdentificationFrame(Disc, encoding);
			ComposerFrame->setText(ToWString(s.GetComposer()));
			PerformerFrame->setText(ToWString(s.GetPerformer()));
			DiscFrame->setText(ToWString(s.GetDisc()));
			tag->removeFrames(Composer);
			tag->addFrame(ComposerFrame);
			tag->removeFrames(Performer);
			tag->addFrame(PerformerFrame);
			tag->removeFrames(Disc);
			tag->addFrame(DiscFrame);
			if (!file.save())
				return false;
		}
		if (!s.GetNewName().empty())
		{
			string new_name;
			if (file_is_from_db)
				new_name += Config.mpd_music_dir;
			new_name += s.GetDirectory() + "/" + s.GetNewName();
			locale_to_utf(new_name);
			if (rename(path_to_file.c_str(), new_name.c_str()) == 0 && !file_is_from_db)
			{
				if (wPrev == myPlaylist->Main())
				{
					// if we rename local file, it won't get updated
					// so just remove it from playlist and add again
					size_t pos = myPlaylist->Main()->Choice();
					Mpd->QueueDeleteSong(pos);
					Mpd->CommitQueue();
					int id = Mpd->AddSong("file://" + new_name);
					if (id >= 0)
					{
						s = myPlaylist->Main()->Back();
						Mpd->Move(s.GetPosition(), pos);
					}
				}
				else // only myBrowser->Main()
					s.SetFile(new_name);
			}
		}
		return true;
	}
	else
		return false;
}

void __deal_with_filenames(SongList &v)
{
	int width = 30;
	int height = 6;
	
	GetPatternList();
	
	Menu<string> *Main = new Menu<string>((COLS-width)/2, (LINES-height)/2, width, height, "", Config.main_color, Config.window_border);
	Main->SetTimeout(ncmpcpp_window_timeout);
	Main->SetItemDisplayer(Display::Generic);
	Main->AddOption("Get tags from filename");
	Main->AddOption("Rename files");
	Main->AddSeparator();
	Main->AddOption("Cancel");
	Main->Display();
	
	int input = 0;
	while (!Keypressed(input, Key.Enter))
	{
		TraceMpdStatus();
		Main->Refresh();
		Main->ReadKey(input);
		if (Keypressed(input, Key.Down))
			Main->Scroll(wDown);
		else if (Keypressed(input, Key.Up))
			Main->Scroll(wUp);
	}
	
	width = COLS*0.9;
	height = LINES*0.8;
	bool exit = 0;
	bool preview = 1;
	size_t choice = Main->Choice();
	size_t one_width = width/2;
	size_t two_width = width-one_width;
	
	delete Main;
	
	Main = 0;
	Scrollpad *Helper = 0;
	Scrollpad *Legend = 0;
	Scrollpad *Preview = 0;
	Window *Active = 0;
	
	if (choice != 3)
	{
		Legend = new Scrollpad((COLS-width)/2+one_width, (LINES-height)/2, two_width, height, "Legend", Config.main_color, Config.window_border);
		Legend->SetTimeout(ncmpcpp_window_timeout);
		*Legend << "%a - artist\n";
		*Legend << "%t - title\n";
		*Legend << "%b - album\n";
		*Legend << "%y - year\n";
		*Legend << "%n - track number\n";
		*Legend << "%g - genre\n";
		*Legend << "%c - composer\n";
		*Legend << "%p - performer\n";
		*Legend << "%d - disc\n";
		*Legend << "%C - comment\n\n";
		*Legend << fmtBold << "Files:\n" << fmtBoldEnd;
		for (SongList::const_iterator it = v.begin(); it != v.end(); it++)
			*Legend << Config.color2 << " * " << clEnd << (*it)->GetName() << "\n";
		Legend->Flush();
		
		Preview = Legend->EmptyClone();
		Preview->SetTitle("Preview");
		Preview->SetTimeout(ncmpcpp_window_timeout);
		
		Main = new Menu<string>((COLS-width)/2, (LINES-height)/2, one_width, height, "", Config.main_color, Config.active_window_border);
		Main->SetTimeout(ncmpcpp_window_timeout);
		Main->SetItemDisplayer(Display::Generic);
		
		if (!patterns_list.empty())
			Config.pattern = patterns_list.front();
		Main->AddOption("Pattern: " + Config.pattern);
		Main->AddOption("Preview");
		Main->AddOption("Legend");
		Main->AddSeparator();
		Main->AddOption("Proceed");
		Main->AddOption("Cancel");
		if (!patterns_list.empty())
		{
			Main->AddSeparator();
			Main->AddOption("Recent patterns", 1, 1, 0);
			Main->AddSeparator();
			for (vector<string>::const_iterator it = patterns_list.begin(); it != patterns_list.end(); it++)
				Main->AddOption(*it);
		}
		
		Active = Main;
		Helper = Legend;
		
		Main->SetTitle(!choice ? "Get tags from filename" : "Rename files");
		Main->Display();
		Helper->Display();
		
		while (!exit)
		{
			TraceMpdStatus();
			Active->Refresh();
			Active->ReadKey(input);
				
			if (Keypressed(input, Key.Up))
				Active->Scroll(wUp);
			else if (Keypressed(input, Key.Down))
				Active->Scroll(wDown);
			else if (Keypressed(input, Key.PageUp))
				Active->Scroll(wPageUp);
			else if (Keypressed(input, Key.PageDown))
				Active->Scroll(wPageDown);
			else if (Keypressed(input, Key.Home))
				Active->Scroll(wHome);
			else if (Keypressed(input, Key.End))
				Active->Scroll(wEnd);
			else if (Keypressed(input, Key.Enter) && Active == Main)
			{
				switch (Main->RealChoice())
				{
					case 0:
					{
						LockStatusbar();
						Statusbar() << "Pattern: ";
						string new_pattern = wFooter->GetString(Config.pattern);
						UnlockStatusbar();
						if (!new_pattern.empty())
						{
							Config.pattern = new_pattern;
							Main->at(0) = "Pattern: ";
							Main->at(0) += Config.pattern;
						}
						break;
					}
					case 3: // save
						preview = 0;
					case 1:
					{
						bool success = 1;
						ShowMessage("Parsing...");
						Preview->Clear();
						for (SongList::iterator it = v.begin(); it != v.end(); it++)
						{
							Song &s = **it;
							if (!choice)
							{
								if (preview)
								{
									*Preview << fmtBold << s.GetName() << ":\n" << fmtBoldEnd;
									*Preview << ParseFilename(s, Config.pattern, preview) << "\n";
								}
								else
									ParseFilename(s, Config.pattern, preview);
							}
							else
							{
								const string &file = s.GetName();
								size_t last_dot = file.rfind(".");
								string extension = file.substr(last_dot);
								basic_buffer<my_char_t> new_file;
								new_file << TO_WSTRING(GenerateFilename(s, Config.pattern));
								if (new_file.Str().empty())
								{
									if (preview)
										new_file << clRed << "!EMPTY!" << clEnd;
									else
									{
										ShowMessage("File '%s' would have an empty name!", s.GetName().c_str());
										success = 0;
										break;
									}
								}
								if (!preview)
									s.SetNewName(TO_STRING(new_file.Str()) + extension);
								*Preview << file << Config.color2 << " -> " << clEnd << new_file << extension << "\n\n";
							}
						}
						if (!success)
						{
							preview = 1;
							continue;
						}
						else
						{
							for (size_t i = 0; i < patterns_list.size(); i++)
							{
								if (patterns_list[i] == Config.pattern)
								{
									patterns_list.erase(patterns_list.begin()+i);
									i--;
								}
							}
							patterns_list.insert(patterns_list.begin(), Config.pattern);
						}
						ShowMessage("Operation finished!");
						if (preview)
						{
							Helper = Preview;
							Helper->Flush();
							Helper->Display();
							break;
						}
					}
					case 4:
					{
						exit = 1;
						break;
					}
					case 2:
					{
						Helper = Legend;
						Helper->Display();
						break;
					}
					default:
					{
						Config.pattern = Main->Current();
						Main->at(0) = "Pattern: " + Config.pattern;
					}
				}
			}
			else if (Keypressed(input, Key.VolumeUp) && Active == Main)
			{
				Active->SetBorder(Config.window_border);
				Active->Display();
				Active = Helper;
				Active->SetBorder(Config.active_window_border);
				Active->Display();
			}
			else if (Keypressed(input, Key.VolumeDown) && Active == Helper)
			{
				Active->SetBorder(Config.window_border);
				Active->Display();
				Active = Main;
				Active->SetBorder(Config.active_window_border);
				Active->Display();
			}
		}
	}
	SavePatternList();
	delete Main;
	delete Legend;
	delete Preview;
}

void CapitalizeFirstLetters(Song &s)
{
	s.SetTitle(tag_capitalize_first_letters(s.GetTitle()));
	s.SetArtist(tag_capitalize_first_letters(s.GetArtist()));
	s.SetAlbum(tag_capitalize_first_letters(s.GetAlbum()));
	s.SetGenre(tag_capitalize_first_letters(s.GetGenre()));
	s.SetComposer(tag_capitalize_first_letters(s.GetComposer()));
	s.SetPerformer(tag_capitalize_first_letters(s.GetPerformer()));
	s.SetDisc(tag_capitalize_first_letters(s.GetDisc()));
	s.SetComment(tag_capitalize_first_letters(s.GetComment()));
}

void LowerAllLetters(Song &s)
{
	string conv = s.GetTitle();
	ToLower(conv);
	s.SetTitle(conv);
	
	conv = s.GetArtist();
	ToLower(conv);
	s.SetArtist(conv);
	
	conv = s.GetAlbum();
	ToLower(conv);
	s.SetAlbum(conv);
	
	conv = s.GetGenre();
	ToLower(conv);
	s.SetGenre(conv);
	
	conv = s.GetComposer();
	ToLower(conv);
	s.SetComposer(conv);
	
	conv = s.GetPerformer();
	ToLower(conv);
	s.SetPerformer(conv);
	
	conv = s.GetDisc();
	ToLower(conv);
	s.SetDisc(conv);
	
	conv = s.GetComment();
	ToLower(conv);
	s.SetComment(conv);
}

#endif

