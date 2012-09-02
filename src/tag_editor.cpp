/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
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
#include <sstream>
#include <stdexcept>

// taglib includes
#include "id3v2tag.h"
#include "textidentificationframe.h"
#include "mpegfile.h"
#include "vorbisfile.h"
#include "flacfile.h"
#include "xiphcomment.h"
#include "fileref.h"
#include "tag.h"

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "song_info.h"
#include "playlist.h"
#include "utility/comparators.h"

using namespace std::placeholders;

using Global::myScreen;
using Global::MainHeight;
using Global::MainStartY;

TagEditor *myTagEditor = new TagEditor;

namespace {//

size_t LeftColumnWidth;
size_t LeftColumnStartX;
size_t MiddleColumnWidth;
size_t MiddleColumnStartX;
size_t RightColumnWidth;
size_t RightColumnStartX;

size_t FParserDialogWidth;
size_t FParserDialogHeight;
size_t FParserWidth;
size_t FParserWidthOne;
size_t FParserWidthTwo;
size_t FParserHeight;

std::list<std::string> Patterns;
std::string PatternsFile = "patterns.list";

std::string CapitalizeFirstLetters(const std::string &s);
void CapitalizeFirstLetters(MPD::MutableSong &s);
void LowerAllLetters(MPD::MutableSong &s);
void GetTagList(TagLib::StringList &list, const MPD::MutableSong &s, MPD::Song::GetFunction f);

template <typename T>
void WriteID3v2(const TagLib::ByteVector &type, TagLib::ID3v2::Tag *tag, const T &list);
void WriteXiphComments(const MPD::MutableSong &s, TagLib::Ogg::XiphComment *tag);

void GetPatternList();
void SavePatternList();

MPD::MutableSong::SetFunction IntoSetFunction(char c);
std::string GenerateFilename(const MPD::MutableSong &s, const std::string &pattern);
std::string ParseFilename(MPD::MutableSong &s, std::string mask, bool preview);

std::string SongToString(const MPD::MutableSong &s);
bool DirEntryMatcher(const Regex &rx, const std::pair<std::string, std::string> &dir, bool filter);
bool AlbumEntryMatcher(const Regex &rx, const std::pair<std::string, std::string> &dir);
bool SongEntryMatcher(const Regex &rx, const MPD::MutableSong &s);

}

void TagEditor::Init()
{
	PatternsFile = Config.ncmpcpp_directory + "patterns.list";
	SetDimensions(0, COLS);
	
	Albums = new NC::Menu< std::pair<std::string, std::string> >(0, MainStartY, LeftColumnWidth, MainHeight, Config.titles_visibility ? "Albums" : "", Config.main_color, NC::brNone);
	Albums->HighlightColor(Config.active_column_color);
	Albums->CyclicScrolling(Config.use_cyclic_scrolling);
	Albums->CenteredCursor(Config.centered_cursor);
	Albums->setItemDisplayer(Display::Pair<std::string, std::string>);
	
	Dirs = new NC::Menu< std::pair<std::string, std::string> >(0, MainStartY, LeftColumnWidth, MainHeight, Config.titles_visibility ? "Directories" : "", Config.main_color, NC::brNone);
	Dirs->HighlightColor(Config.active_column_color);
	Dirs->CyclicScrolling(Config.use_cyclic_scrolling);
	Dirs->CenteredCursor(Config.centered_cursor);
	Dirs->setItemDisplayer(Display::Pair<std::string, std::string>);
	
	LeftColumn = Config.albums_in_tag_editor ? Albums : Dirs;
	
	TagTypes = new NC::Menu<std::string>(MiddleColumnStartX, MainStartY, MiddleColumnWidth, MainHeight, Config.titles_visibility ? "Tag types" : "", Config.main_color, NC::brNone);
	TagTypes->HighlightColor(Config.main_highlight_color);
	TagTypes->CyclicScrolling(Config.use_cyclic_scrolling);
	TagTypes->CenteredCursor(Config.centered_cursor);
	TagTypes->setItemDisplayer(Display::Default<std::string>);
	
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m)
		TagTypes->AddItem(m->Name);
	TagTypes->AddSeparator();
	TagTypes->AddItem("Filename");
	TagTypes->AddSeparator();
	if (Config.titles_visibility)
		TagTypes->AddItem("Options", 1, 1);
	TagTypes->AddSeparator();
	TagTypes->AddItem("Capitalize First Letters");
	TagTypes->AddItem("lower all letters");
	TagTypes->AddSeparator();
	TagTypes->AddItem("Reset");
	TagTypes->AddItem("Save");
	
	Tags = new NC::Menu<MPD::MutableSong>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Tags" : "", Config.main_color, NC::brNone);
	Tags->HighlightColor(Config.main_highlight_color);
	Tags->CyclicScrolling(Config.use_cyclic_scrolling);
	Tags->CenteredCursor(Config.centered_cursor);
	Tags->SetSelectPrefix(Config.selected_item_prefix);
	Tags->SetSelectSuffix(Config.selected_item_suffix);
	Tags->setItemDisplayer(Display::Tags);
	
	FParserDialog = new NC::Menu<std::string>((COLS-FParserDialogWidth)/2, (MainHeight-FParserDialogHeight)/2+MainStartY, FParserDialogWidth, FParserDialogHeight, "", Config.main_color, Config.window_border);
	FParserDialog->CyclicScrolling(Config.use_cyclic_scrolling);
	FParserDialog->CenteredCursor(Config.centered_cursor);
	FParserDialog->setItemDisplayer(Display::Default<std::string>);
	FParserDialog->AddItem("Get tags from filename");
	FParserDialog->AddItem("Rename files");
	FParserDialog->AddSeparator();
	FParserDialog->AddItem("Cancel");
	
	FParser = new NC::Menu<std::string>((COLS-FParserWidth)/2, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthOne, FParserHeight, "_", Config.main_color, Config.active_window_border);
	FParser->CyclicScrolling(Config.use_cyclic_scrolling);
	FParser->CenteredCursor(Config.centered_cursor);
	FParser->setItemDisplayer(Display::Default<std::string>);
	
	FParserLegend = new NC::Scrollpad((COLS-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthTwo, FParserHeight, "Legend", Config.main_color, Config.window_border);
	
	FParserPreview = new NC::Scrollpad((COLS-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthTwo, FParserHeight, "Preview", Config.main_color, Config.window_border);
	
	w = LeftColumn;
	isInitialized = 1;
}

void TagEditor::SetDimensions(size_t x_offset, size_t width)
{
	MiddleColumnWidth = std::min(26, COLS-2);
	LeftColumnStartX = x_offset;
	LeftColumnWidth = (width-MiddleColumnWidth)/2;
	MiddleColumnStartX = LeftColumnStartX+LeftColumnWidth+1;
	RightColumnWidth = width-LeftColumnWidth-MiddleColumnWidth-2;
	RightColumnStartX = MiddleColumnStartX+MiddleColumnWidth+1;
	
	FParserDialogWidth = std::min(30, COLS);
	FParserDialogHeight = std::min(size_t(6), MainHeight);
	FParserWidth = width*0.9;
	FParserHeight = std::min(size_t(LINES*0.8), MainHeight);
	FParserWidthOne = FParserWidth/2;
	FParserWidthTwo = FParserWidth-FParserWidthOne;
}

void TagEditor::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	SetDimensions(x_offset, width);
	
	Albums->Resize(LeftColumnWidth, MainHeight);
	Dirs->Resize(LeftColumnWidth, MainHeight);
	TagTypes->Resize(MiddleColumnWidth, MainHeight);
	Tags->Resize(RightColumnWidth, MainHeight);
	FParserDialog->Resize(FParserDialogWidth, FParserDialogHeight);
	FParser->Resize(FParserWidthOne, FParserHeight);
	FParserLegend->Resize(FParserWidthTwo, FParserHeight);
	FParserPreview->Resize(FParserWidthTwo, FParserHeight);
	
	Albums->MoveTo(LeftColumnStartX, MainStartY);
	Dirs->MoveTo(LeftColumnStartX, MainStartY);
	TagTypes->MoveTo(MiddleColumnStartX, MainStartY);
	Tags->MoveTo(RightColumnStartX, MainStartY);
	
	FParserDialog->MoveTo(x_offset+(width-FParserDialogWidth)/2, (MainHeight-FParserDialogHeight)/2+MainStartY);
	FParser->MoveTo(x_offset+(width-FParserWidth)/2, (MainHeight-FParserHeight)/2+MainStartY);
	FParserLegend->MoveTo(x_offset+(width-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY);
	FParserPreview->MoveTo(x_offset+(width-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY);
	
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
	using Global::myLockedScreen;
	
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	Global::RedrawHeader = true;
	Refresh();
}

void TagEditor::Refresh()
{
	LeftColumn->Display();
	mvvline(MainStartY, MiddleColumnStartX-1, 0, MainHeight);
	TagTypes->Display();
	mvvline(MainStartY, RightColumnStartX-1, 0, MainHeight);
	Tags->Display();
	
	if (w == FParserDialog)
	{
		FParserDialog->Display();
	}
	else if (w == FParser || w == FParserHelper)
	{
		FParser->Display();
		FParserHelper->Display();
	}
}

void TagEditor::Update()
{
	if (LeftColumn->ReallyEmpty())
	{
		LeftColumn->Window::Clear();
		Tags->Clear();
		if (Config.albums_in_tag_editor)
		{
			*Albums << NC::XY(0, 0) << "Fetching albums...";
			Albums->Window::Refresh();
			Mpd.BlockIdle(true); // for the same reason as in media library
			auto albums = Mpd.GetList(MPD_TAG_ALBUM);
			for (auto album = albums.begin(); album != albums.end(); ++album)
			{
				Mpd.StartSearch(1);
				Mpd.AddSearch(MPD_TAG_ALBUM, *album);
				auto songs = Mpd.CommitSearchSongs();
				if (!songs.empty())
					Albums->AddItem(std::make_pair(songs[0].toString(Config.tag_editor_album_format), *album));
			}
			Mpd.BlockIdle(false);
			std::sort(Albums->BeginV(), Albums->EndV(), CaseInsensitiveSorting());
		}
		else
		{
			int highlightme = -1;
			auto dirs = Mpd.GetDirectories(itsBrowsedDir);
			std::sort(dirs.begin(), dirs.end(), CaseInsensitiveSorting());
			if (itsBrowsedDir != "/")
			{
				size_t slash = itsBrowsedDir.rfind("/");
				std::string parent = slash != std::string::npos ? itsBrowsedDir.substr(0, slash) : "/";
				Dirs->AddItem(make_pair("..", parent));
			}
			else
				Dirs->AddItem(std::make_pair(".", "/"));
			for (auto dir = dirs.begin(); dir != dirs.end(); ++dir)
			{
				size_t slash = dir->rfind("/");
				std::string to_display = slash != std::string::npos ? dir->substr(slash+1) : *dir;
				Dirs->AddItem(make_pair(to_display, *dir));
				if (*dir == itsHighlightedDir)
					highlightme = Dirs->Size()-1;
			}
			if (highlightme != -1)
				Dirs->Highlight(highlightme);
		}
		LeftColumn->Display();
		TagTypes->Refresh();
	}
	
	if (Tags->ReallyEmpty())
	{
		Tags->Reset();
		if (Config.albums_in_tag_editor)
		{
			if (!Albums->Empty())
			{
				Mpd.StartSearch(1);
				Mpd.AddSearch(MPD_TAG_ALBUM, Albums->Current().value().second);
				auto albums = Mpd.CommitSearchSongs();
				std::sort(albums.begin(), albums.end(), CaseInsensitiveSorting());
				for (auto album = albums.begin(); album != albums.end(); ++album)
					Tags->AddItem(*album);
			}
		}
		else
		{
			auto songs = Mpd.GetSongs(Dirs->Current().value().second);
			std::sort(songs.begin(), songs.end(), CaseInsensitiveSorting());
			for (auto s = songs.begin(); s != songs.end(); ++s)
				Tags->AddItem(*s);
		}
		Tags->Refresh();
	}
	
	if (w == TagTypes && TagTypes->Choice() < 13)
	{
		Tags->Refresh();
	}
	else if (TagTypes->Choice() >= 13)
	{
		Tags->Window::Clear();
		Tags->Window::Refresh();
	}
}

void TagEditor::EnterPressed()
{
	using Global::wFooter;
	
	if (w == Dirs)
	{
		auto dirs = Mpd.GetDirectories(LeftColumn->Current().value().second);
		if (!dirs.empty())
		{
			itsHighlightedDir = itsBrowsedDir;
			itsBrowsedDir = LeftColumn->Current().value().second;
			LeftColumn->Clear();
			LeftColumn->Reset();
		}
		else
			ShowMessage("No subdirs found");
	}
	else if (w == FParserDialog)
	{
		size_t choice = FParserDialog->Choice();
		if (choice == 3) // cancel
		{
			w = TagTypes;
			Refresh();
			return;
		}
		GetPatternList();
		
		// prepare additional windows
		
		FParserLegend->Clear();
		*FParserLegend << U("%a - artist\n");
		*FParserLegend << U("%A - album artist\n");
		*FParserLegend << U("%t - title\n");
		*FParserLegend << U("%b - album\n");
		*FParserLegend << U("%y - date\n");
		*FParserLegend << U("%n - track number\n");
		*FParserLegend << U("%g - genre\n");
		*FParserLegend << U("%c - composer\n");
		*FParserLegend << U("%p - performer\n");
		*FParserLegend << U("%d - disc\n");
		*FParserLegend << U("%C - comment\n\n");
		*FParserLegend << NC::fmtBold << U("Files:\n") << NC::fmtBoldEnd;
		for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			*FParserLegend << Config.color2 << U(" * ") << NC::clEnd << (*it)->getName() << '\n';
		FParserLegend->Flush();
		
		if (!Patterns.empty())
			Config.pattern = Patterns.front();
		FParser->Clear();
		FParser->Reset();
		FParser->AddItem("Pattern: " + Config.pattern);
		FParser->AddItem("Preview");
		FParser->AddItem("Legend");
		FParser->AddSeparator();
		FParser->AddItem("Proceed");
		FParser->AddItem("Cancel");
		if (!Patterns.empty())
		{
			FParser->AddSeparator();
			FParser->AddItem("Recent patterns", 1, 1);
			FParser->AddSeparator();
			for (std::list<std::string>::const_iterator it = Patterns.begin(); it != Patterns.end(); ++it)
				FParser->AddItem(*it);
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
		size_t pos = FParser->Choice();
		
		if (pos == 4) // save
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
				FParser->at(0).value() = "Pattern: ";
				FParser->at(0).value() += Config.pattern;
			}
		}
		else if (pos == 1 || pos == 4) // preview or proceed
		{
			bool success = 1;
			ShowMessage("Parsing...");
			FParserPreview->Clear();
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			{
				MPD::MutableSong &s = **it;
				if (FParserDialog->Choice() == 0) // get tags from filename
				{
					if (FParserUsePreview)
					{
						*FParserPreview << NC::fmtBold << s.getName() << U(":\n") << NC::fmtBoldEnd;
						*FParserPreview << ParseFilename(s, Config.pattern, FParserUsePreview) << '\n';
					}
					else
						ParseFilename(s, Config.pattern, FParserUsePreview);
				}
				else // rename files
				{
					std::string file = s.getName();
					size_t last_dot = file.rfind(".");
					std::string extension = file.substr(last_dot);
					std::string new_file = GenerateFilename(s, "{" + Config.pattern + "}");
					if (new_file.empty() && !FParserUsePreview)
					{
						ShowMessage("File \"%s\" would have an empty name", s.getName().c_str());
						FParserUsePreview = 1;
						success = 0;
					}
					if (!FParserUsePreview)
						s.setNewURI(new_file + extension);
					*FParserPreview << file << Config.color2 << U(" -> ") << NC::clEnd;
					if (new_file.empty())
						*FParserPreview << Config.empty_tags_color << Config.empty_tag << NC::clEnd;
					else
						*FParserPreview << new_file << extension;
					*FParserPreview << '\n' << '\n';
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
			if (pos != 4 || success)
				ShowMessage("Operation finished");
		}
		else if (pos == 2) // show legend
		{
			FParserHelper = FParserLegend;
			FParserHelper->Display();
		}
		else if (pos == 5) // cancel
		{
			quit = 1;
		}
		else // list of patterns
		{
			Config.pattern = FParser->Current().value();
			FParser->at(0).value() = "Pattern: " + Config.pattern;
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
		for (auto it = selected.begin(); it != selected.end(); ++it)
			EditedSongs.push_back(&(*Tags)[*it].value());
	}
	else
		for (size_t i = 0; i < Tags->Size(); ++i)
			EditedSongs.push_back(&(*Tags)[i].value());
	
	size_t id = TagTypes->Choice();
	
	if (w == TagTypes && id == 5)
	{
		bool yes = Action::AskYesNoQuestion("Number tracks?", TraceMpdStatus);
		if (yes)
		{
			auto it = EditedSongs.begin();
			for (unsigned i = 1; i <= EditedSongs.size(); ++i, ++it)
			{
				if (Config.tag_editor_extended_numeration)
					(*it)->setTrack(unsignedIntTo<std::string>::apply(i) + "/" + unsignedIntTo<std::string>::apply(EditedSongs.size()));
				else
					(*it)->setTrack(unsignedIntTo<std::string>::apply(i));
			}
			ShowMessage("Tracks numbered");
		}
		else
			ShowMessage("Aborted");
		return;
	}
	
	if (id < 11)
	{
		MPD::Song::GetFunction get = SongInfo::Tags[id].Get;
		MPD::MutableSong::SetFunction set = SongInfo::Tags[id].Set;
		if (id > 0 && w == TagTypes)
		{
			LockStatusbar();
			Statusbar() << NC::fmtBold << TagTypes->Current().value() << NC::fmtBoldEnd << ": ";
			std::string new_tag = wFooter->GetString(Tags->Current().value().getTags(get));
			UnlockStatusbar();
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				(*it)->setTag(set, new_tag);
		}
		else if (w == Tags)
		{
			LockStatusbar();
			Statusbar() << NC::fmtBold << TagTypes->Current().value() << NC::fmtBoldEnd << ": ";
			std::string new_tag = wFooter->GetString(Tags->Current().value().getTags(get));
			UnlockStatusbar();
			if (new_tag != Tags->Current().value().getTags(get))
				Tags->Current().value().setTag(set, new_tag);
			Tags->Scroll(NC::wDown);
		}
	}
	else
	{
		if (id == 12) // filename related options
		{
			if (w == TagTypes)
			{
				if (size_t(COLS) < FParserDialogWidth || MainHeight < FParserDialogHeight)
				{
					ShowMessage("Screen is too small to display additional windows");
					return;
				}
				FParserDialog->Reset();
				w = FParserDialog;
			}
			else if (w == Tags)
			{
				MPD::MutableSong &s = Tags->Current().value();
				std::string old_name = s.getNewURI().empty() ? s.getName() : s.getNewURI();
				size_t last_dot = old_name.rfind(".");
				std::string extension = old_name.substr(last_dot);
				old_name = old_name.substr(0, last_dot);
				LockStatusbar();
				Statusbar() << NC::fmtBold << "New filename: " << NC::fmtBoldEnd;
				std::string new_name = wFooter->GetString(old_name);
				UnlockStatusbar();
				if (!new_name.empty() && new_name != old_name)
					s.setNewURI(new_name + extension);
				Tags->Scroll(NC::wDown);
			}
		}
		else if (id == 16) // capitalize first letters
		{
			ShowMessage("Processing...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				CapitalizeFirstLetters(**it);
			ShowMessage("Done");
		}
		else if (id == 17) // lower all letters
		{
			ShowMessage("Processing...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				LowerAllLetters(**it);
			ShowMessage("Done");
		}
		else if (id == 18) // reset
		{
			Tags->Clear();
			ShowMessage("Changes reset");
		}
		else if (id == 19) // save
		{
			bool success = 1;
			ShowMessage("Writing changes...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			{
				ShowMessage("Writing tags in \"%s\"...", (*it)->getName().c_str());
				if (!WriteTags(**it))
				{
					const char msg[] = "Error while writing tags in \"%s\"";
					ShowMessage(msg, Shorten(TO_WSTRING((*it)->getURI()), COLS-const_strlen(msg)).c_str());
					success = 0;
					break;
				}
			}
			if (success)
			{
				ShowMessage("Tags updated");
				TagTypes->HighlightColor(Config.main_highlight_color);
				TagTypes->Reset();
				w->Refresh();
				w = LeftColumn;
				LeftColumn->HighlightColor(Config.active_column_color);
				Mpd.UpdateDirectory(getSharedDirectory(Tags->BeginV(), Tags->EndV()));
			}
			else
				Tags->Clear();
		}
	}
}

void TagEditor::SpacePressed()
{
	if (w == Tags && !Tags->Empty())
	{
		Tags->Current().setSelected(!Tags->Current().isSelected());
		w->Scroll(NC::wDown);
		return;
	}
	if (w != LeftColumn)
		return;
	
	Config.albums_in_tag_editor = !Config.albums_in_tag_editor;
	w = LeftColumn = Config.albums_in_tag_editor ? Albums : Dirs;
	ShowMessage("Switched to %s view", Config.albums_in_tag_editor ? "albums" : "directories");
	LeftColumn->Display();
	Tags->Clear();
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
				Screen<NC::Window>::MouseButtonPressed(me);
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
				Screen<NC::Window>::MouseButtonPressed(me);
		}
		else if (FParserHelper->hasCoords(me.x, me.y))
		{
			if (w != FParserHelper)
				NextColumn();
			ScrollpadMouseButtonPressed(FParserHelper, me);
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
			Screen<NC::Window>::MouseButtonPressed(me);
		Tags->Clear();
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
			Screen<NC::Window>::MouseButtonPressed(me);
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
			Screen<NC::Window>::MouseButtonPressed(me);
	}
}

/***********************************************************************/

std::string TagEditor::currentFilter()
{
	std::string filter;
	if (w == Dirs)
		filter = RegexFilter< std::pair<std::string, std::string> >::currentFilter(*Dirs);
	else if (w == Albums)
		filter = RegexFilter< std::pair<std::string, std::string> >::currentFilter(*Albums);
	else if (w == Tags)
		filter = RegexFilter<MPD::MutableSong>::currentFilter(*Tags);
	return filter;
}

void TagEditor::applyFilter(const std::string &filter)
{
	if (w == Dirs)
	{
		Dirs->ShowAll();
		auto fun = std::bind(DirEntryMatcher, _1, _2, true);
		auto rx = RegexFilter< std::pair<std::string, std::string> >(filter, Config.regex_type, fun);
		Dirs->filter(Dirs->Begin(), Dirs->End(), rx);
	}
	else if (w == Albums)
	{
		Albums->ShowAll();
		auto rx = RegexFilter< std::pair<std::string, std::string> >(filter, Config.regex_type, AlbumEntryMatcher);
		Albums->filter(Albums->Begin(), Albums->End(), rx);
	}
	else if (w == Tags)
	{
		Tags->ShowAll();
		auto rx = RegexFilter<MPD::MutableSong>(filter, Config.regex_type, SongEntryMatcher);
		Tags->filter(Tags->Begin(), Tags->End(), rx);
	}
}

/***********************************************************************/

bool TagEditor::search(const std::string &constraint)
{
	bool result = false;
	if (w == Dirs)
	{
		auto fun = std::bind(DirEntryMatcher, _1, _2, false);
		auto rx = RegexFilter< std::pair<std::string, std::string> >(constraint, Config.regex_type, fun);
		result = Dirs->search(Dirs->Begin(), Dirs->End(), rx);
	}
	else if (w == Albums)
	{
		auto rx = RegexFilter< std::pair<std::string, std::string> >(constraint, Config.regex_type, AlbumEntryMatcher);
		result = Albums->search(Albums->Begin(), Albums->End(), rx);
	}
	else if (w == Tags)
	{
		auto rx = RegexFilter<MPD::MutableSong>(constraint, Config.regex_type, SongEntryMatcher);
		result = Tags->search(Tags->Begin(), Tags->End(), rx);
	}
	return result;
}

void TagEditor::nextFound(bool wrap)
{
	if (w == Dirs)
		Dirs->NextFound(wrap);
	else if (w == Albums)
		Albums->NextFound(wrap);
	else if (w == Tags)
		Tags->NextFound(wrap);
}

void TagEditor::prevFound(bool wrap)
{
	if (w == Dirs)
		Dirs->PrevFound(wrap);
	else if (w == Albums)
		Albums->PrevFound(wrap);
	else if (w == Tags)
		Tags->PrevFound(wrap);
}

/***********************************************************************/

std::shared_ptr<ProxySongList> TagEditor::getProxySongList()
{
	auto ptr = nullProxySongList();
	if (w == Tags)
		ptr = mkProxySongList(*Tags, [](NC::Menu<MPD::MutableSong>::Item &item) {
			return &item.value();
		});
	return ptr;
}

bool TagEditor::allowsSelection()
{
	return w == Tags;
}

void TagEditor::reverseSelection()
{
	if (w == Tags)
		reverseSelectionHelper(Tags->Begin(), Tags->End());
}

MPD::SongList TagEditor::getSelectedSongs()
{
	MPD::SongList result;
	if (w == Tags)
	{
		for (auto it = Tags->Begin(); it != Tags->End(); ++it)
			if (it->isSelected())
				result.push_back(it->value());
		// if no song was selected, add current one
		if (result.empty() && !Tags->Empty())
			result.push_back(Tags->Current().value());
	}
	return result;
}

/***********************************************************************/

NC::List *TagEditor::GetList()
{
	if (w == LeftColumn)
		return LeftColumn;
	else if (w == Tags)
		return Tags;
	else
		return 0;
}

bool TagEditor::isNextColumnAvailable()
{
	if (w == LeftColumn)
	{
		if (!TagTypes->ReallyEmpty() && !Tags->ReallyEmpty())
			return true;
	}
	else if (w == TagTypes)
	{
		if (!Tags->ReallyEmpty())
			return true;
	}
	else if (w == FParser)
	{
		return true;
	}
	return false;
}

bool TagEditor::NextColumn()
{
	if (w == LeftColumn)
	{
		LeftColumn->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = TagTypes;
		TagTypes->HighlightColor(Config.active_column_color);
		return true;
	}
	else if (w == TagTypes && TagTypes->Choice() < 13 && !Tags->ReallyEmpty())
	{
		TagTypes->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Tags;
		Tags->HighlightColor(Config.active_column_color);
		return true;
	}
	else if (w == FParser)
	{
		FParser->SetBorder(Config.window_border);
		FParser->Display();
		w = FParserHelper;
		FParserHelper->SetBorder(Config.active_window_border);
		FParserHelper->Display();
		return true;
	}
	return false;
}

bool TagEditor::isPrevColumnAvailable()
{
	if (w == Tags)
	{
		if (!TagTypes->ReallyEmpty() && !LeftColumn->ReallyEmpty())
			return true;
	}
	else if (w == TagTypes)
	{
		if (!LeftColumn->ReallyEmpty())
			return true;
	}
	else if (w == FParserHelper)
	{
		return true;
	}
	return false;
}

bool TagEditor::PrevColumn()
{
	if (w == Tags)
	{
		Tags->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = TagTypes;
		TagTypes->HighlightColor(Config.active_column_color);
		return true;
	}
	else if (w == TagTypes)
	{
		TagTypes->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = LeftColumn;
		LeftColumn->HighlightColor(Config.active_column_color);
		return true;
	}
	else if (w == FParserHelper)
	{
		FParserHelper->SetBorder(Config.window_border);
		FParserHelper->Display();
		w = FParser;
		FParser->SetBorder(Config.active_window_border);
		FParser->Display();
		return true;
	}
	return false;
}

void TagEditor::LocateSong(const MPD::Song &s)
{
	if (myScreen == this)
		return;
	
	if (s.getDirectory().empty())
		return;
	
	if (LeftColumn == Albums)
	{
		Config.albums_in_tag_editor = false;
		if (w == LeftColumn)
			w = Dirs;
		LeftColumn = Dirs;
	}
		
	if (Global::myScreen != this)
		SwitchTo();
	
	// go to right directory
	if (itsBrowsedDir != s.getDirectory())
	{
		itsBrowsedDir = s.getDirectory();
		size_t last_slash = itsBrowsedDir.rfind('/');
		if (last_slash != std::string::npos)
			itsBrowsedDir = itsBrowsedDir.substr(0, last_slash);
		else
			itsBrowsedDir = "/";
		if (itsBrowsedDir.empty())
			itsBrowsedDir = "/";
		Dirs->Clear();
		Update();
	}
	if (itsBrowsedDir == "/")
		Dirs->Reset(); // go to the first pos, which is "." (music dir root)
	
	// highlight directory we need and get files from it
	std::string dir = getBasename(s.getDirectory());
	for (size_t i = 0; i < Dirs->Size(); ++i)
	{
		if ((*Dirs)[i].value().first == dir)
		{
			Dirs->Highlight(i);
			break;
		}
	}
	Tags->Clear();
	Update();
	
	// go to the right column
	NextColumn();
	NextColumn();
	
	// highlight our file
	for (size_t i = 0; i < Tags->Size(); ++i)
	{
		if ((*Tags)[i].value().getHash() == s.getHash())
		{
			Tags->Highlight(i);
			break;
		}
	}
}

void TagEditor::ReadTags(MPD::MutableSong &s)
{
	TagLib::FileRef f(s.getURI().c_str());
	if (f.isNull())
		return;
	
	TagLib::MPEG::File *mpegf = dynamic_cast<TagLib::MPEG::File *>(f.file());
	s.setDuration(f.audioProperties()->length());
	
	s.setArtist(f.tag()->artist().to8Bit(1));
	s.setTitle(f.tag()->title().to8Bit(1));
	s.setAlbum(f.tag()->album().to8Bit(1));
	s.setTrack(intTo<std::string>::apply(f.tag()->track()));
	s.setDate(intTo<std::string>::apply(f.tag()->year()));
	s.setGenre(f.tag()->genre().to8Bit(1));
	if (mpegf)
	{
		s.setAlbumArtist(!mpegf->ID3v2Tag()->frameListMap()["TPE2"].isEmpty() ? mpegf->ID3v2Tag()->frameListMap()["TPE2"].front()->toString().to8Bit(1) : "");
		s.setComposer(!mpegf->ID3v2Tag()->frameListMap()["TCOM"].isEmpty() ? mpegf->ID3v2Tag()->frameListMap()["TCOM"].front()->toString().to8Bit(1) : "");
		s.setPerformer(!mpegf->ID3v2Tag()->frameListMap()["TOPE"].isEmpty() ? mpegf->ID3v2Tag()->frameListMap()["TOPE"].front()->toString().to8Bit(1) : "");
		s.setDisc(!mpegf->ID3v2Tag()->frameListMap()["TPOS"].isEmpty() ? mpegf->ID3v2Tag()->frameListMap()["TPOS"].front()->toString().to8Bit(1) : "");
	}
	s.setComment(f.tag()->comment().to8Bit(1));
}

bool TagEditor::WriteTags(MPD::MutableSong &s)
{
	std::string path_to_file;
	bool file_is_from_db = s.isFromDatabase();
	if (file_is_from_db)
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.getURI();
	TagLib::FileRef f(path_to_file.c_str());
	if (!f.isNull())
	{
		f.tag()->setTitle(ToWString(s.getTitle()));
		f.tag()->setArtist(ToWString(s.getArtist()));
		f.tag()->setAlbum(ToWString(s.getAlbum()));
		f.tag()->setYear(stringToInt(s.getDate()));
		f.tag()->setTrack(stringToInt(s.getTrack()));
		f.tag()->setGenre(ToWString(s.getGenre()));
		f.tag()->setComment(ToWString(s.getComment()));
		if (TagLib::MPEG::File *mp3_file = dynamic_cast<TagLib::MPEG::File *>(f.file()))
		{
			TagLib::ID3v2::Tag *tag = mp3_file->ID3v2Tag(1);
			TagLib::StringList list;
			
			WriteID3v2("TIT2", tag, ToWString(s.getTitle()));  // title
			WriteID3v2("TPE1", tag, ToWString(s.getArtist())); // artist
			WriteID3v2("TALB", tag, ToWString(s.getAlbum()));  // album
			WriteID3v2("TDRC", tag, ToWString(s.getDate()));   // date
			WriteID3v2("TRCK", tag, ToWString(s.getTrack()));  // track
			WriteID3v2("TCON", tag, ToWString(s.getGenre()));  // genre
			WriteID3v2("TPOS", tag, ToWString(s.getDisc()));   // disc
			
			GetTagList(list, s, &MPD::Song::getAlbumArtist);
			WriteID3v2("TPE2", tag, list); // album artist
			
			GetTagList(list, s, &MPD::Song::getComposer);
			WriteID3v2("TCOM", tag, list); // composer
			
			GetTagList(list, s, &MPD::Song::getPerformer);
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
		
		if (!s.getNewURI().empty())
		{
			std::string new_name;
			if (file_is_from_db)
				new_name += Config.mpd_music_dir;
			new_name += s.getDirectory() + "/" + s.getNewURI();
			locale_to_utf(new_name);
			if (rename(path_to_file.c_str(), new_name.c_str()) == 0 && !file_is_from_db)
			{
				if (Global::myOldScreen == myPlaylist)
				{
					// if we rename local file, it won't get updated
					// so just remove it from playlist and add again
					size_t pos = myPlaylist->Items->Choice();
					Mpd.StartCommandsList();
					Mpd.Delete(pos);
					int id = Mpd.AddSong("file://" + new_name);
					if (id >= 0)
					{
						s = myPlaylist->Items->Back().value();
						Mpd.Move(s.getPosition(), pos);
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

namespace {//

std::string CapitalizeFirstLetters(const std::string &s)
{
	if (s.empty())
		return "";
	std::string result = s;
	if (!isspace(result[0]))
		result[0] = toupper(result[0]);
	for (std::string::iterator it = result.begin()+1; it != result.end(); ++it)
	{
		if (!isspace(*it) && isspace(*(it-1)) && *(it-1) != '\'')
			*it = toupper(*it);
	}
	return result;
}

void CapitalizeFirstLetters(MPD::MutableSong &s)
{
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m)
	{
		unsigned i = 0;
		for (std::string tag; !(tag = (s.*m->Get)(i)).empty(); ++i)
			(s.*m->Set)(CapitalizeFirstLetters(tag), i);
	}
}

void LowerAllLetters(MPD::MutableSong &s)
{
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m)
	{
		unsigned i = 0;
		for (std::string tag; !(tag = (s.*m->Get)(i)).empty(); ++i)
		{
			lowercase(tag);
			(s.*m->Set)(tag, i);
		}
	}
}

void GetTagList(TagLib::StringList &list, const MPD::MutableSong &s, MPD::Song::GetFunction f)
{
	list.clear();
	unsigned pos = 0;
	for (std::string value; !(value = (s.*f)(pos)).empty(); ++pos)
		list.append(ToWString(value));
}

template <typename T> void WriteID3v2(const TagLib::ByteVector &type, TagLib::ID3v2::Tag *tag, const T &list)
{
	using TagLib::ID3v2::TextIdentificationFrame;
	tag->removeFrames(type);
	TextIdentificationFrame *frame = new TextIdentificationFrame(type, TagLib::String::UTF8);
	frame->setText(list);
	tag->addFrame(frame);
}

void WriteXiphComments(const MPD::MutableSong &s, TagLib::Ogg::XiphComment *tag)
{
	TagLib::StringList list;
	
	tag->addField("DISCNUMBER", ToWString(s.getDisc())); // disc
	
	tag->removeField("ALBUM ARTIST"); // album artist
	GetTagList(list, s, &MPD::Song::getAlbumArtist);
	for (TagLib::StringList::ConstIterator it = list.begin(); it != list.end(); ++it)
		tag->addField("ALBUM ARTIST", *it, 0);
	
	tag->removeField("COMPOSER"); // composer
	GetTagList(list, s, &MPD::Song::getComposer);
	for (TagLib::StringList::ConstIterator it = list.begin(); it != list.end(); ++it)
		tag->addField("COMPOSER", *it, 0);
	
	tag->removeField("PERFORMER"); // performer
	GetTagList(list, s, &MPD::Song::getPerformer);
	for (TagLib::StringList::ConstIterator it = list.begin(); it != list.end(); ++it)
		tag->addField("PERFORMER", *it, 0);
}

void GetPatternList()
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

void SavePatternList()
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
MPD::MutableSong::SetFunction IntoSetFunction(char c)
{
	switch (c)
	{
		case 'a':
			return &MPD::MutableSong::setArtist;
		case 'A':
			return &MPD::MutableSong::setAlbumArtist;
		case 't':
			return &MPD::MutableSong::setTitle;
		case 'b':
			return &MPD::MutableSong::setAlbum;
		case 'y':
			return &MPD::MutableSong::setDate;
		case 'n':
			return &MPD::MutableSong::setTrack;
		case 'g':
			return &MPD::MutableSong::setGenre;
		case 'c':
			return &MPD::MutableSong::setComposer;
		case 'p':
			return &MPD::MutableSong::setPerformer;
		case 'd':
			return &MPD::MutableSong::setDisc;
		case 'C':
			return &MPD::MutableSong::setComment;
		default:
			return 0;
	}
}

std::string GenerateFilename(const MPD::MutableSong &s, const std::string &pattern)
{
	std::string result = s.toString(pattern);
	removeInvalidCharsFromFilename(result);
	return result;
}

std::string ParseFilename(MPD::MutableSong &s, std::string mask, bool preview)
{
	std::ostringstream result;
	std::vector<std::string> separators;
	std::vector< std::pair<char, std::string> > tags;
	std::string file = s.getName().substr(0, s.getName().rfind("."));
	
	for (size_t i = mask.find("%"); i != std::string::npos; i = mask.find("%"))
	{
		tags.push_back(std::make_pair(mask.at(i+1), ""));
		mask = mask.substr(i+2);
		i = mask.find("%");
		if (!mask.empty())
			separators.push_back(mask.substr(0, i));
	}
	size_t i = 0;
	for (auto it = separators.begin(); it != separators.end(); ++it, ++i)
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
	
	for (auto it = tags.begin(); it != tags.end(); ++it)
	{
		for (std::string::iterator j = it->second.begin(); j != it->second.end(); ++j)
			if (*j == '_')
				*j = ' ';
			
			if (!preview)
			{
				MPD::MutableSong::SetFunction set = IntoSetFunction(it->first);
				if (set)
					s.setTag(set, it->second);
			}
			else
				result << "%" << it->first << ": " << it->second << "\n";
	}
	return result.str();
}

std::string SongToString(const MPD::MutableSong &s)
{
	std::string result;
	size_t i = myTagEditor->TagTypes->Choice();
	if (i < 11)
		result = (s.*SongInfo::Tags[i].Get)(0);
	else if (i == 12)
		result = s.getNewURI().empty() ? s.getName() : s.getName() + " -> " + s.getNewURI();
	return result.empty() ? Config.empty_tag : result;
}

bool DirEntryMatcher(const Regex &rx, const std::pair<std::string, std::string> &dir, bool filter)
{
	if (dir.first == "." || dir.first == "..")
		return filter;
	return rx.match(dir.first);
}

bool AlbumEntryMatcher(const Regex &rx, const std::pair<std::string, std::string> &dir)
{
	return rx.match(dir.first);
}

bool SongEntryMatcher(const Regex &rx, const MPD::MutableSong &s)
{
	return rx.match(SongToString(s));
}

}

#endif

