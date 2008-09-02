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

#ifndef HAVE_MENU_H
#define HAVE_MENU_H

#include "window.h"

#include <stdexcept>

enum Location { lLeft, lCenter, lRight };

struct Option
{
	Option() : is_static(0), is_bold(0), selected(0), have_separator(0) { }
	string content;
	bool is_static;
	bool is_bold;
	bool selected;
	bool have_separator;
	Location location;
};

class Menu : public Window
{
	public:
		Menu(int startx, int starty, int width, int height, string title, Color color, Border border) : Window(startx, starty, width, height, title, color, border), itsSelectedPrefix("[r]"), itsSelectedSuffix("[/r]"), itsStaticsNumber(0), itsBeginning(0), itsHighlight(0), itsHighlightColor(itsBaseColor), itsHighlightEnabled(1) { SetColor(color); }
		Menu(const Menu &);
		virtual ~Menu();
		
		virtual void Add(string str) { AddOption(str); }
		void AddOption(const string &, Location = lLeft, bool separator = 0);
		void AddBoldOption(const string &str, Location location = lLeft, bool separator = 0);
		void AddStaticOption(const string &str, Location location = lLeft, bool separator = 0);
		void AddStaticBoldOption(const string &str, Location location = lLeft, bool separator = 0);
		void AddSeparator();
		void UpdateOption(int, string, Location = lLeft, bool separator = 0);
		void BoldOption(int, bool);
		void MakeStatic(int, bool);
		void DeleteOption(int);
		void Swap(int, int);
		string GetCurrentOption() const;
		string GetOption(int i) const;
		
		virtual void Display(bool redraw_whole_window = 0);
		virtual void Refresh(bool redraw_whole_window = 0);
		virtual void Go(Where);
		void Highlight(int);
		virtual void Reset();
		virtual void Clear(bool clear_screen = 1);
		
		void Select(int, bool);
		bool Selected(int);
		bool IsAnySelected();
		void SetSelectPrefix(string str) { itsSelectedPrefix = str; }
		void SetSelectSuffix(string str) { itsSelectedSuffix = str; }
		void GetSelectedList(vector<int> &);
		
		void HighlightColor(Color col) { itsHighlightColor = col; NeedsRedraw.push_back(itsHighlight); }
		void Highlighting(bool hl) { itsHighlightEnabled = hl; NeedsRedraw.push_back(itsHighlight); Refresh(); }
		
		int GetRealChoice() const;
		virtual int GetChoice() const { return itsHighlight+1; }
		
		int MaxChoice() const { return itsOptions.size(); }
		int MaxRealChoice() const { return itsOptions.size()-itsStaticsNumber; }
		
		bool Empty() { return itsOptions.empty(); }
		bool IsStatic(int);
		virtual Window * Clone() { return new Menu(*this); }
		virtual Window * EmptyClone();
	protected:
		vector<Option *> itsOptions;
		vector<int> NeedsRedraw;
		
		string itsSelectedPrefix;
		string itsSelectedSuffix;
		
		int itsStaticsNumber;
		int count_length(string);
		
		void redraw_screen();
		bool is_static() { return itsOptions[itsHighlight]->is_static; }
		
		int itsBeginning;
		int itsHighlight;
		
		Color itsHighlightColor;
		
		bool itsHighlightEnabled;
};

#endif

