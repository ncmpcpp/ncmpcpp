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

// taglib includes
#include "id3v2tag.h"
#include "textidentificationframe.h"
#include "mpegfile.h"
#include "vorbisfile.h"
#include "flacfile.h"

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "playlist.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myOldScreen;
using Global::myScreen;
using Global::wFooter;

TagEditor *myTagEditor = new TagEditor;

const std::string TagEditor::PatternsFile = config_dir + "patterns.list";
std::list<std::string> TagEditor::Patterns;

size_t TagEditor::LeftColumnWidth;
size_t TagEditor::MiddleColumnWidth;
size_t TagEditor::MiddleColumnStartX;
size_t TagEditor::RightColumnWidth;
size_t TagEditor::RightColumnStartX;

size_t TagEditor::FParserDialogWidth;
size_t TagEditor::FParserDialogHeight;
size_t TagEditor::FParserWidth;
size_t TagEditor::FParserWidthOne;
size_t TagEditor::FParserWidthTwo;
size_t TagEditor::FParserHeight;

void TagEditor::Init()
{
	SetDimensions();
	
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
	
	TagTypes = new Menu<std::string>(MiddleColumnStartX, MainStartY, MiddleColumnWidth, MainHeight, "Tag types", Config.main_color, brNone);
	TagTypes->HighlightColor(Config.main_highlight_color);
	TagTypes->SetTimeout(ncmpcpp_window_timeout);
	TagTypes->CyclicScrolling(Config.use_cyclic_scrolling);
	TagTypes->SetItemDisplayer(Display::Generic);
	
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
	
	Tags = new Menu<MPD::Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, "Tags", Config.main_color, brNone);
	Tags->HighlightColor(Config.main_highlight_color);
	Tags->SetTimeout(ncmpcpp_window_timeout);
	Tags->CyclicScrolling(Config.use_cyclic_scrolling);
	Tags->SetSelectPrefix(&Config.selected_item_prefix);
	Tags->SetSelectSuffix(&Config.selected_item_suffix);
	Tags->SetItemDisplayer(Display::Tags);
	Tags->SetItemDisplayerUserData(TagTypes);
	Tags->SetGetStringFunction(TagToString);
	Tags->SetGetStringFunctionUserData(TagTypes);
	
	FParserDialog = new Menu<std::string>((COLS-FParserDialogWidth)/2, (MainHeight-FParserDialogHeight)/2+MainStartY, FParserDialogWidth, FParserDialogHeight, "", Config.main_color, Config.window_border);
	FParserDialog->SetTimeout(ncmpcpp_window_timeout);
	FParserDialog->CyclicScrolling(Config.use_cyclic_scrolling);
	FParserDialog->SetItemDisplayer(Display::Generic);
	FParserDialog->AddOption("Get tags from filename");
	FParserDialog->AddOption("Rename files");
	FParserDialog->AddSeparator();
	FParserDialog->AddOption("Cancel");
	
	FParser = new Menu<std::string>((COLS-FParserWidth)/2, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthOne, FParserHeight, "_", Config.main_color, Config.active_window_border);
	FParser->SetTimeout(ncmpcpp_window_timeout);
	FParser->CyclicScrolling(Config.use_cyclic_scrolling);
	FParser->SetItemDisplayer(Display::Generic);
	
	FParserLegend = new Scrollpad((COLS-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthTwo, FParserHeight, "Legend", Config.main_color, Config.window_border);
	FParserLegend->SetTimeout(ncmpcpp_window_timeout);
	
	FParserPreview = new Scrollpad((COLS-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthTwo, FParserHeight, "Preview", Config.main_color, Config.window_border);
	FParserPreview->SetTimeout(ncmpcpp_window_timeout);
	
	w = LeftColumn;
	isInitialized = 1;
}

void TagEditor::SetDimensions()
{
	MiddleColumnWidth = std::min(26, COLS-2);
	LeftColumnWidth = (COLS-MiddleColumnWidth)/2;
	MiddleColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-MiddleColumnWidth-2;
	RightColumnStartX = LeftColumnWidth+MiddleColumnWidth+2;
	
	FParserDialogWidth = std::min(30, COLS);
	FParserDialogHeight = std::min(size_t(6), MainHeight);
	FParserWidth = COLS*0.9;
	FParserHeight = std::min(size_t(LINES*0.8), MainHeight);
	FParserWidthOne = FParserWidth/2;
	FParserWidthTwo = FParserWidth-FParserWidthOne;
}

void TagEditor::Resize()
{
	SetDimensions();
	
	Albums->Resize(LeftColumnWidth, MainHeight);
	Dirs->Resize(LeftColumnWidth, MainHeight);
	TagTypes->Resize(MiddleColumnWidth, MainHeight);
	Tags->Resize(RightColumnWidth, MainHeight);
	FParserDialog->Resize(FParserDialogWidth, FParserDialogHeight);
	FParser->Resize(FParserWidthOne, FParserHeight);
	FParserLegend->Resize(FParserWidthTwo, FParserHeight);
	FParserPreview->Resize(FParserWidthTwo, FParserHeight);
	
	Albums->MoveTo(0, MainStartY);
	Dirs->MoveTo(0, MainStartY);
	TagTypes->MoveTo(MiddleColumnStartX, MainStartY);
	Tags->MoveTo(RightColumnStartX, MainStartY);
	
	FParserDialog->MoveTo((COLS-FParserDialogWidth)/2, (MainHeight-FParserDialogHeight)/2+MainStartY);
	FParser->MoveTo((COLS-FParserWidth)/2, (MainHeight-FParserHeight)/2+MainStartY);
	FParserLegend->MoveTo((COLS-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY);
	FParserPreview->MoveTo((COLS-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY);
	
	if (MainHeight < 5 && (w == FParserDialog || w == FParser || w == FParserHelper)) // screen too low
		w = TagTypes; // fall back to main columns
	
	hasToBeResized = 0;
}

std::basic_string<my_char_t> TagEditor::Title()
{
	return U("Tag editor");
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
	Global::RedrawHeader = 1;
	Refresh();
}

void TagEditor::Refresh()
{
	LeftColumn->Display();
	mvvline(MainStartY, MiddleColumnStartX-1, 0, MainHeight);
	TagTypes->Display();
	mvvline(MainStartY, RightColumnStartX-1, 0, MainHeight);
	Tags->Display();
	
	if (w == FParser)
		FParserHelper->Display();
	else if (w == FParserHelper)
		FParser->Display();
}

void TagEditor::Update()
{
	if (LeftColumn->Empty())
	{
		LeftColumn->Window::Clear();
		Tags->Clear();
		MPD::TagList list;
		if (Config.albums_in_tag_editor)
		{
			*Albums << XY(0, 0) << "Fetching albums...";
			Albums->Window::Refresh();
			Mpd.GetAlbums("", list);
			for (MPD::TagList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				MPD::SongList l;
				Mpd.StartSearch(1);
				Mpd.AddSearch(MPD_TAG_ALBUM, *it);
				Mpd.CommitSearch(l);
				if (!l.empty())
				{
					l[0]->Localize();
					Albums->AddOption(std::make_pair(l[0]->toString(Config.tag_editor_album_format), *it));
				}
				MPD::FreeSongList(l);
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
				std::string parent = slash != std::string::npos ? itsBrowsedDir.substr(0, slash) : "/";
				Dirs->AddOption(make_pair("[..]", parent));
			}
			else
			{
				Dirs->AddOption(std::make_pair(".", "/"));
			}
			for (MPD::TagList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				size_t slash = it->rfind("/");
				std::string to_display = slash != std::string::npos ? it->substr(slash+1) : *it;
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
		MPD::SongList list;
		if (Config.albums_in_tag_editor)
		{
			Mpd.StartSearch(1);
			Mpd.AddSearch(MPD_TAG_ALBUM, Albums->Current().second);
			Mpd.CommitSearch(list);
			sort(list.begin(), list.end(), CaseInsensitiveSorting());
			for (MPD::SongList::iterator it = list.begin(); it != list.end(); ++it)
			{
				(*it)->Localize();
				Tags->AddOption(**it);
			}
		}
		else
		{
			Mpd.GetSongs(Dirs->Current().second, list);
			sort(list.begin(), list.end(), CaseInsensitiveSorting());
			for (MPD::SongList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				(*it)->Localize();
				Tags->AddOption(**it);
			}
		}
		MPD::FreeSongList(list);
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
		MPD::TagList test;
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
	}
	else if (w == FParserDialog)
	{
		size_t choice = FParserDialog->RealChoice();
		if (choice == 2) // cancel
		{
			w = TagTypes;
			Refresh();
			return;
		}
		GetPatternList();
		
		// prepare additional windows
		
		FParserLegend->Clear();
		*FParserLegend << "%a - artist\n";
		*FParserLegend << "%t - title\n";
		*FParserLegend << "%b - album\n";
		*FParserLegend << "%y - year\n";
		*FParserLegend << "%n - track number\n";
		*FParserLegend << "%g - genre\n";
		*FParserLegend << "%c - composer\n";
		*FParserLegend << "%p - performer\n";
		*FParserLegend << "%d - disc\n";
		*FParserLegend << "%C - comment\n\n";
		*FParserLegend << fmtBold << "Files:\n" << fmtBoldEnd;
		for (MPD::SongList::const_iterator it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			*FParserLegend << Config.color2 << " * " << clEnd << (*it)->GetName() << "\n";
		FParserLegend->Flush();
		
		if (!Patterns.empty())
			Config.pattern = Patterns.front();
		FParser->Clear();
		FParser->Reset();
		FParser->AddOption("Pattern: " + Config.pattern);
		FParser->AddOption("Preview");
		FParser->AddOption("Legend");
		FParser->AddSeparator();
		FParser->AddOption("Proceed");
		FParser->AddOption("Cancel");
		if (!Patterns.empty())
		{
			FParser->AddSeparator();
			FParser->AddOption("Recent patterns", 1, 1);
			FParser->AddSeparator();
			for (std::list<std::string>::const_iterator it = Patterns.begin(); it != Patterns.end(); ++it)
				FParser->AddOption(*it);
		}
		
		FParser->SetTitle(choice == 0 ? "Get tags from filename" : "Rename files");
		w = FParser;
		FParserUsePreview = 1;
		FParserHelper = FParserLegend;
		FParserHelper->Display();
	}
	else if (w == FParser)
	{
		bool quit = 0;
		size_t pos = FParser->RealChoice();
		
		if (pos == 3) // save
			FParserUsePreview = 0;
		
		if (pos == 0) // change pattern
		{
			LockStatusbar();
			Statusbar() << "Pattern: ";
			std::string new_pattern = wFooter->GetString(Config.pattern);
			UnlockStatusbar();
			if (!new_pattern.empty())
			{
				Config.pattern = new_pattern;
				FParser->at(0) = "Pattern: ";
				FParser->at(0) += Config.pattern;
			}
		}
		else if (pos == 1 || pos == 3) // preview or proceed
		{
			bool success = 1;
			ShowMessage("Parsing...");
			FParserPreview->Clear();
			for (MPD::SongList::iterator it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			{
				MPD::Song &s = **it;
				if (FParserDialog->Choice() == 0) // get tags from filename
				{
					if (FParserUsePreview)
					{
						*FParserPreview << fmtBold << s.GetName() << ":\n" << fmtBoldEnd;
						*FParserPreview << ParseFilename(s, Config.pattern, FParserUsePreview) << "\n";
					}
					else
						ParseFilename(s, Config.pattern, FParserUsePreview);
				}
				else // rename files
				{
					std::string file = s.GetName();
					size_t last_dot = file.rfind(".");
					std::string extension = file.substr(last_dot);
					std::string new_file = GenerateFilename(s, "{" + Config.pattern + "}");
					if (new_file.empty() && !FParserUsePreview)
					{
						ShowMessage("File \"%s\" would have an empty name!", s.GetName().c_str());
						FParserUsePreview = 1;
						success = 0;
					}
					if (!FParserUsePreview)
						s.SetNewName(new_file + extension);
					*FParserPreview << file << Config.color2 << " -> " << clEnd;
					if (new_file.empty())
						*FParserPreview << Config.empty_tags_color << Config.empty_tag << clEnd;
					else
						*FParserPreview << new_file << extension;
					*FParserPreview << "\n\n";
					if (!success)
						break;
				}
			}
			if (FParserUsePreview)
			{
				FParserHelper = FParserPreview;
				FParserHelper->Flush();
				FParserHelper->Display();
			}
			else if (success)
			{
				Patterns.remove(Config.pattern);
				Patterns.insert(Patterns.begin(), Config.pattern);
				quit = 1;
			}
			if (pos != 3 || success)
				ShowMessage("Operation finished!");
		}
		else if (pos == 2) // show legend
		{
			FParserHelper = FParserLegend;
			FParserHelper->Display();
		}
		else if (pos == 4) // cancel
		{
			quit = 1;
		}
		else // list of patterns
		{
			Config.pattern = FParser->Current();
			FParser->at(0) = "Pattern: " + Config.pattern;
		}
		
		if (quit)
		{
			SavePatternList();
			w = TagTypes;
			Refresh();
			return;
		}
	}
	
	if ((w != TagTypes && w != Tags) || Tags->Empty()) // after this point we start dealing with tags
		return;
	
	EditedSongs.clear();
	if (Tags->hasSelected()) // if there are selected songs, perform operations only on them
	{
		std::vector<size_t> selected;
		Tags->GetSelected(selected);
		for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
			EditedSongs.push_back(&(*Tags)[*it]);
	}
	else
		for (size_t i = 0; i < Tags->Size(); ++i)
			EditedSongs.push_back(&(*Tags)[i]);
	
	MPD::Song::GetFunction get = 0;
	MPD::Song::SetFunction set = 0;
	
	size_t id = TagTypes->RealChoice();
	switch (id)
	{
		case 0:
			get = &MPD::Song::GetTitle;
			set = &MPD::Song::SetTitle;
			break;
		case 1:
			get = &MPD::Song::GetArtist;
			set = &MPD::Song::SetArtist;
			break;
		case 2:
			get = &MPD::Song::GetAlbum;
			set = &MPD::Song::SetAlbum;
			break;
		case 3:
			get = &MPD::Song::GetDate;
			set = &MPD::Song::SetDate;
			break;
		case 4:
			get = &MPD::Song::GetTrack;
			set = &MPD::Song::SetTrack;
			if (w == TagTypes)
			{
				LockStatusbar();
				Statusbar() << "Number tracks? [" << fmtBold << 'y' << fmtBoldEnd << '/' << fmtBold << 'n' << fmtBoldEnd << "] ";
				wFooter->Refresh();
				int in = 0;
				do
				{
					TraceMpdStatus();
					wFooter->ReadKey(in);
				}
				while (in != 'y' && in != 'n');
				UnlockStatusbar();
				if (in == 'y')
				{
					MPD::SongList::iterator it = EditedSongs.begin();
					for (unsigned i = 1; i <= EditedSongs.size(); ++i, ++it)
						(*it)->SetTrack(i);
					ShowMessage("Tracks numbered!");
				}
				else
					ShowMessage("Aborted!");
			}
			break;
		case 5:
			get = &MPD::Song::GetGenre;
			set = &MPD::Song::SetGenre;
			break;
		case 6:
			get = &MPD::Song::GetComposer;
			set = &MPD::Song::SetComposer;
			break;
		case 7:
			get = &MPD::Song::GetPerformer;
			set = &MPD::Song::SetPerformer;
			break;
		case 8:
			get = &MPD::Song::GetDisc;
			set = &MPD::Song::SetDisc;
			break;
		case 9:
			get = &MPD::Song::GetComment;
			set = &MPD::Song::SetComment;
			break;
		case 10:
		{
			if (w == TagTypes)
			{
				if (size_t(COLS) < FParserDialogWidth || MainHeight < FParserDialogHeight)
				{
					ShowMessage("Screen is too small to display additional windows!");
					return;
				}
				FParserDialog->Reset();
				w = FParserDialog;
			}
			else if (w == Tags)
			{
				MPD::Song &s = Tags->Current();
				std::string old_name = s.GetNewName().empty() ? s.GetName() : s.GetNewName();
				size_t last_dot = old_name.rfind(".");
				std::string extension = old_name.substr(last_dot);
				old_name = old_name.substr(0, last_dot);
				LockStatusbar();
				Statusbar() << fmtBold << "New filename: " << fmtBoldEnd;
				std::string new_name = wFooter->GetString(old_name);
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
			for (MPD::SongList::iterator it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
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
			for (MPD::SongList::iterator it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				CapitalizeFirstLetters(**it);
			ShowMessage("Done!");
			break;
		}
		case 14: // lower all letters
		{
			ShowMessage("Processing...");
			for (MPD::SongList::iterator it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				LowerAllLetters(**it);
			ShowMessage("Done!");
			break;
		}
		default:
			break;
	}
	
	if (w == TagTypes && id != 0 && id != 4 && set)
	{
		LockStatusbar();
		Statusbar() << fmtBold << TagTypes->Current() << fmtBoldEnd << ": ";
		std::string new_tag = wFooter->GetString((Tags->Current().*get)());
		UnlockStatusbar();
		for (MPD::SongList::iterator it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			(**it.*set)(new_tag);
	}
	else if (w == Tags && set)
	{
		LockStatusbar();
		Statusbar() << fmtBold << TagTypes->Current() << fmtBoldEnd << ": ";
		std::string new_tag = wFooter->GetString((Tags->Current().*get)());
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
		Tags->Select(Tags->Choice(), !Tags->isSelected());
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
	if (w == FParserDialog)
	{
		if (FParserDialog->hasCoords(me.x, me.y))
		{
			if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
			{
				FParserDialog->Goto(me.y);
				if (me.bstate & BUTTON3_PRESSED)
					EnterPressed();
			}
			else
				Screen<Window>::MouseButtonPressed(me);
		}
	}
	else if (w == FParser || w == FParserHelper)
	{
		if (FParser->hasCoords(me.x, me.y))
		{
			if (w != FParser)
				PrevColumn();
			if (size_t(me.y) < FParser->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
			{
				FParser->Goto(me.y);
				if (me.bstate & BUTTON3_PRESSED)
					EnterPressed();
			}
			else
				Screen<Window>::MouseButtonPressed(me);
		}
		else if (FParserHelper->hasCoords(me.x, me.y))
		{
			if (w != FParserHelper)
				NextColumn();
			reinterpret_cast<Screen<Scrollpad> *>(this)->Screen<Scrollpad>::MouseButtonPressed(me);
		}
	}
	else if (!LeftColumn->Empty() && LeftColumn->hasCoords(me.x, me.y))
	{
		if (w != LeftColumn)
		{
			PrevColumn();
			PrevColumn();
		}
		if (size_t(me.y) < LeftColumn->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
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
		if (size_t(me.y) < TagTypes->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
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
		if (size_t(me.y) < Tags->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
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
		v.push_back(new MPD::Song(Tags->at(*it)));
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
		w = Tags;
		Tags->HighlightColor(Config.active_column_color);
	}
	else if (w == FParser)
	{
		FParser->SetBorder(Config.window_border);
		FParser->Display();
		w = FParserHelper;
		FParserHelper->SetBorder(Config.active_window_border);
		FParserHelper->Display();
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
	else if (w == FParserHelper)
	{
		FParserHelper->SetBorder(Config.window_border);
		FParserHelper->Display();
		w = FParser;
		FParser->SetBorder(Config.active_window_border);
		FParser->Display();
	}
}

void TagEditor::ReadTags(MPD::Song &s)
{
	TagLib::FileRef f(s.GetFile().c_str());
	if (f.isNull())
		return;
	
	TagLib::MPEG::File *mpegf = dynamic_cast<TagLib::MPEG::File *>(f.file());

	s.SetArtist(f.tag()->artist().to8Bit(1));
	s.SetTitle(f.tag()->title().to8Bit(1));
	s.SetAlbum(f.tag()->album().to8Bit(1));
	s.SetTrack(IntoStr(f.tag()->track()));
	s.SetDate(IntoStr(f.tag()->year()));
	s.SetGenre(f.tag()->genre().to8Bit(1));
	if (mpegf)
	{
		s.SetComposer(!mpegf->ID3v2Tag()->frameListMap()["TCOM"].isEmpty() ? mpegf->ID3v2Tag()->frameListMap()["TCOM"].front()->toString().to8Bit(1) : "");
		s.SetPerformer(!mpegf->ID3v2Tag()->frameListMap()["TOPE"].isEmpty() ? mpegf->ID3v2Tag()->frameListMap()["TOPE"].front()->toString().to8Bit(1) : "");
		s.SetDisc(!mpegf->ID3v2Tag()->frameListMap()["TPOS"].isEmpty() ? mpegf->ID3v2Tag()->frameListMap()["TPOS"].front()->toString().to8Bit(1) : "");
	}
	s.SetComment(f.tag()->comment().to8Bit(1));
}

namespace
{
	template <typename T> void WriteID3v2(const TagLib::ByteVector &type, TagLib::ID3v2::Tag *tag, const T &list)
	{
		using TagLib::ID3v2::TextIdentificationFrame;
		tag->removeFrames(type);
		TextIdentificationFrame *frame = new TextIdentificationFrame(type, TagLib::String::UTF8);
		frame->setText(list);
		tag->addFrame(frame);
	}
}

void TagEditor::WriteXiphComments(const MPD::Song &s, TagLib::Ogg::XiphComment *tag)
{
	TagLib::StringList list;
	
	tag->addField("DISCNUMBER", ToWString(s.GetDisc())); // disc
	
	tag->removeField("COMPOSER"); // composer
	GetTagList(list, s.GetComposer());
	for (TagLib::StringList::ConstIterator it = list.begin(); it != list.end(); ++it)
		tag->addField("COMPOSER", *it, 0);
	
	tag->removeField("PERFORMER"); // performer
	GetTagList(list, s.GetPerformer());
	for (TagLib::StringList::ConstIterator it = list.begin(); it != list.end(); ++it)
		tag->addField("PERFORMER", *it, 0);
}

bool TagEditor::WriteTags(MPD::Song &s)
{
	std::string path_to_file;
	bool file_is_from_db = s.isFromDB();
	if (file_is_from_db)
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	locale_to_utf(path_to_file);
	TagLib::FileRef f(path_to_file.c_str());
	if (!f.isNull())
	{
		f.tag()->setTitle(ToWString(s.GetTitle()));
		f.tag()->setArtist(ToWString(s.GetArtist()));
		f.tag()->setAlbum(ToWString(s.GetAlbum()));
		f.tag()->setYear(StrToInt(s.GetDate()));
		f.tag()->setTrack(StrToInt(s.GetTrack()));
		f.tag()->setGenre(ToWString(s.GetGenre()));
		f.tag()->setComment(ToWString(s.GetComment()));
		if (TagLib::MPEG::File *mp3_file = dynamic_cast<TagLib::MPEG::File *>(f.file()))
		{
			TagLib::ID3v2::Tag *tag = mp3_file->ID3v2Tag(1);
			TagLib::StringList list;
			
			WriteID3v2("TIT2", tag, ToWString(s.GetTitle()));  // title
			WriteID3v2("TPE1", tag, ToWString(s.GetArtist())); // artist
			WriteID3v2("TALB", tag, ToWString(s.GetAlbum()));  // album
			WriteID3v2("TDRC", tag, ToWString(s.GetDate()));   // date
			WriteID3v2("TRCK", tag, ToWString(s.GetTrack()));  // track
			WriteID3v2("TCON", tag, ToWString(s.GetGenre()));  // genre
			WriteID3v2("TPOS", tag, ToWString(s.GetDisc()));   // disc
			
			GetTagList(list, s.GetComposer());
			WriteID3v2("TCOM", tag, list); // composer
			
			GetTagList(list, s.GetPerformer());
			// in >=mpd-0.16 treating TOPE frame as performer tag
			// was dropped in favor of TPE3/TPE4 frames, so we have
			// to write frame accurate to used mpd version
			WriteID3v2(Mpd.Version() < 16 ? "TOPE" : "TPE3", tag, list); // performer
		}
		else if (TagLib::Ogg::Vorbis::File *ogg_file = dynamic_cast<TagLib::Ogg::Vorbis::File *>(f.file()))
		{
			WriteXiphComments(s, ogg_file->tag());
		}
		else if (TagLib::FLAC::File *flac_file = dynamic_cast<TagLib::FLAC::File *>(f.file()))
		{
			WriteXiphComments(s, flac_file->xiphComment(1));
		}
		if (!f.save())
			return false;
		
		if (!s.GetNewName().empty())
		{
			std::string new_name;
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
					size_t pos = myPlaylist->Items->Choice();
					Mpd.StartCommandsList();
					Mpd.Delete(pos);
					int id = Mpd.AddSong("file://" + new_name);
					if (id >= 0)
					{
						s = myPlaylist->Items->Back();
						Mpd.Move(s.GetPosition(), pos);
					}
					Mpd.CommitCommandsList();
				}
				else // only myBrowser->Main()
					myBrowser->GetDirectory(myBrowser->CurrentDir());
			}
		}
		return true;
	}
	else
		return false;
}

std::string TagEditor::CapitalizeFirstLetters(const std::string &s)
{
	if (s.empty())
		return "";
	std::string result = s;
	if (isalpha(result[0]))
		result[0] = toupper(result[0]);
	for (std::string::iterator it = result.begin()+1; it != result.end(); ++it)
	{
		if (isalpha(*it) && !isalpha(*(it-1)) && *(it-1) != '\'')
			*it = toupper(*it);
	}
	return result;
}

void TagEditor::CapitalizeFirstLetters(MPD::Song &s)
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

void TagEditor::LowerAllLetters(MPD::Song &s)
{
	std::string conv = s.GetTitle();
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
	for (size_t i = 0; i != std::string::npos; i = s.find(",", i))
	{
		if (i)
			++i;
		while (s[i] == ' ')
			++i;
		size_t j = s.find(",", i);
		list.append(ToWString(s.substr(i, j-i)));
	}
}

std::string TagEditor::TagToString(const MPD::Song &s, void *data)
{
	std::string result;
	switch (static_cast<Menu<std::string> *>(data)->Choice())
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
			result = s.GetDate();
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
			result = s.GetNewName().empty() ? s.GetName() : s.GetName() + " -> " + s.GetNewName();
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
			std::string line;
			while (getline(input, line))
				if (!line.empty())
					Patterns.push_back(line);
			input.close();
		}
	}
}

void TagEditor::SavePatternList()
{
	std::ofstream output(PatternsFile.c_str());
	if (output.is_open())
	{
		std::list<std::string>::const_iterator it = Patterns.begin();
		for (unsigned i = 30; it != Patterns.end() && i; ++it, --i)
			output << *it << std::endl;
		output.close();
	}
}

MPD::Song::SetFunction TagEditor::IntoSetFunction(char c)
{
	switch (c)
	{
		case 'a':
			return &MPD::Song::SetArtist;
		case 't':
			return &MPD::Song::SetTitle;
		case 'b':
			return &MPD::Song::SetAlbum;
		case 'y':
			return &MPD::Song::SetDate;
		case 'n':
			return &MPD::Song::SetTrack;
		case 'g':
			return &MPD::Song::SetGenre;
		case 'c':
			return &MPD::Song::SetComposer;
		case 'p':
			return &MPD::Song::SetPerformer;
		case 'd':
			return &MPD::Song::SetDisc;
		case 'C':
			return &MPD::Song::SetComment;
		default:
			return 0;
	}
}

std::string TagEditor::GenerateFilename(const MPD::Song &s, const std::string &pattern)
{
	std::string result = s.toString(pattern);
	EscapeUnallowedChars(result);
	return result;
}

std::string TagEditor::ParseFilename(MPD::Song &s, std::string mask, bool preview)
{
	std::ostringstream result;
	std::vector<std::string> separators;
	std::vector< std::pair<char, std::string> > tags;
	std::string file = s.GetName().substr(0, s.GetName().rfind("."));
	
	for (size_t i = mask.find("%"); i != std::string::npos; i = mask.find("%"))
	{
		tags.push_back(std::make_pair(mask.at(i+1), ""));
		mask = mask.substr(i+2);
		i = mask.find("%");
		if (!mask.empty())
			separators.push_back(mask.substr(0, i));
	}
	size_t i = 0;
	for (std::vector<std::string>::const_iterator it = separators.begin(); it != separators.end(); ++it, ++i)
	{
		size_t j = file.find(*it);
		tags.at(i).second = file.substr(0, j);
		if (j+it->length() > file.length())
			goto PARSE_FAILED;
		file = file.substr(j+it->length());
	}
	if (!file.empty())
	{
		if (i >= tags.size())
			goto PARSE_FAILED;
		tags.at(i).second = file;
	}
	
	if (0) // tss...
	{
		PARSE_FAILED:
		return "Error while parsing filename!\n";
	}
	
	for (std::vector< std::pair<char, std::string> >::iterator it = tags.begin(); it != tags.end(); ++it)
	{
		for (std::string::iterator j = it->second.begin(); j != it->second.end(); ++j)
			if (*j == '_')
				*j = ' ';
		
		if (!preview)
		{
			MPD::Song::SetFunction set = IntoSetFunction(it->first);
			if (set)
				(s.*set)(it->second);
		}
		else
			result << "%" << it->first << ": " << it->second << "\n";
	}
	return result.str();
}

#endif

