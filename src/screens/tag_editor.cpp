/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#include "screens/tag_editor.h"

#ifdef HAVE_TAGLIB_H

#include <boost/locale/conversion.hpp>
#include <algorithm>
#include <fstream>

#include "actions.h"
#include "screens/browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "format_impl.h"
#include "curses/menu_impl.h"
#include "screens/playlist.h"
#include "screens/song_info.h"
#include "statusbar.h"
#include "helpers/song_iterator_maker.h"
#include "utility/functional.h"
#include "utility/comparators.h"
#include "title.h"
#include "tags.h"
#include "screens/screen_switcher.h"

using Global::myScreen;
using Global::MainHeight;
using Global::MainStartY;

namespace ph = std::placeholders;

TagEditor *myTagEditor;

namespace {

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
bool DirEntryMatcher(const Regex::Regex &rx, const std::pair<std::string, std::string> &dir, bool filter);
bool SongEntryMatcher(const Regex::Regex &rx, const MPD::MutableSong &s);

}

SongIterator TagsWindow::currentS()
{
	return makeSongIterator(current());
}

ConstSongIterator TagsWindow::currentS() const
{
	return makeConstSongIterator(current());
}

SongIterator TagsWindow::beginS()
{
	return makeSongIterator(begin());
}

ConstSongIterator TagsWindow::beginS() const
{
	return makeConstSongIterator(begin());
}

SongIterator TagsWindow::endS()
{
	return makeSongIterator(end());
}

ConstSongIterator TagsWindow::endS() const
{
	return makeConstSongIterator(end());
}

std::vector<MPD::Song> TagsWindow::getSelectedSongs()
{
	return {}; // TODO
}

/**********************************************************************/

TagEditor::TagEditor() : FParser(0), FParserHelper(0), FParserLegend(0), FParserPreview(0), itsBrowsedDir("/")
{
	PatternsFile = Config.ncmpcpp_directory + "patterns.list";
	SetDimensions(0, COLS);
	
	Dirs = new NC::Menu< std::pair<std::string, std::string> >(0, MainStartY, LeftColumnWidth, MainHeight, Config.titles_visibility ? "Directories" : "", Config.main_color, NC::Border());
	setHighlightFixes(*Dirs);
	Dirs->cyclicScrolling(Config.use_cyclic_scrolling);
	Dirs->centeredCursor(Config.centered_cursor);
	Dirs->setItemDisplayer([](NC::Menu<std::pair<std::string, std::string>> &menu) {
		menu << Charset::utf8ToLocale(menu.drawn()->value().first);
	});
	
	TagTypes = new NC::Menu<std::string>(MiddleColumnStartX, MainStartY, MiddleColumnWidth, MainHeight, Config.titles_visibility ? "Tag types" : "", Config.main_color, NC::Border());
	setHighlightInactiveColumnFixes(*TagTypes);
	TagTypes->cyclicScrolling(Config.use_cyclic_scrolling);
	TagTypes->centeredCursor(Config.centered_cursor);
	TagTypes->setItemDisplayer([](NC::Menu<std::string> &menu) {
		menu << Charset::utf8ToLocale(menu.drawn()->value());
	});
	
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m)
		TagTypes->addItem(m->Name);
	TagTypes->addSeparator();
	TagTypes->addItem("Filename");
	TagTypes->addSeparator();
	if (Config.titles_visibility)
	{
		TagTypes->addItem("Options", NC::List::Properties::Inactive);
		TagTypes->addSeparator();
	}
	TagTypes->addItem("Capitalize First Letters");
	TagTypes->addItem("lower all letters");
	TagTypes->addSeparator();
	TagTypes->addItem("Reset");
	TagTypes->addItem("Save");
	
	Tags = new TagsWindow(NC::Menu<MPD::MutableSong>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Tags" : "", Config.main_color, NC::Border()));
	setHighlightInactiveColumnFixes(*Tags);
	Tags->cyclicScrolling(Config.use_cyclic_scrolling);
	Tags->centeredCursor(Config.centered_cursor);
	Tags->setSelectedPrefix(Config.selected_item_prefix);
	Tags->setSelectedSuffix(Config.selected_item_suffix);
	Tags->setItemDisplayer(Display::Tags);
	
	auto parser_display = [](NC::Menu<std::string> &menu) {
		menu << Charset::utf8ToLocale(menu.drawn()->value());
	};
	
	FParserDialog = new NC::Menu<std::string>((COLS-FParserDialogWidth)/2, (MainHeight-FParserDialogHeight)/2+MainStartY, FParserDialogWidth, FParserDialogHeight, "", Config.main_color, Config.window_border);
	FParserDialog->cyclicScrolling(Config.use_cyclic_scrolling);
	FParserDialog->centeredCursor(Config.centered_cursor);
	FParserDialog->setItemDisplayer(parser_display);
	FParserDialog->addItem("Get tags from filename");
	FParserDialog->addItem("Rename files");
	FParserDialog->addItem("Cancel");
	
	FParser = new NC::Menu<std::string>((COLS-FParserWidth)/2, (MainHeight-FParserHeight)/2+MainStartY, FParserWidthOne, FParserHeight, "_", Config.main_color, Config.active_window_border);
	FParser->cyclicScrolling(Config.use_cyclic_scrolling);
	FParser->centeredCursor(Config.centered_cursor);
	FParser->setItemDisplayer(parser_display);
	
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
	SwitchTo::execute(this);
	drawHeader();
	refresh();
}

void TagEditor::refresh()
{
	Dirs->display();
	drawSeparator(MiddleColumnStartX-1);
	TagTypes->display();
	drawSeparator(RightColumnStartX-1);
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
	if (Dirs->empty())
	{
		Dirs->Window::clear();
		Tags->clear();
		
		if (itsBrowsedDir != "/")
			Dirs->addItem(std::make_pair("..", getParentDirectory(itsBrowsedDir)));
		else
			Dirs->addItem(std::make_pair(".", "/"));
		MPD::DirectoryIterator directory = Mpd.GetDirectories(itsBrowsedDir), end;
		for (; directory != end; ++directory)
		{
			Dirs->addItem(std::make_pair(getBasename(directory->path()), directory->path()));
			if (directory->path() == itsHighlightedDir)
				Dirs->highlight(Dirs->size()-1);
		};
		std::sort(Dirs->beginV()+1, Dirs->endV(),
			LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
		Dirs->display();
	}
	
	if (Tags->empty())
	{
		Tags->reset();
		MPD::SongIterator s = Mpd.GetSongs(Dirs->current()->value().second), end;
		for (; s != end; ++s)
			Tags->addItem(std::move(*s));
		std::sort(Tags->beginV(), Tags->endV(),
			LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
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

bool TagEditor::enterDirectory()
{
	bool result = false;
	if (w == Dirs && !Dirs->empty())
	{
		MPD::DirectoryIterator directory = Mpd.GetDirectories(Dirs->current()->value().second), end;
		bool has_subdirs = directory != end;
		if (has_subdirs)
		{
			directory.finish();
			itsHighlightedDir = itsBrowsedDir;
			itsBrowsedDir = Dirs->current()->value().second;
			Dirs->clear();
			Dirs->reset();
			result = true;
		}
	}
	return result;
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
					runAction();
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
					runAction();
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
				enterDirectory();
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
				runAction();
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
				runAction();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/***********************************************************************/

bool TagEditor::allowsSearching()
{
	return w == Dirs || w == Tags;
}

const std::string &TagEditor::searchConstraint()
{
	if (w == Dirs)
		return m_directories_search_predicate.constraint();
	else if (w == Tags)
		return m_songs_search_predicate.constraint();
	throw std::runtime_error("shouldn't happen due to condition in allowsSearching");
}

void TagEditor::setSearchConstraint(const std::string &constraint)
{
	if (w == Dirs)
	{
		m_directories_search_predicate = Regex::Filter<std::pair<std::string, std::string>>(
			constraint,
			Config.regex_type,
			std::bind(DirEntryMatcher, ph::_1, ph::_2, false));
	}
	else if (w == Tags)
	{
		m_songs_search_predicate = Regex::Filter<MPD::MutableSong>(
			constraint,
			Config.regex_type,
			SongEntryMatcher);
	}
}

void TagEditor::clearSearchConstraint()
{
	if (w == Dirs)
		m_directories_search_predicate.clear();
	else if (w == Tags)
		m_songs_search_predicate.clear();
}

bool TagEditor::search(SearchDirection direction, bool wrap, bool skip_current)
{
	bool result = false;
	if (w == Dirs)
		result = ::search(*Dirs, m_directories_search_predicate, direction, wrap, skip_current);
	else if (w == Tags)
		result = ::search(*Tags, m_songs_search_predicate, direction, wrap, skip_current);
	return result;
}

/***********************************************************************/

bool TagEditor::actionRunnable()
{
	// TODO: put something more refined here. It requires reworking
	// runAction though, i.e. splitting it into smaller parts.
	return (w == Tags && !Tags->empty())
		|| w != Tags;
}

void TagEditor::runAction()
{
	using Global::wFooter;

	if (w == FParserDialog)
	{
		size_t choice = FParserDialog->choice();
		if (choice == 2) // cancel
		{
			w = TagTypes;
			refresh();
			return;
		}
		GetPatternList();

		// prepare additional windows

		FParserLegend->clear();
		*FParserLegend << "%a - artist\n";
		*FParserLegend << "%A - album artist\n";
		*FParserLegend << "%t - title\n";
		*FParserLegend << "%b - album\n";
		*FParserLegend << "%y - date\n";
		*FParserLegend << "%n - track number\n";
		*FParserLegend << "%g - genre\n";
		*FParserLegend << "%c - composer\n";
		*FParserLegend << "%p - performer\n";
		*FParserLegend << "%d - disc\n";
		*FParserLegend << "%C - comment\n\n";
		*FParserLegend << NC::Format::Bold << "Files:\n" << NC::Format::NoBold;
		for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			*FParserLegend << Config.color2
			               << " * "
			               << NC::FormattedColor::End<>(Config.color2)
			               << (*it)->getName()
			               << "\n";
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
			FParser->addItem("Recent patterns", NC::List::Properties::Inactive);
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
			std::string new_pattern;
			{
				Statusbar::ScopedLock slock;
				Statusbar::put() << "Pattern: ";
				new_pattern = wFooter->prompt(Config.pattern);
			}
			Config.pattern = new_pattern;
			FParser->at(0).value() = "Pattern: ";
			FParser->at(0).value() += Config.pattern;
		}
		else if (pos == 1 || pos == 4) // preview or proceed
		{
			bool success = 1;
			Statusbar::print("Parsing...");
			FParserPreview->clear();
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			{
				MPD::MutableSong &s = **it;
				if (FParserDialog->choice() == 0) // get tags from filename
				{
					if (FParserUsePreview)
					{
						*FParserPreview << NC::Format::Bold << s.getName() << ":\n" << NC::Format::NoBold;
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
						Statusbar::printf("File \"%1%\" would have an empty name", s.getName());
						FParserUsePreview = 1;
						success = 0;
					}
					if (!FParserUsePreview)
						s.setNewName(new_file + extension);
					*FParserPreview << file
					                << Config.color2
					                << " -> "
					                << NC::FormattedColor::End<>(Config.color2);
					if (new_file.empty())
						*FParserPreview << Config.empty_tags_color
						                << Config.empty_tag
						                << NC::FormattedColor::End<>(Config.empty_tags_color);
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
				Statusbar::print("Operation finished");
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
			Config.pattern = FParser->current()->value();
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
	// if there are selected songs, perform operations only on them
	if (hasSelected(Tags->begin(), Tags->end()))
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
		Actions::confirmAction("Number tracks?");
		auto it = EditedSongs.begin();
		for (unsigned i = 1; i <= EditedSongs.size(); ++i, ++it)
		{
			if (Config.tag_editor_extended_numeration)
				(*it)->setTrack(boost::lexical_cast<std::string>(i) + "/" + boost::lexical_cast<std::string>(EditedSongs.size()));
			else
				(*it)->setTrack(boost::lexical_cast<std::string>(i));
			// discard other track number tags
			(*it)->setTrack("", 1);
		}
		Statusbar::print("Tracks numbered");
		return;
	}

	if (id < 11)
	{
		MPD::Song::GetFunction get = SongInfo::Tags[id].Get;
		MPD::MutableSong::SetFunction set = SongInfo::Tags[id].Set;
		if (id > 0 && w == TagTypes)
		{
			Statusbar::ScopedLock slock;
			Statusbar::put() << NC::Format::Bold << TagTypes->current()->value() << NC::Format::NoBold << ": ";
			std::string new_tag = wFooter->prompt(Tags->current()->value().getTags(get));
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				(*it)->setTags(set, new_tag);
		}
		else if (w == Tags)
		{
			Statusbar::ScopedLock slock;
			Statusbar::put() << NC::Format::Bold << TagTypes->current()->value() << NC::Format::NoBold << ": ";
			std::string new_tag = wFooter->prompt(Tags->current()->value().getTags(get));
			if (new_tag != Tags->current()->value().getTags(get))
				Tags->current()->value().setTags(set, new_tag);
			Tags->scroll(NC::Scroll::Down);
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
				Statusbar::ScopedLock slock;
				MPD::MutableSong &s = Tags->current()->value();
				std::string old_name = s.getNewName().empty() ? s.getName() : s.getNewName();
				size_t last_dot = old_name.rfind(".");
				std::string extension = old_name.substr(last_dot);
				old_name = old_name.substr(0, last_dot);
				Statusbar::put() << NC::Format::Bold << "New filename: " << NC::Format::NoBold;
				std::string new_name = wFooter->prompt(old_name);
				if (!new_name.empty())
					s.setNewName(new_name + extension);
				Tags->scroll(NC::Scroll::Down);
			}
		}
		else if (id == TagTypes->size()-5) // capitalize first letters
		{
			Statusbar::print("Processing...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				CapitalizeFirstLetters(**it);
			Statusbar::print("Done");
		}
		else if (id == TagTypes->size()-4) // lower all letters
		{
			Statusbar::print("Processing...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
				LowerAllLetters(**it);
			Statusbar::print("Done");
		}
		else if (id == TagTypes->size()-2) // reset
		{
			for (auto it = Tags->beginV(); it != Tags->endV(); ++it)
				it->clearModifications();
			Statusbar::print("Changes reset");
		}
		else if (id == TagTypes->size()-1) // save
		{
			bool success = 1;
			Statusbar::print("Writing changes...");
			for (auto it = EditedSongs.begin(); it != EditedSongs.end(); ++it)
			{
				Statusbar::printf("Writing tags in \"%1%\"...", (*it)->getName());
				if (!Tags::write(**it))
				{
					Statusbar::printf("Error while writing tags to \"%1%\": %2%",
					                  (*it)->getName(), strerror(errno));
					success = 0;
					break;
				}
			}
			if (success)
			{
				Statusbar::print("Tags updated");
				setHighlightInactiveColumnFixes(*TagTypes);
				TagTypes->reset();
				w->refresh();
				w = Dirs;
				setHighlightFixes(*Dirs);
				Mpd.UpdateDirectory(getSharedDirectory(Tags->beginV(), Tags->endV()));
			}
			else
				Tags->clear();
		}
	}
}


/***********************************************************************/

bool TagEditor::itemAvailable()
{
	if (w == Tags)
		return !Tags->empty();
	return false;
}

bool TagEditor::addItemToPlaylist(bool play)
{
	return addSongToPlaylist(*Tags->currentV(), play);
}

std::vector<MPD::Song> TagEditor::getSelectedSongs()
{
	std::vector<MPD::Song> result;
	if (w == Tags)
	{
		for (auto it = Tags->begin(); it != Tags->end(); ++it)
			if (it->isSelected())
				result.push_back(it->value());
		// if no song was selected, add current one
		if (result.empty() && !Tags->empty())
			result.push_back(Tags->current()->value());
	}
	return result;
}

/***********************************************************************/

bool TagEditor::previousColumnAvailable()
{
	bool result = false;
	if (w == Tags)
	{
		if (!TagTypes->empty() && !Dirs->empty())
			result = true;
	}
	else if (w == TagTypes)
	{
		if (!Dirs->empty() && isAnyModified(*Tags))
			Actions::confirmAction("There are pending changes, are you sure?");
		result = true;
	}
	else if (w == FParserHelper)
		result = true;
	return result;
}

void TagEditor::previousColumn()
{
	if (w == Tags)
	{
		setHighlightInactiveColumnFixes(*Tags);
		w->refresh();
		w = TagTypes;
		setHighlightFixes(*TagTypes);
	}
	else if (w == TagTypes)
	{
		setHighlightInactiveColumnFixes(*TagTypes);
		w->refresh();
		w = Dirs;
		setHighlightFixes(*Dirs);
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
		if (!TagTypes->empty() && !Tags->empty())
			result = true;
	}
	else if (w == TagTypes)
	{
		if (!Tags->empty())
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
		setHighlightInactiveColumnFixes(*Dirs);
		w->refresh();
		w = TagTypes;
		setHighlightFixes(*TagTypes);
	}
	else if (w == TagTypes && TagTypes->choice() < 13 && !Tags->empty())
	{
		setHighlightInactiveColumnFixes(*TagTypes);
		w->refresh();
		w = Tags;
		setHighlightFixes(*Tags);
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
		if ((*Tags)[i].value() == s)
		{
			Tags->highlight(i);
			break;
		}
	}
}

namespace {

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
			(s.*m->Set)(boost::locale::to_lower(tag), i);
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
			while (std::getline(input, line))
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
	std::string result = Format::stringify<char>(Format::parse(pattern), &s);
	removeInvalidCharsFromFilename(result, Config.generate_win32_compatible_filenames);
	return result;
}

std::string ParseFilename(MPD::MutableSong &s, std::string mask, bool preview)
{
	std::ostringstream result;
	std::vector<std::string> separators;
	std::vector< std::pair<char, std::string> > tags;
	std::string file = s.getName().substr(0, s.getName().rfind("."));
	
	size_t i = mask.find("%");

	if (!mask.substr(0, i).empty())
		file = file.substr(i);

	for (; i != std::string::npos; i = mask.find("%"))
	{
		tags.push_back(std::make_pair(mask.at(i+1), ""));
		mask = mask.substr(i+2);
		i = mask.find("%");
		if (!mask.empty())
			separators.push_back(mask.substr(0, i));
	}
	i = 0;
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
					s.setTags(set, it->second);
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
		result = s.getNewName().empty() ? s.getName() : s.getName() + " -> " + s.getNewName();
	return result.empty() ? Config.empty_tag : result;
}

bool DirEntryMatcher(const Regex::Regex &rx, const std::pair<std::string, std::string> &dir, bool filter)
{
	if (dir.first == "." || dir.first == "..")
		return filter;
	return Regex::search(dir.first, rx, Config.ignore_diacritics);
}

bool SongEntryMatcher(const Regex::Regex &rx, const MPD::MutableSong &s)
{
	return Regex::search(SongToString(s), rx, Config.ignore_diacritics);
}

}

#endif

