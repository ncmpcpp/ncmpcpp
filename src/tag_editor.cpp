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

#include <algorithm>
#include <fstream>
#include <sstream>

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "song_info.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "title.h"
#include "tags.h"

using namespace std::placeholders;

using Global::myScreen;
using Global::MainHeight;
using Global::MainStartY;

TagEditor *myTagEditor;

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

bool isAnyModified(const NC::Menu<MPD::MutableSong> &m);

std::string CapitalizeFirstLetters(const std::string &s);
void CapitalizeFirstLetters(MPD::MutableSong &s);
void LowerAllLetters(MPD::MutableSong &s);

void GetPatternList();
void SavePatternList();

MPD::MutableSong::SetFunction IntoSetFunction(char c);
std::string GenerateFilename(const MPD::MutableSong &s, const std::string &pattern);
std::string ParseFilename(MPD::MutableSong &s, std::string mask, bool preview);

std::string SongToString(const MPD::MutableSong &s);
bool DirEntryMatcher(const Regex &rx, const std::pair<std::string, std::string> &dir, bool filter);
bool SongEntryMatcher(const Regex &rx, const MPD::MutableSong &s);

}

TagEditor::TagEditor() : FParser(0), FParserHelper(0), FParserLegend(0), FParserPreview(0), itsBrowsedDir("/")
{
	PatternsFile = Config.ncmpcpp_directory + "patterns.list";
	SetDimensions(0, COLS);
	
	Dirs = new NC::Menu< std::pair<std::string, std::string> >(0, MainStartY, LeftColumnWidth, MainHeight, Config.titles_visibility ? "Directories" : "", Config.main_color, NC::brNone);
	Dirs->setHighlightColor(Config.active_column_color);
	Dirs->cyclicScrolling(Config.use_cyclic_scrolling);
	Dirs->centeredCursor(Config.centered_cursor);
	Dirs->setItemDisplayer(Display::Pair<std::string, std::string>);
	
	TagTypes = new NC::Menu<std::string>(MiddleColumnStartX, MainStartY, MiddleColumnWidth, MainHeight, Config.titles_visibility ? "Tag types" : "", Config.main_color, NC::brNone);
	TagTypes->setHighlightColor(Config.main_highlight_color);
	TagTypes->cyclicScrolling(Config.use_cyclic_scrolling);
	TagTypes->centeredCursor(Config.centered_cursor);
	TagTypes->setItemDisplayer(Display::Default<std::string>);
	
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m)
		TagTypes->addItem(m->Name);
	TagTypes->addSeparator();
	TagTypes->addItem("Filename");
	TagTypes->addSeparator();
	if (Config.titles_visibility)
		TagTypes->addItem("Options", 1, 1);
	TagTypes->addSeparator();
	TagTypes->addItem("Capitalize First Letters");
	TagTypes->addItem("lower all letters");
	TagTypes->addSeparator();
	TagTypes->addItem("Reset");
	TagTypes->addItem("Save");
	
	Tags = new NC::Menu<MPD::MutableSong>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Tags" : "", Config.main_color, NC::brNone);
	Tags->setHighlightColor(Config.main_highlight_color);
	Tags->cyclicScrolling(Config.use_cyclic_scrolling);
	Tags->centeredCursor(Config.centered_cursor);
	Tags->setSelectedPrefix(Config.selected_item_prefix);
	Tags->setSelectedSuffix(Config.selected_item_suffix);
	Tags->setItemDisplayer(Display::Tags);
	
	FParserDialog = new NC::Menu<std::string>((COLS-FParserDialogWidth)/2, (MainHeight-FParserDialogHeight)/2+MainStartY, FParserDialogWidth, FParserDialogHeight, "", Config.main_color, Config.window_border);
	FParserDialog->cyclicScrolling(Config.use_cyclic_scrolling);
	FParserDialog->centeredCursor(Config.centered_cursor);
	FParserDialog->setItemDisplayer(Display::Default<std::string>);
	FParserDialog->addItem("Get tags from filename");
	FParserDialog->addItem("Rename files");
	FParserDialog->addSeparator();
	FParserDialog->addItem("Cancel");
	
	FParser = new NC::Menu<std::string>((COLS-FParserWidth)/2, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthOne, FParserHeight, "_", Config.main_color, Config.active_window_border);
	FParser->cyclicScrolling(Config.use_cyclic_scrolling);
	FParser->centeredCursor(Config.centered_cursor);
	FParser->setItemDisplayer(Display::Default<std::string>);
	
	FParserLegend = new NC::Scrollpad((COLS-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthTwo, FParserHeight, "Legend", Config.main_color, Config.window_border);
	
	FParserPreview = new NC::Scrollpad((COLS-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthTwo, FParserHeight, "Preview", Config.main_color, Config.window_border);
	
	w = Dirs;
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
	FParserDialogHeight = std::min(size_t(5), MainHeight);
	FParserWidth = width*0.9;
	FParserHeight = std::min(size_t(LINES*0.8), MainHeight);
	FParserWidthOne = FParserWidth/2;
	FParserWidthTwo = FParserWidth-FParserWidthOne;
}

void TagEditor::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	SetDimensions(x_offset, width);
	
	Dirs->resize(LeftColumnWidth, MainHeight);
	TagTypes->resize(MiddleColumnWidth, MainHeight);
	Tags->resize(RightColumnWidth, MainHeight);
	FParserDialog->resize(FParserDialogWidth, FParserDialogHeight);
	FParser->resize(FParserWidthOne, FParserHeight);
	FParserLegend->resize(FParserWidthTwo, FParserHeight);
	FParserPreview->resize(FParserWidthTwo, FParserHeight);
	
	Dirs->moveTo(LeftColumnStartX, MainStartY);
	TagTypes->moveTo(MiddleColumnStartX, MainStartY);
	Tags->moveTo(RightColumnStartX, MainStartY);
	
	FParserDialog->moveTo(x_offset+(width-FParserDialogWidth)/2, (MainHeight-FParserDialogHeight)/2+MainStartY);
	FParser->moveTo(x_offset+(width-FParserWidth)/2, (MainHeight-FParserHeight)/2+MainStartY);
	FParserLegend->moveTo(x_offset+(width-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY);
	FParserPreview->moveTo(x_offset+(width-FParserWidth)/2+FParserWidthOne, (MainHeight-FParserHeight)/2+MainStartY);
	
	hasToBeResized = 0;
}

std::wstring TagEditor::title()
{
	return L"Tag editor";
}

void TagEditor::switchTo()
{
	using Global::myLockedScreen;
	
	if (myScreen == this)
		return;
	
	if (myLockedScreen)
		updateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	drawHeader();
	refresh();
}

void TagEditor::refresh()
{
	Dirs->display();
	mvvline(MainStartY, MiddleColumnStartX-1, 0, MainHeight);
	TagTypes->display();
	mvvline(MainStartY, RightColumnStartX-1, 0, MainHeight);
	Tags->display();
	
	if (w == FParserDialog)
	{
		FParserDialog->display();
	}
	else if (w == FParser || w == FParserHelper)
	{
		FParser->display();
		FParserHelper->display();
	}
}

void TagEditor::update()
{
	if (Dirs->reallyEmpty())
	{
		Dirs->Window::clear();
		Tags->clear();
		
		int highlightme = -1;
		auto dirs = Mpd.GetDirectories(itsBrowsedDir);
		std::sort(dirs.begin(), dirs.end(), LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
		if (itsBrowsedDir != "/")
		{
			size_t slash = itsBrowsedDir.rfind("/");
			std::string parent = slash != std::string::npos ? itsBrowsedDir.substr(0, slash) : "/";
			Dirs->addItem(make_pair("..", parent));
		}
		else
			Dirs->addItem(std::make_pair(".", "/"));
		for (auto dir = dirs.begin(); dir != dirs.end(); ++dir)
		{
			size_t slash = dir->rfind("/");
			std::string to_display = slash != std::string::npos ? dir->substr(slash+1) : *dir;
			Dirs->addItem(make_pair(to_display, *dir));
			if (*dir == itsHighlightedDir)
				highlightme = Dirs->size()-1;
		}
		if (highlightme != -1)
			Dirs->highlight(highlightme);
		
		Dirs->display();
	}
	
	if (Tags->reallyEmpty())
	{
		Tags->reset();
		auto songs = Mpd.GetSongs(Dirs->current().value().second);
		std::sort(songs.begin(), songs.end(),
			LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
		for (auto s = songs.begin(); s != songs.end(); ++s)
			Tags->addItem(*s);
		Tags->refresh();
	}
	
	if (w == TagTypes && TagTypes->choice() < 13)
	{
		Tags->refresh();
	}
	else if (TagTypes->choice() >= 13)
	{
		Tags->Window::clear();
		Tags->Window::refresh();
	}
}

void TagEditor::enterPressed()
{
	using Global::wFooter;
	
	if (w == Dirs)
	{
		auto dirs = Mpd.GetDirectories(Dirs->current().value().second);
		if (!dirs.empty())
		{
			itsHighlightedDir = itsBrowsedDir;
			itsBrowsedDir = Dirs->current().value().second;
			Dirs->clear();
			Dirs->reset();
		}
		else
			Statusbar::msg("No subdirectories found");
	}
	else if (w == FParserDialog)
	{
		size_t choice = FParserDialog->choice();
		if (choice == 3) // cancel
		{
			w = TagTypes;
			refresh();
			return;
		}
		GetPatternList();
		
		// prepare additional windows
		
		FParserLegend->clear();
		*FParserLegend << L"%a - artist\n";
		*FParserLegend << L"%A - album artist\n";
		*FParserLegend << L"%t - title\n";
		*FParserLegend << L"%b - album\n";
		*FParserLegend << L"%y - date\n";
		*FParserLegend << L"%n - track number\n";
		*FParserLegend << L"%g - genre\n";
		*FParserLegend << L"%c - composer\n";
		*FParserLegend << L"%p - performer\n";
		*FParserLegend << L"%d - disc\n";
		*FParserLegend << L"%C - comment\n\n";
		*FParserLegend << NC::fmtBold << L"Files:\n" << NC::fmtBoldEnd;
		for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			*FParserLegend << Config.color2 << L" * " << NC::clEnd << (*it)->getName() << '\n';
		FParserLegend->flush();
		
		if (!Patterns.empty())
			Config.pattern = Patterns.front();
		FParser->clear();
		FParser->reset();
		FParser->addItem("Pattern: " + Config.pattern);
		FParser->addItem("Preview");
		FParser->addItem("Legend");
		FParser->addSeparator();
		FParser->addItem("Proceed");
		FParser->addItem("Cancel");
		if (!Patterns.empty())
		{
			FParser->addSeparator();
			FParser->addItem("Recent patterns", 1, 1);
			FParser->addSeparator();
			for (std::list<std::string>::const_iterator it = Patterns.begin(); it != Patterns.end(); ++it)
				FParser->addItem(*it);
		}
		
		FParser->setTitle(choice == 0 ? "Get tags from filename" : "Rename files");
		w = FParser;
		FParserUsePreview = 1;
		FParserHelper = FParserLegend;
		FParserHelper->display();
	}
	else if (w == FParser)
	{
		bool quit = 0;
		size_t pos = FParser->choice();
		
		if (pos == 4) // save
			FParserUsePreview = 0;
		
		if (pos == 0) // change pattern
		{
			Statusbar::lock();
			Statusbar::put() << "Pattern: ";
			std::string new_pattern = wFooter->getString(Config.pattern);
			Statusbar::unlock();
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
			Statusbar::msg("Parsing...");
			FParserPreview->clear();
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			{
				MPD::MutableSong &s = **it;
				if (FParserDialog->choice() == 0) // get tags from filename
				{
					if (FParserUsePreview)
					{
						*FParserPreview << NC::fmtBold << s.getName() << L":\n" << NC::fmtBoldEnd;
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
						Statusbar::msg("File \"%s\" would have an empty name", s.getName().c_str());
						FParserUsePreview = 1;
						success = 0;
					}
					if (!FParserUsePreview)
						s.setNewURI(new_file + extension);
					*FParserPreview << file << Config.color2 << L" -> " << NC::clEnd;
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
				FParserHelper->flush();
				FParserHelper->display();
			}
			else if (success)
			{
				Patterns.remove(Config.pattern);
				Patterns.insert(Patterns.begin(), Config.pattern);
				quit = 1;
			}
			if (pos != 4 || success)
				Statusbar::msg("Operation finished");
		}
		else if (pos == 2) // show legend
		{
			FParserHelper = FParserLegend;
			FParserHelper->display();
		}
		else if (pos == 5) // cancel
		{
			quit = 1;
		}
		else // list of patterns
		{
			Config.pattern = FParser->current().value();
			FParser->at(0).value() = "Pattern: " + Config.pattern;
		}
		
		if (quit)
		{
			SavePatternList();
			w = TagTypes;
			refresh();
			return;
		}
	}
	
	if ((w != TagTypes && w != Tags) || Tags->empty()) // after this point we start dealing with tags
		return;
	
	EditedSongs.clear();
	if (Tags->hasSelected()) // if there are selected songs, perform operations only on them
	{
		for (auto it = Tags->begin(); it != Tags->end(); ++it)
			if (it->isSelected())
				EditedSongs.push_back(&it->value());
	}
	else
	{
		for (auto it = Tags->begin(); it != Tags->end(); ++it)
			EditedSongs.push_back(&it->value());
	}
	
	size_t id = TagTypes->choice();
	
	if (w == TagTypes && id == 5)
	{
		bool yes = Action::AskYesNoQuestion("Number tracks?", Status::trace);
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
			Statusbar::msg("Tracks numbered");
		}
		else
			Statusbar::msg("Aborted");
		return;
	}
	
	if (id < 11)
	{
		MPD::Song::GetFunction get = SongInfo::Tags[id].Get;
		MPD::MutableSong::SetFunction set = SongInfo::Tags[id].Set;
		if (id > 0 && w == TagTypes)
		{
			Statusbar::lock();
			Statusbar::put() << NC::fmtBold << TagTypes->current().value() << NC::fmtBoldEnd << ": ";
			std::string new_tag = wFooter->getString(Tags->current().value().getTags(get, Config.tags_separator));
			Statusbar::unlock();
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				(*it)->setTags(set, new_tag, Config.tags_separator);
		}
		else if (w == Tags)
		{
			Statusbar::lock();
			Statusbar::put() << NC::fmtBold << TagTypes->current().value() << NC::fmtBoldEnd << ": ";
			std::string new_tag = wFooter->getString(Tags->current().value().getTags(get, Config.tags_separator));
			Statusbar::unlock();
			if (new_tag != Tags->current().value().getTags(get, Config.tags_separator))
				Tags->current().value().setTags(set, new_tag, Config.tags_separator);
			Tags->scroll(NC::wDown);
		}
	}
	else
	{
		if (id == 12) // filename related options
		{
			if (w == TagTypes)
			{
				FParserDialog->reset();
				w = FParserDialog;
			}
			else if (w == Tags)
			{
				MPD::MutableSong &s = Tags->current().value();
				std::string old_name = s.getNewURI().empty() ? s.getName() : s.getNewURI();
				size_t last_dot = old_name.rfind(".");
				std::string extension = old_name.substr(last_dot);
				old_name = old_name.substr(0, last_dot);
				Statusbar::lock();
				Statusbar::put() << NC::fmtBold << "New filename: " << NC::fmtBoldEnd;
				std::string new_name = wFooter->getString(old_name);
				Statusbar::unlock();
				if (!new_name.empty() && new_name != old_name)
					s.setNewURI(new_name + extension);
				Tags->scroll(NC::wDown);
			}
		}
		else if (id == 16) // capitalize first letters
		{
			Statusbar::msg("Processing...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				CapitalizeFirstLetters(**it);
			Statusbar::msg("Done");
		}
		else if (id == 17) // lower all letters
		{
			Statusbar::msg("Processing...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				LowerAllLetters(**it);
			Statusbar::msg("Done");
		}
		else if (id == 19) // reset
		{
			for (auto it = Tags->beginV(); it != Tags->endV(); ++it)
				it->clearModifications();
			Statusbar::msg("Changes reset");
		}
		else if (id == 20) // save
		{
			bool success = 1;
			Statusbar::msg("Writing changes...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			{
				Statusbar::msg("Writing tags in \"%s\"...", (*it)->getName().c_str());
				if (!Tags::write(**it))
				{
					const char msg[] = "Error while writing tags in \"%ls\"";
					Statusbar::msg(msg, wideShorten(ToWString((*it)->getURI()), COLS-const_strlen(msg)).c_str());
					success = 0;
					break;
				}
			}
			if (success)
			{
				Statusbar::msg("Tags updated");
				TagTypes->setHighlightColor(Config.main_highlight_color);
				TagTypes->reset();
				w->refresh();
				w = Dirs;
				Dirs->setHighlightColor(Config.active_column_color);
				Mpd.UpdateDirectory(getSharedDirectory(Tags->beginV(), Tags->endV()));
			}
			else
				Tags->clear();
		}
	}
}

void TagEditor::spacePressed()
{
	if (w == Tags && !Tags->empty())
	{
		Tags->current().setSelected(!Tags->current().isSelected());
		w->scroll(NC::wDown);
	}
}

void TagEditor::mouseButtonPressed(MEVENT me)
{
	auto tryPreviousColumn = [this]() -> bool {
		bool result = true;
		if (w != Dirs)
		{
			if (previousColumnAvailable())
				previousColumn();
			else
				result = false;
		}
		return result;
	};
	auto tryNextColumn = [this]() -> bool {
		bool result = true;
		if (w != Tags)
		{
			if (nextColumnAvailable())
				nextColumn();
			else
				result = false;
		}
		return result;
	};
	if (w == FParserDialog)
	{
		if (FParserDialog->hasCoords(me.x, me.y))
		{
			if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
			{
				FParserDialog->Goto(me.y);
				if (me.bstate & BUTTON3_PRESSED)
					enterPressed();
			}
			else
				Screen<WindowType>::mouseButtonPressed(me);
		}
	}
	else if (w == FParser || w == FParserHelper)
	{
		if (FParser->hasCoords(me.x, me.y))
		{
			if (w != FParser)
			{
				if (previousColumnAvailable())
					previousColumn();
				else
					return;
			}
			if (size_t(me.y) < FParser->size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
			{
				FParser->Goto(me.y);
				if (me.bstate & BUTTON3_PRESSED)
					enterPressed();
			}
			else
				Screen<WindowType>::mouseButtonPressed(me);
		}
		else if (FParserHelper->hasCoords(me.x, me.y))
		{
			if (w != FParserHelper)
			{
				if (nextColumnAvailable())
					nextColumn();
				else
					return;
			}
			scrollpadMouseButtonPressed(*FParserHelper, me);
		}
	}
	else if (!Dirs->empty() && Dirs->hasCoords(me.x, me.y))
	{
		if (!tryPreviousColumn() || !tryPreviousColumn())
			return;
		if (size_t(me.y) < Dirs->size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Dirs->Goto(me.y);
			if (me.bstate & BUTTON1_PRESSED)
				enterPressed();
			else
				spacePressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
		Tags->clear();
	}
	else if (!TagTypes->empty() && TagTypes->hasCoords(me.x, me.y))
	{
		if (w != TagTypes)
		{
			bool success;
			if (w == Dirs)
				success = tryNextColumn();
			else
				success = tryPreviousColumn();
			if (!success)
				return;
		}
		if (size_t(me.y) < TagTypes->size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			if (!TagTypes->Goto(me.y))
				return;
			TagTypes->refresh();
			Tags->refresh();
			if (me.bstate & BUTTON3_PRESSED)
				enterPressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
	else if (!Tags->empty() && Tags->hasCoords(me.x, me.y))
	{
		if (!tryNextColumn() || !tryNextColumn())
			return;
		if (size_t(me.y) < Tags->size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Tags->Goto(me.y);
			Tags->refresh();
			if (me.bstate & BUTTON3_PRESSED)
				enterPressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/***********************************************************************/

bool TagEditor::allowsFiltering()
{
	return w == Dirs || w == Tags;
}

std::string TagEditor::currentFilter()
{
	std::string filter;
	if (w == Dirs)
		filter = RegexFilter< std::pair<std::string, std::string> >::currentFilter(*Dirs);
	else if (w == Tags)
		filter = RegexFilter<MPD::MutableSong>::currentFilter(*Tags);
	return filter;
}

void TagEditor::applyFilter(const std::string &filter)
{
	if (w == Dirs)
	{
		Dirs->showAll();
		auto fun = std::bind(DirEntryMatcher, _1, _2, true);
		auto rx = RegexFilter< std::pair<std::string, std::string> >(filter, Config.regex_type, fun);
		Dirs->filter(Dirs->begin(), Dirs->end(), rx);
	}
	else if (w == Tags)
	{
		Tags->showAll();
		auto rx = RegexFilter<MPD::MutableSong>(filter, Config.regex_type, SongEntryMatcher);
		Tags->filter(Tags->begin(), Tags->end(), rx);
	}
}

/***********************************************************************/

bool TagEditor::allowsSearching()
{
	return w == Dirs || w == Tags;
}

bool TagEditor::search(const std::string &constraint)
{
	bool result = false;
	if (w == Dirs)
	{
		auto fun = std::bind(DirEntryMatcher, _1, _2, false);
		auto rx = RegexFilter< std::pair<std::string, std::string> >(constraint, Config.regex_type, fun);
		result = Dirs->search(Dirs->begin(), Dirs->end(), rx);
	}
	else if (w == Tags)
	{
		auto rx = RegexFilter<MPD::MutableSong>(constraint, Config.regex_type, SongEntryMatcher);
		result = Tags->search(Tags->begin(), Tags->end(), rx);
	}
	return result;
}

void TagEditor::nextFound(bool wrap)
{
	if (w == Dirs)
		Dirs->nextFound(wrap);
	else if (w == Tags)
		Tags->nextFound(wrap);
}

void TagEditor::prevFound(bool wrap)
{
	if (w == Dirs)
		Dirs->prevFound(wrap);
	else if (w == Tags)
		Tags->prevFound(wrap);
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
		reverseSelectionHelper(Tags->begin(), Tags->end());
}

MPD::SongList TagEditor::getSelectedSongs()
{
	MPD::SongList result;
	if (w == Tags)
	{
		for (auto it = Tags->begin(); it != Tags->end(); ++it)
			if (it->isSelected())
				result.push_back(it->value());
		// if no song was selected, add current one
		if (result.empty() && !Tags->empty())
			result.push_back(Tags->current().value());
	}
	return result;
}

/***********************************************************************/

bool TagEditor::previousColumnAvailable()
{
	bool result = false;
	if (w == Tags)
	{
		if (!TagTypes->reallyEmpty() && !Dirs->reallyEmpty())
			result = true;
	}
	else if (w == TagTypes)
	{
		if (!Dirs->reallyEmpty())
			result = ifAnyModifiedAskForDiscarding();
	}
	else if (w == FParserHelper)
		result = true;
	return result;
}

void TagEditor::previousColumn()
{
	if (w == Tags)
	{
		Tags->setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = TagTypes;
		TagTypes->setHighlightColor(Config.active_column_color);
	}
	else if (w == TagTypes)
	{
		TagTypes->setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = Dirs;
		Dirs->setHighlightColor(Config.active_column_color);
	}
	else if (w == FParserHelper)
	{
		FParserHelper->setBorder(Config.window_border);
		FParserHelper->display();
		w = FParser;
		FParser->setBorder(Config.active_window_border);
		FParser->display();
	}
}

bool TagEditor::nextColumnAvailable()
{
	bool result = false;
	if (w == Dirs)
	{
		if (!TagTypes->reallyEmpty() && !Tags->reallyEmpty())
			result = true;
	}
	else if (w == TagTypes)
	{
		if (!Tags->reallyEmpty())
			result = true;
	}
	else if (w == FParser)
		result = true;
	return result;
}

void TagEditor::nextColumn()
{
	if (w == Dirs)
	{
		Dirs->setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = TagTypes;
		TagTypes->setHighlightColor(Config.active_column_color);
	}
	else if (w == TagTypes && TagTypes->choice() < 13 && !Tags->reallyEmpty())
	{
		TagTypes->setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = Tags;
		Tags->setHighlightColor(Config.active_column_color);
	}
	else if (w == FParser)
	{
		FParser->setBorder(Config.window_border);
		FParser->display();
		w = FParserHelper;
		FParserHelper->setBorder(Config.active_window_border);
		FParserHelper->display();
	}
}

/***********************************************************************/

bool TagEditor::ifAnyModifiedAskForDiscarding()
{
	bool result = true;
	if (isAnyModified(*Tags))
		result = Action::AskYesNoQuestion("There are pending changes, are you sure?", Status::trace);
	return result;
}

void TagEditor::LocateSong(const MPD::Song &s)
{
	if (myScreen == this)
		return;
	
	if (s.getDirectory().empty())
		return;
	
	if (Global::myScreen != this)
		switchTo();
	
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
		Dirs->clear();
		update();
	}
	if (itsBrowsedDir == "/")
		Dirs->reset(); // go to the first pos, which is "." (music dir root)
	
	// highlight directory we need and get files from it
	std::string dir = getBasename(s.getDirectory());
	for (size_t i = 0; i < Dirs->size(); ++i)
	{
		if ((*Dirs)[i].value().first == dir)
		{
			Dirs->highlight(i);
			break;
		}
	}
	// refresh window so we can be highlighted item
	Dirs->refresh();
	
	Tags->clear();
	update();
	
	// reset TagTypes since it can be under Filename
	// and then songs in right column are not visible.
	TagTypes->reset();
	// go to the right column
	nextColumn();
	nextColumn();
	
	// highlight our file
	for (size_t i = 0; i < Tags->size(); ++i)
	{
		if ((*Tags)[i].value().getHash() == s.getHash())
		{
			Tags->highlight(i);
			break;
		}
	}
}

namespace {//

bool isAnyModified(const NC::Menu<MPD::MutableSong> &m)
{
	for (auto it = m.beginV(); it != m.endV(); ++it)
		if (it->isModified())
			return true;
	return false;
}

std::string CapitalizeFirstLetters(const std::string &s)
{
	std::wstring ws = ToWString(s);
	wchar_t prev = 0;
	for (auto it = ws.begin(); it != ws.end(); ++it)
	{
		if (!iswalpha(prev) && prev != L'\'')
			*it = towupper(*it);
		prev = *it;
	}
	return ToString(ws);
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
			(s.*m->Set)(ToString(lowercase(ToWString(tag))), i);
	}
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
	std::string result = s.toString(pattern, Config.tags_separator);
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
					s.setTags(set, it->second, Config.tags_separator);
			}
			else
				result << "%" << it->first << ": " << it->second << "\n";
	}
	return result.str();
}

std::string SongToString(const MPD::MutableSong &s)
{
	std::string result;
	size_t i = myTagEditor->TagTypes->choice();
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

bool SongEntryMatcher(const Regex &rx, const MPD::MutableSong &s)
{
	return rx.match(SongToString(s));
}

}

#endif

