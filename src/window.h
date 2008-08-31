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

#ifndef HAVE_WINDOW_H
#define HAVE_WINDOW_H

#include "ncurses.h"

#include <stack>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>

#ifdef UTF8_ENABLED
# define TO_STRING(x) ToString(x)
#else
# define TO_STRING(x) x
#endif

typedef bool CLEAR_TO_EOL;

using std::string;
using std::wstring;
using std::vector;

enum COLOR { clDefault, clBlack, clRed, clGreen, clYellow, clBlue, clMagenta, clCyan, clWhite };
enum BORDER { brNone, brBlack, brRed, brGreen, brYellow, brBlue, brMagenta, brCyan, brWhite };
enum WHERE { UP, DOWN, PAGE_UP, PAGE_DOWN, HOME, END };

void EnableColors();

char * ToString(const wchar_t *);
wchar_t * ToWString(const char *);
string ToString(const wstring &);
wstring ToWString(const string &);

bool is_valid_color(const string &);
string OmitBBCodes(const string &);
int CountBBCodes(const string &);
int CountBBCodes(const wstring &);

class Window
{
	public:
		Window(int, int, int, int, string, COLOR, BORDER);
		virtual ~Window();
		virtual WINDOW *RawWin() { return itsWindow; }
		virtual void SetColor(COLOR, COLOR = clDefault);
		virtual void SetBaseColor(COLOR, COLOR = clDefault);
		virtual void SetBorder(BORDER);
		virtual void EnableBB() { BBEnabled = 1; }
		virtual void DisableBB() { BBEnabled = 0; }
		virtual void SetTitle(string);
		virtual void MoveTo(int, int);
		virtual void Resize(int, int);
		virtual void Display(bool = 0);
		virtual void Refresh(bool = 0);
		virtual void Clear();
		virtual void Hide(char = 32) const;
		virtual void Bold(bool) const;
		virtual void Reverse(bool) const;
		virtual void AltCharset(bool) const;
		virtual void Delay(bool) const;
		virtual void Timeout(int) const;
		virtual void AutoRefresh(bool val) { AutoRefreshEnabled = val; }
		virtual void ReadKey(int &) const;
		virtual void ReadKey() const;
		virtual void Write(const string &s, CLEAR_TO_EOL cte = 0) { Write(0xFFFF, s, cte); }
		virtual void Write(int, const string &, CLEAR_TO_EOL = 0);
		virtual void WriteXY(int x, int y, const string &s, CLEAR_TO_EOL ete = 0) { WriteXY(x, y, 0xFFFF, s, ete); }
		virtual void WriteXY(int, int, int, const string &, CLEAR_TO_EOL = 0);
#ifdef UTF8_ENABLED
		virtual void Write(const wstring &s, CLEAR_TO_EOL cte = 0) { Write(0xFFFF, s, cte); }
		virtual void Write(int, const wstring &, CLEAR_TO_EOL = 0);
		virtual void WriteXY(int x, int y, const wstring &s, CLEAR_TO_EOL ete = 0) { WriteXY(x, y, 0xFFFF, s, ete); }
		virtual void WriteXY(int, int, int, const wstring &, CLEAR_TO_EOL = 0);
#endif
		virtual string GetString(int num, void (*ptr)() = NULL) const { return GetString("", num, ptr); }
		virtual string GetString(const string &str, void (*ptr)()) const { return GetString(str, -1, ptr); }
		virtual string GetString(const string &, unsigned int = -1, void (*)() = NULL) const;
		virtual void Scrollable(bool) const;
		virtual void GetXY(int &, int &) const;
		virtual void GotoXY(int, int) const;
		virtual int GetWidth() const;
		virtual int GetHeight() const;
		virtual int GetStartX() const;
		virtual int GetStartY() const;
		virtual string GetTitle() const;
		virtual COLOR GetColor() const;
		virtual BORDER GetBorder() const;
		
		virtual Window * EmptyClone();
		
		virtual void Go(WHERE) { } // for Menu and Scrollpad class
		virtual int GetChoice() const { return -1; } // for Menu class
		virtual void Add(string str) { Write(str); } // for Scrollpad class
	protected:
		virtual bool have_to_recreate(string);
		virtual bool have_to_recreate(BORDER);
		virtual bool reallocate_win(int, int);
		virtual void recreate_win();
		virtual void show_border() const;
		virtual std::pair<COLOR, COLOR> into_color(const string &);
		//bool is_valid_color(const string &);
		WINDOW *itsWindow;
		WINDOW *itsWinBorder;
		int itsStartX;
		int itsStartY;
		int itsWidth;
		int itsHeight;
		bool BBEnabled;
		bool AutoRefreshEnabled;
		string itsTitle;
		std::stack< std::pair<COLOR, COLOR> > itsColors;
		COLOR itsColor;
		COLOR itsBaseColor;
		COLOR itsBgColor;
		COLOR itsBaseBgColor;
		BORDER itsBorder;
};

#endif
