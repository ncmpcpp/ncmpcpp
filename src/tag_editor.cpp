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
#include "status.h"

using namespace Global;
using namespace MPD;

TinyTagEditor *myTinyTagEditor = new TinyTagEditor;

void TinyTagEditor::Init()
{
	w = new Menu<Buffer>(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	w->HighlightColor(Config.main_highlight_color);
	w->SetTimeout(ncmpcpp_window_timeout);
	w->CyclicScrolling(Config.use_cyclic_scrolling);
	w->SetItemDisplayer(Display::Generic);
	isInitialized = 1;
}

void TinyTagEditor::Resize()
{
	w->Resize(COLS, MainHeight);
	hasToBeResized = 0;
}

void TinyTagEditor::SwitchTo()
{
	if (itsEdited.IsStream())
	{
		ShowMessage("Cannot edit streams!");
	}
	else if (GetTags())
	{
		if (hasToBeResized)
			Resize();
		myOldScreen = myScreen;
		myScreen = this;
		RedrawHeader = 1;
	}
	else
	{
		string message = "Cannot read file '";
		if (itsEdited.IsFromDB())
			message += Config.mpd_music_dir;
		message += itsEdited.GetFile();
		message += "'!";
		ShowMessage("%s", message.c_str());
	}
}

std::string TinyTagEditor::Title()
{
	return "Tiny tag editor";
}

void TinyTagEditor::EnterPressed()
{
	size_t option = w->Choice();
	LockStatusbar();
	
	if (option >= 8 && option <= 20)
		w->at(option).Clear();
	
	Song &s = itsEdited;
	
	switch (option-7)
	{
		case 1:
		{
			Statusbar() << fmtBold << "Title: " << fmtBoldEnd;
			s.SetTitle(wFooter->GetString(s.GetTitle()));
			w->at(option) << fmtBold << "Title:" << fmtBoldEnd << ' ' << ShowTag(s.GetTitle());
			break;
		}
		case 2:
		{
			Statusbar() << fmtBold << "Artist: " << fmtBoldEnd;
			s.SetArtist(wFooter->GetString(s.GetArtist()));
			w->at(option) << fmtBold << "Artist:" << fmtBoldEnd << ' ' << ShowTag(s.GetArtist());
			break;
		}
		case 3:
		{
			Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
			s.SetAlbum(wFooter->GetString(s.GetAlbum()));
			w->at(option) << fmtBold << "Album:" << fmtBoldEnd << ' ' << ShowTag(s.GetAlbum());
			break;
		}
		case 4:
		{
			Statusbar() << fmtBold << "Year: " << fmtBoldEnd;
			s.SetYear(wFooter->GetString(s.GetYear(), 4));
			w->at(option) << fmtBold << "Year:" << fmtBoldEnd << ' ' << ShowTag(s.GetYear());
			break;
		}
		case 5:
		{
			Statusbar() << fmtBold << "Track: " << fmtBoldEnd;
			s.SetTrack(wFooter->GetString(s.GetTrack(), 3));
			w->at(option) << fmtBold << "Track:" << fmtBoldEnd << ' ' << ShowTag(s.GetTrack());
			break;
		}
		case 6:
		{
			Statusbar() << fmtBold << "Genre: " << fmtBoldEnd;
			s.SetGenre(wFooter->GetString(s.GetGenre()));
			w->at(option) << fmtBold << "Genre:" << fmtBoldEnd << ' ' << ShowTag(s.GetGenre());
			break;
		}
		case 7:
		{
			Statusbar() << fmtBold << "Composer: " << fmtBoldEnd;
			s.SetComposer(wFooter->GetString(s.GetComposer()));
			w->at(option) << fmtBold << "Composer:" << fmtBoldEnd << ' ' << ShowTag(s.GetComposer());
			break;
		}
		case 8:
		{
			Statusbar() << fmtBold << "Performer: " << fmtBoldEnd;
			s.SetPerformer(wFooter->GetString(s.GetPerformer()));
			w->at(option) << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(s.GetPerformer());
			break;
		}
		case 9:
		{
			Statusbar() << fmtBold << "Disc: " << fmtBoldEnd;
			s.SetDisc(wFooter->GetString(s.GetDisc()));
			w->at(option) << fmtBold << "Disc:" << fmtBoldEnd << ' ' << ShowTag(s.GetDisc());
			break;
		}
		case 10:
		{
			Statusbar() << fmtBold << "Comment: " << fmtBoldEnd;
			s.SetComment(wFooter->GetString(s.GetComment()));
			w->at(option) << fmtBold << "Comment:" << fmtBoldEnd << ' ' << ShowTag(s.GetComment());
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
			w->at(option) << fmtBold << "Filename:" << fmtBoldEnd << ' ' << (s.GetNewName().empty() ? s.GetName() : s.GetNewName());
			break;
		}
		case 14:
		{
			ShowMessage("Updating tags...");
			if (TagEditor::WriteTags(s))
			{
				ShowMessage("Tags updated!");
				if (s.IsFromDB())
				{
					Mpd.UpdateDirectory(locale_to_utf_cpy(s.GetDirectory()));
					if (myOldScreen == mySearcher)
						*mySearcher->Main()->Current().second = s;
				}
				else
				{
					if (myOldScreen == myPlaylist)
						myPlaylist->Main()->Current() = s;
					else if (myOldScreen == myBrowser)
						*myBrowser->Main()->Current().song = s;
				}
			}
			else
				ShowMessage("Error writing tags!");
		}
		case 15:
		{
			myOldScreen->SwitchTo();
			break;
		}
	}
	UnlockStatusbar();
}

void TinyTagEditor::MouseButtonPressed(MEVENT me)
{
	if (w->Empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->Size())
		return;
	if (me.bstate & BUTTON1_PRESSED)
	{
		if (!w->Goto(me.y))
			return;
		w->Refresh();
		EnterPressed();
	}
	else
		Screen< Menu<Buffer> >::MouseButtonPressed(me);
}

bool TinyTagEditor::SetEdited(MPD::Song *s)
{
	if (!s)
		return false;
	itsEdited = *s;
	return true;
}

bool TinyTagEditor::GetTags()
{
	Song &s = itsEdited;
	
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
	
	if (!isInitialized)
		Init();
	
	w->Clear();
	w->Reset();
	
	w->ResizeList(23);
	
	for (size_t i = 0; i < 7; ++i)
		w->Static(i, 1);
	
	w->IntoSeparator(7);
	w->IntoSeparator(18);
	w->IntoSeparator(20);
	
	if (ext != "mp3")
		for (size_t i = 14; i <= 16; ++i)
			w->Static(i, 1);
	
	w->Highlight(8);
	
	w->at(0) << fmtBold << Config.color1 << "Song name: " << fmtBoldEnd << Config.color2 << s.GetName() << clEnd;
	w->at(1) << fmtBold << Config.color1 << "Location in DB: " << fmtBoldEnd << Config.color2 << ShowTag(s.GetDirectory()) << clEnd;
	w->at(3) << fmtBold << Config.color1 << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << clEnd;
	w->at(4) << fmtBold << Config.color1 << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps" << clEnd;
	w->at(5) << fmtBold << Config.color1 << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz" << clEnd;
	w->at(6) << fmtBold << Config.color1 << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << clDefault;
	
	w->at(8) << fmtBold << "Title:" << fmtBoldEnd << ' ' << ShowTag(s.GetTitle());
	w->at(9) << fmtBold << "Artist:" << fmtBoldEnd << ' ' << ShowTag(s.GetArtist());
	w->at(10) << fmtBold << "Album:" << fmtBoldEnd << ' ' << ShowTag(s.GetAlbum());
	w->at(11) << fmtBold << "Year:" << fmtBoldEnd << ' ' << ShowTag(s.GetYear());
	w->at(12) << fmtBold << "Track:" << fmtBoldEnd << ' ' << ShowTag(s.GetTrack());
	w->at(13) << fmtBold << "Genre:" << fmtBoldEnd << ' ' << ShowTag(s.GetGenre());
	w->at(14) << fmtBold << "Composer:" << fmtBoldEnd << ' ' << ShowTag(s.GetComposer());
	w->at(15) << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(s.GetPerformer());
	w->at(16) << fmtBold << "Disc:" << fmtBoldEnd << ' ' << ShowTag(s.GetDisc());
	w->at(17) << fmtBold << "Comment:" << fmtBoldEnd << ' ' << ShowTag(s.GetComment());

	w->at(19) << fmtBold << "Filename:" << fmtBoldEnd << ' ' << s.GetName();

	w->at(21) << "Save";
	w->at(22) << "Cancel";
	return true;
}

TagEditor *myTagEditor = new TagEditor;

const string TagEditor::PatternsFile = config_dir + "patterns.list";
vector<string> TagEditor::Patterns;

const size_t TagEditor::MiddleColumnWidth = 26;
size_t TagEditor::LeftColumnWidth;
size_t TagEditor::MiddleColumnStartX;
size_t TagEditor::RightColumnWidth;
size_t TagEditor::RightColumnStartX;

void TagEditor::Init()
{
	LeftColumnWidth = COLS/2-MiddleColumnWidth/2;
	MiddleColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-MiddleColumnWidth-2;
	RightColumnStartX = LeftColumnWidth+MiddleColumnWidth+2;
	
	Albums = new Menu<string_pair>(0, MainStartY, LeftColumnWidth, MainHeight, "Albums", Config.main_color, brNone);
	Albums->HighlightColor(Config.active_column_color);
	Albums->SetTimeout(ncmpcpp_window_timeout);
	Albums->CyclicScrolling(Config.use_cyclic_scrolling);
	Albums->SetItemDisplayer(Display::Pairs);
	Albums->SetGetStringFunction(StringPairToString);
	
	Dirs = new Menu<string_pair>(0, MainStartY, LeftColumnWidth, MainHeight, "Directories", Config.main_color, brNone);
	Dirs->HighlightColor(Config.active_column_color);
	Dirs->SetTimeout(ncmpcpp_window_timeout);
	Dirs->CyclicScrolling(Config.use_cyclic_scrolling);
	Dirs->SetItemDisplayer(Display::Pairs);
	Dirs->SetGetStringFunction(StringPairToString);
	
	LeftColumn = Config.albums_in_tag_editor ? Albums : Dirs;
	
	TagTypes = new Menu<string>(MiddleColumnStartX, MainStartY, MiddleColumnWidth, MainHeight, "Tag types", Config.main_color, brNone);
	TagTypes->HighlightColor(Config.main_highlight_color);
	TagTypes->SetTimeout(ncmpcpp_window_timeout);
	TagTypes->CyclicScrolling(Config.use_cyclic_scrolling);
	TagTypes->SetItemDisplayer(Display::Generic);
	
	Tags = new Menu<Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, "Tags", Config.main_color, brNone);
	Tags->HighlightColor(Config.main_highlight_color);
	Tags->SetTimeout(ncmpcpp_window_timeout);
	Tags->CyclicScrolling(Config.use_cyclic_scrolling);
	Tags->SetSelectPrefix(&Config.selected_item_prefix);
	Tags->SetSelectSuffix(&Config.selected_item_suffix);
	Tags->SetItemDisplayer(Display::Tags);
	Tags->SetItemDisplayerUserData(TagTypes);
	Tags->SetGetStringFunction(TagToString);
	Tags->SetGetStringFunctionUserData(TagTypes);
	
	w = LeftColumn;
	isInitialized = 1;
}

void TagEditor::Resize()
{
	LeftColumnWidth = COLS/2-MiddleColumnWidth/2;
	MiddleColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-MiddleColumnWidth-2;
	RightColumnStartX = LeftColumnWidth+MiddleColumnWidth+2;
	
	Albums->Resize(LeftColumnWidth, MainHeight);
	Dirs->Resize(LeftColumnWidth, MainHeight);
	TagTypes->Resize(MiddleColumnWidth, MainHeight);
	Tags->Resize(RightColumnWidth, MainHeight);
	
	TagTypes->MoveTo(MiddleColumnStartX, MainStartY);
	Tags->MoveTo(RightColumnStartX, MainStartY);
	
	hasToBeResized = 0;
}

std::string TagEditor::Title()
{
	return "Tag editor";
}

void TagEditor::SwitchTo()
{
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (hasToBeResized)
		Resize();
	
	myScreen = this;
	RedrawHeader = 1;
	Refresh();
	
	if (TagTypes->Empty())
	{
		TagTypes->AddOption("Title");
		TagTypes->AddOption("Artist");
		TagTypes->AddOption("Album");
		TagTypes->AddOption("Year");
		TagTypes->AddOption("Track");
		TagTypes->AddOption("Genre");
		TagTypes->AddOption("Composer");
		TagTypes->AddOption("Performer");
		TagTypes->AddOption("Disc");
		TagTypes->AddOption("Comment");
		TagTypes->AddSeparator();
		TagTypes->AddOption("Filename");
		TagTypes->AddSeparator();
		TagTypes->AddOption("Options", 1, 1);
		TagTypes->AddSeparator();
		TagTypes->AddOption("Reset");
		TagTypes->AddOption("Save");
		TagTypes->AddSeparator();
		TagTypes->AddOption("Capitalize First Letters");
		TagTypes->AddOption("lower all letters");
	}
}

void TagEditor::Refresh()
{
	LeftColumn->Display();
	mvvline(MainStartY, MiddleColumnStartX-1, 0, MainHeight);
	TagTypes->Display();
	mvvline(MainStartY, RightColumnStartX-1, 0, MainHeight);
	Tags->Display();
}

void TagEditor::Update()
{
	if (LeftColumn->Empty())
	{
		LeftColumn->Window::Clear();
		Tags->Clear();
		TagList list;
		if (Config.albums_in_tag_editor)
		{
			*Albums << XY(0, 0) << "Fetching albums...";
			Albums->Window::Refresh();
			Mpd.GetAlbums("", list);
			for (TagList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				
				SongList l;
				Mpd.StartSearch(1);
				Mpd.AddSearch(MPD_TAG_ITEM_ALBUM, *it);
				Mpd.CommitSearch(l);
				if (!l.empty())
				{
					l[0]->Localize();
					Albums->AddOption(std::make_pair(l[0]->toString(Config.tag_editor_album_format), *it));
				}
				FreeSongList(l);
			}
			Albums->Sort<CaseInsensitiveSorting>();
		}
		else
		{
			int highlightme = -1;
			Mpd.GetDirectories(itsBrowsedDir, list);
			sort(list.begin(), list.end(), CaseInsensitiveSorting());
			if (itsBrowsedDir != "/")
			{
				size_t slash = itsBrowsedDir.rfind("/");
				string parent = slash != string::npos ? itsBrowsedDir.substr(0, slash) : "/";
				Dirs->AddOption(make_pair("[..]", parent));
			}
			else
			{
				Dirs->AddOption(make_pair(".", "/"));
			}
			for (TagList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				size_t slash = it->rfind("/");
				string to_display = slash != string::npos ? it->substr(slash+1) : *it;
				utf_to_locale(to_display);
				Dirs->AddOption(make_pair(to_display, *it));
				if (*it == itsHighlightedDir)
					highlightme = Dirs->Size()-1;
			}
			if (highlightme != -1)
				Dirs->Highlight(highlightme);
		}
		LeftColumn->Display();
		TagTypes->Refresh();
	}
	
	if (Tags->Empty())
	{
		Tags->Reset();
		SongList list;
		if (Config.albums_in_tag_editor)
		{
			Mpd.StartSearch(1);
			Mpd.AddSearch(MPD_TAG_ITEM_ALBUM, Albums->Current().second);
			Mpd.CommitSearch(list);
			sort(list.begin(), list.end(), CaseInsensitiveSorting());
			for (SongList::iterator it = list.begin(); it != list.end(); ++it)
			{
				(*it)->Localize();
				Tags->AddOption(**it);
			}
		}
		else
		{
			Mpd.GetSongs(Dirs->Current().second, list);
			sort(list.begin(), list.end(), CaseInsensitiveSorting());
			for (SongList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				(*it)->Localize();
				Tags->AddOption(**it);
			}
		}
		FreeSongList(list);
		Tags->Window::Clear();
		Tags->Refresh();
	}
	
	if (w == TagTypes && TagTypes->Choice() < 13)
	{
		Tags->Refresh();
	}
	else if (TagTypes->Choice() >= 13)
		Tags->Window::Clear();
}

void TagEditor::EnterPressed()
{
	if (w == Dirs)
	{
		TagList test;
		Mpd.GetDirectories(LeftColumn->Current().second, test);
		if (!test.empty())
		{
			itsHighlightedDir = itsBrowsedDir;
			itsBrowsedDir = LeftColumn->Current().second;
			LeftColumn->Clear(0);
			LeftColumn->Reset();
		}
		else
			ShowMessage("No subdirs found");
		return;
	}
	
	if (Tags->Empty()) // we need songs to deal with, don't we?
		return;
	
	// if there are selected songs, perform operations only on them
	SongList list;
	if (Tags->hasSelected())
	{
		vector<size_t> selected;
		Tags->GetSelected(selected);
		for (vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
			list.push_back(&Tags->at(*it));
	}
	else
		for (size_t i = 0; i < Tags->Size(); ++i)
			list.push_back(&Tags->at(i));
	
	Song::GetFunction get = 0;
	Song::SetFunction set = 0;
	
	size_t id = TagTypes->RealChoice();
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
			if (w == TagTypes)
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
					for (SongList::iterator it = list.begin(); it != list.end(); ++it, ++i)
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
			if (w == TagTypes)
			{
				DealWithFilenames(list);
				Refresh();
			}
			else if (w == Tags)
			{
				Song &s = Tags->Current();
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
				Tags->Scroll(wDown);
			}
			return;
		}
		case 11: // reset
		{
			Tags->Clear(0);
			ShowMessage("Changes reset");
			return;
		}
		case 12: // save
		{
			bool success = 1;
			ShowMessage("Writing changes...");
			for (SongList::iterator it = list.begin(); it != list.end(); ++it)
			{
				ShowMessage("Writing tags in \"%s\"...", (*it)->GetName().c_str());
				if (!WriteTags(**it))
				{
					ShowMessage("Error writing tags in \"%s\"!", (*it)->GetFile().c_str());
					success = 0;
					break;
				}
			}
			if (success)
			{
				ShowMessage("Tags updated!");
				TagTypes->HighlightColor(Config.main_highlight_color);
				TagTypes->Reset();
				w->Refresh();
				w = LeftColumn;
				LeftColumn->HighlightColor(Config.active_column_color);
				Mpd.UpdateDirectory(locale_to_utf_cpy(FindSharedDir(Tags)));
			}
			else
				Tags->Clear(0);
			return;
		}
		case 13: // capitalize first letters
		{
			ShowMessage("Processing...");
			for (SongList::iterator it = list.begin(); it != list.end(); ++it)
				CapitalizeFirstLetters(**it);
			ShowMessage("Done!");
			break;
		}
		case 14: // lower all letters
		{
			ShowMessage("Processing...");
			for (SongList::iterator it = list.begin(); it != list.end(); ++it)
				LowerAllLetters(**it);
			ShowMessage("Done!");
			break;
		}
		default:
			break;
	}
	
	if (w == TagTypes && id != 0 && id != 4 && set != NULL)
	{
		LockStatusbar();
		Statusbar() << fmtBold << TagTypes->Current() << fmtBoldEnd << ": ";
		string new_tag = wFooter->GetString((Tags->Current().*get)());
		UnlockStatusbar();
		for (SongList::iterator it = list.begin(); it != list.end(); ++it)
			(**it.*set)(new_tag);
	}
	else if (w == Tags && set != NULL)
	{
		LockStatusbar();
		Statusbar() << fmtBold << TagTypes->Current() << fmtBoldEnd << ": ";
		string new_tag = wFooter->GetString((Tags->Current().*get)());
		UnlockStatusbar();
		if (new_tag != (Tags->Current().*get)())
			(Tags->Current().*set)(new_tag);
		Tags->Scroll(wDown);
	}
}

void TagEditor::SpacePressed()
{
	if (w == Tags)
	{
		Tags->SelectCurrent();
		w->Scroll(wDown);
		return;
	}
	if (w != LeftColumn)
		return;
	
	Config.albums_in_tag_editor = !Config.albums_in_tag_editor;
	w = LeftColumn = Config.albums_in_tag_editor ? Albums : Dirs;
	ShowMessage("Switched to %s view", Config.albums_in_tag_editor ? "albums" : "directories");
	LeftColumn->Display();
	Tags->Clear(0);
}

void TagEditor::MouseButtonPressed(MEVENT me)
{
	if (!LeftColumn->Empty() && LeftColumn->hasCoords(me.x, me.y))
	{
		if (w != LeftColumn)
		{
			PrevColumn();
			PrevColumn();
		}
		if (size_t(me.y) < LeftColumn->Size() && (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED))
		{
			LeftColumn->Goto(me.y);
			if (me.bstate & BUTTON1_PRESSED)
				EnterPressed();
			else
				SpacePressed();
		}
		else
			Screen<Window>::MouseButtonPressed(me);
		Tags->Clear(0);
	}
	else if (!TagTypes->Empty() && TagTypes->hasCoords(me.x, me.y))
	{
		if (w != TagTypes)
			w == LeftColumn ? NextColumn() : PrevColumn();
		if (size_t(me.y) < TagTypes->Size() && (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED))
		{
			if (!TagTypes->Goto(me.y))
				return;
			TagTypes->Refresh();
			Tags->Refresh();
			if (me.bstate & BUTTON3_PRESSED)
				EnterPressed();
		}
		else
			Screen<Window>::MouseButtonPressed(me);
	}
	else if (!Tags->Empty() && Tags->hasCoords(me.x, me.y))
	{
		if (w != Tags)
		{
			NextColumn();
			NextColumn();
		}
		if (size_t(me.y) < Tags->Size() && (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED))
		{
			Tags->Goto(me.y);
			Tags->Refresh();
			if (me.bstate & BUTTON3_PRESSED)
				EnterPressed();
		}
		else
			Screen<Window>::MouseButtonPressed(me);
	}
}

MPD::Song *TagEditor::CurrentSong()
{
	return w == Tags && !Tags->Empty() ? &Tags->Current() : 0;
}

void TagEditor::GetSelectedSongs(MPD::SongList &v)
{
	std::vector<size_t> selected;
	Tags->GetSelected(selected);
	for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
	{
		v.push_back(new MPD::Song(Tags->at(*it)));
	}
}

void TagEditor::ApplyFilter(const std::string &s)
{
	if (w == Dirs)
		Dirs->ApplyFilter(s, 1, REG_ICASE | Config.regex_type);
	else if (w == Albums)
		Albums->ApplyFilter(s, 0, REG_ICASE | Config.regex_type);
	else if (w == Tags)
		Tags->ApplyFilter(s, 0, REG_ICASE | Config.regex_type);
}

List *TagEditor::GetList()
{
	if (w == LeftColumn)
		return LeftColumn;
	else if (w == Tags)
		return Tags;
	else
		return 0;
}

void TagEditor::NextColumn()
{
	if (w == LeftColumn)
	{
		LeftColumn->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = TagTypes;
		TagTypes->HighlightColor(Config.active_column_color);
	}
	else if (w == TagTypes && TagTypes->Choice() < 12 && !Tags->Empty())
	{
		TagTypes->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = myTagEditor->Tags;
		Tags->HighlightColor(Config.active_column_color);
	}
}

void TagEditor::PrevColumn()
{
	if (w == Tags)
	{
		Tags->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = TagTypes;
		TagTypes->HighlightColor(Config.active_column_color);
	}
	else if (w == TagTypes)
	{
		TagTypes->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = LeftColumn;
		LeftColumn->HighlightColor(Config.active_column_color);
	}
}

void TagEditor::ReadTags(mpd_Song *s)
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

bool TagEditor::WriteTags(Song &s)
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
			
			TagLib::StringList list;
			
			GetTagList(list, s.GetComposer());
			tag->removeFrames(Composer);
			ID3v2::TextIdentificationFrame *ComposerFrame = new ID3v2::TextIdentificationFrame(Composer, encoding);
			ComposerFrame->setText(list);
			tag->addFrame(ComposerFrame);
			
			GetTagList(list, s.GetPerformer());
			tag->removeFrames(Performer);
			ID3v2::TextIdentificationFrame *PerformerFrame = new ID3v2::TextIdentificationFrame(Performer, encoding);
			PerformerFrame->setText(list);
			tag->addFrame(PerformerFrame);
			
			GetTagList(list, s.GetDisc());
			tag->removeFrames(Disc);
			ID3v2::TextIdentificationFrame *DiscFrame = new ID3v2::TextIdentificationFrame(Disc, encoding);
			DiscFrame->setText(list);
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
				if (myOldScreen == myPlaylist)
				{
					// if we rename local file, it won't get updated
					// so just remove it from playlist and add again
					size_t pos = myPlaylist->Main()->Choice();
					Mpd.StartCommandsList();
					Mpd.Delete(pos);
					int id = Mpd.AddSong("file://" + new_name);
					if (id >= 0)
					{
						s = myPlaylist->Main()->Back();
						Mpd.Move(s.GetPosition(), pos);
					}
					Mpd.CommitCommandsList();
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

std::string TagEditor::CapitalizeFirstLetters(const string &s)
{
	if (s.empty())
		return "";
	string result = s;
	if (isalpha(result[0]))
		result[0] = toupper(result[0]);
	for (string::iterator it = result.begin()+1; it != result.end(); ++it)
	{
		if (isalpha(*it) && !isalpha(*(it-1)) && *(it-1) != '\'')
			*it = toupper(*it);
	}
	return result;
}

void TagEditor::CapitalizeFirstLetters(Song &s)
{
	s.SetTitle(CapitalizeFirstLetters(s.GetTitle()));
	s.SetArtist(CapitalizeFirstLetters(s.GetArtist()));
	s.SetAlbum(CapitalizeFirstLetters(s.GetAlbum()));
	s.SetGenre(CapitalizeFirstLetters(s.GetGenre()));
	s.SetComposer(CapitalizeFirstLetters(s.GetComposer()));
	s.SetPerformer(CapitalizeFirstLetters(s.GetPerformer()));
	s.SetDisc(CapitalizeFirstLetters(s.GetDisc()));
	s.SetComment(CapitalizeFirstLetters(s.GetComment()));
}

void TagEditor::LowerAllLetters(Song &s)
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

void TagEditor::GetTagList(TagLib::StringList &list, const std::string &s)
{
	list.clear();
	for (size_t i = 0; i != string::npos; i = s.find(",", i))
	{
		if (i)
			i++;
		size_t j = s.find(",", i);
		list.append(TagLib::String(s.substr(i, j-i), TagLib::String::UTF8));
	}
}

std::string TagEditor::TagToString(const MPD::Song &s, void *data)
{
	std::string result;
	switch (static_cast<Menu<string> *>(data)->Choice())
	{
		case 0:
			result = s.GetTitle();
			break;
		case 1:
			result = s.GetArtist();
			break;
		case 2:
			result = s.GetAlbum();
			break;
		case 3:
			result = s.GetYear();
			break;
		case 4:
			result = s.GetTrack();
			break;
		case 5:
			result = s.GetGenre();
			break;
		case 6:
			result = s.GetComposer();
			break;
		case 7:
			result = s.GetPerformer();
			break;
		case 8:
			result = s.GetDisc();
			break;
		case 9:
			result = s.GetComment();
			break;
		case 11:
			if (s.GetNewName().empty())
				result = s.GetName();
			else
				result = s.GetName() + " -> " + s.GetNewName();
			break;
		default:
			break;
	}
	return result.empty() ? Config.empty_tag : result;
}

void TagEditor::GetPatternList()
{
	if (Patterns.empty())
	{
		std::ifstream input(PatternsFile.c_str());
		if (input.is_open())
		{
			string line;
			while (getline(input, line))
			{
				if (!line.empty())
					Patterns.push_back(line);
			}
			input.close();
		}
	}
}

void TagEditor::SavePatternList()
{
	std::ofstream output(PatternsFile.c_str());
	if (output.is_open())
	{
		for (vector<string>::const_iterator it = Patterns.begin(); it != Patterns.end() && it != Patterns.begin()+30; ++it)
			output << *it << std::endl;
		output.close();
	}
}

Song::SetFunction TagEditor::IntoSetFunction(char c)
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

string TagEditor::GenerateFilename(const Song &s, string &pattern)
{
	string result = s.toString(pattern);
	EscapeUnallowedChars(result);
	return result;
}

string TagEditor::ParseFilename(Song &s, string mask, bool preview)
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
		for (vector<string>::const_iterator it = separators.begin(); it != separators.end(); ++it, ++i)
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
	
	for (vector< std::pair<char, string> >::iterator it = tags.begin(); it != tags.end(); ++it)
	{
		for (string::iterator j = it->second.begin(); j != it->second.end(); ++j)
			if (*j == '_')
				*j = ' ';
			
		if (!preview)
		{
			Song::SetFunction set = IntoSetFunction(it->first);
			if (set)
				(s.*set)(it->second);
		}
		else
			result << "%" << it->first << ": " << it->second << "\n";
	}
	return result.str();
}

void TagEditor::DealWithFilenames(SongList &v)
{
	int width = 30;
	int height = 6;
	
	GetPatternList();
	
	Menu<string> *Main = new Menu<string>((COLS-width)/2, (LINES-height)/2, width, height, "", Config.main_color, Config.window_border);
	Main->SetTimeout(ncmpcpp_window_timeout);
	Main->CyclicScrolling(Config.use_cyclic_scrolling);
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
		for (SongList::const_iterator it = v.begin(); it != v.end(); ++it)
			*Legend << Config.color2 << " * " << clEnd << (*it)->GetName() << "\n";
		Legend->Flush();
		
		Preview = Legend->EmptyClone();
		Preview->SetTitle("Preview");
		Preview->SetTimeout(ncmpcpp_window_timeout);
		
		Main = new Menu<string>((COLS-width)/2, (LINES-height)/2, one_width, height, "", Config.main_color, Config.active_window_border);
		Main->SetTimeout(ncmpcpp_window_timeout);
		Main->CyclicScrolling(Config.use_cyclic_scrolling);
		Main->SetItemDisplayer(Display::Generic);
		
		if (!Patterns.empty())
			Config.pattern = Patterns.front();
		Main->AddOption("Pattern: " + Config.pattern);
		Main->AddOption("Preview");
		Main->AddOption("Legend");
		Main->AddSeparator();
		Main->AddOption("Proceed");
		Main->AddOption("Cancel");
		if (!Patterns.empty())
		{
			Main->AddSeparator();
			Main->AddOption("Recent patterns", 1, 1);
			Main->AddSeparator();
			for (vector<string>::const_iterator it = Patterns.begin(); it != Patterns.end(); ++it)
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
						for (SongList::iterator it = v.begin(); it != v.end(); ++it)
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
										ShowMessage("File \"%s\" would have an empty name!", s.GetName().c_str());
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
							for (size_t i = 0; i < Patterns.size(); ++i)
							{
								if (Patterns[i] == Config.pattern)
								{
									Patterns.erase(Patterns.begin()+i);
									i--;
								}
							}
							Patterns.insert(Patterns.begin(), Config.pattern);
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

#endif

