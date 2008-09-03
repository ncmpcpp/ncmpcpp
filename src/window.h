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
# define ncmpcpp_string_t wstring
# define TO_STRING(x) ToString(x)
# define TO_WSTRING(x) ToWString(x)
#else
# define ncmpcpp_string_t string
# define TO_STRING(x) x
# define TO_WSTRING(x) x
#endif

using std::string;
using std::wstring;
using std::vector;

enum Color { clDefault, clBlack, clRed, clGreen, clYellow, clBlue, clMagenta, clCyan, clWhite };
enum Border { brNone, brBlack, brRed, brGreen, brYellow, brBlue, brMagenta, brCyan, brWhite };
enum Where { wUp, wDown, wPageUp, wPageDown, wHome, wEnd };

typedef void (*GetStringHelper)();
typedef std::pair<Color, Color> ColorPair;

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
		Window(int, int, int, int, string, Color, Border);
		Window(const Window &);
		virtual ~Window();
		virtual WINDOW *RawWin() { return itsWindow; }
		virtual void SetGetStringHelper(GetStringHelper helper) { itsGetStringHelper = helper; }
		virtual void SetColor(Color, Color = clDefault);
		virtual void SetBaseColor(Color, Color = clDefault);
		virtual void SetBorder(Border);
		virtual void EnableBB() { BBEnabled = 1; }
		virtual void DisableBB() { BBEnabled = 0; }
		virtual void SetTitle(string);
		virtual void MoveTo(int, int);
		virtual void Resize(int, int);
		virtual void Display(bool = 0);
		virtual void Refresh(bool = 0);
		virtual void Clear(bool stub = 1);
		virtual void Hide(char = 32) const;
		virtual void Bold(bool) const;
		virtual void Reverse(bool) const;
		virtual void AltCharset(bool) const;
		virtual void Delay(bool) const;
		virtual void Timeout(int) const;
		virtual void AutoRefresh(bool val) { AutoRefreshEnabled = val; }
		virtual void ReadKey(int &) const;
		virtual void ReadKey() const;
		virtual void Write(const string &s, bool cte = 0) { Write(0xFFFF, s, cte); }
		virtual void Write(int, const string &, bool = 0);
		virtual void WriteXY(int x, int y, const string &s, bool ete = 0) { WriteXY(x, y, 0xFFFF, s, ete); }
		virtual void WriteXY(int, int, int, const string &, bool = 0);
#ifdef UTF8_ENABLED
		virtual void Write(const wstring &s, bool cte = 0) { Write(0xFFFF, s, cte); }
		virtual void Write(int, const wstring &, bool = 0);
		virtual void WriteXY(int x, int y, const wstring &s, bool ete = 0) { WriteXY(x, y, 0xFFFF, s, ete); }
		virtual void WriteXY(int, int, int, const wstring &, bool = 0);
#endif
		virtual string GetString(const string &, unsigned int = -1) const;
		virtual string GetString(unsigned int length = -1) const { return GetString("", length); }
		virtual void Scrollable(bool) const;
		virtual void GetXY(int &, int &) const;
		virtual void GotoXY(int, int) const;
		virtual int GetWidth() const;
		virtual int GetHeight() const;
		virtual int GetStartX() const;
		virtual int GetStartY() const;
		virtual string GetTitle() const;
		virtual Color GetColor() const;
		virtual Border GetBorder() const;
		
		virtual Window * Clone() { return new Window(*this); }
		virtual Window * EmptyClone();
		
		// stubs for inherits, ugly shit, needs improvement
		virtual void Select(int, bool) { }
		virtual bool Selected(int) { return 0; }
		virtual int Size() const { return 0; }
		virtual bool IsAnySelected() { return 0; }
		virtual void GetSelectedList(vector<int> &) { }
		virtual bool IsStatic(int) { return 0; }
		virtual void Highlight(int) { }
		virtual void Go(Where) { } // for Menu and Scrollpad class
		virtual int GetChoice() const { return -1; } // for Menu class
		virtual void Add(string str) { Write(str); } // for Scrollpad class
		
		static void EnableColors();
		
	protected:
		virtual bool have_to_recreate(string);
		virtual bool have_to_recreate(Border);
		virtual bool reallocate_win(int, int);
		virtual void recreate_win();
		virtual void show_border() const;
		virtual ColorPair into_color(const string &);
		WINDOW *itsWindow;
		WINDOW *itsWinBorder;
		GetStringHelper itsGetStringHelper;
		int itsStartX;
		int itsStartY;
		int itsWidth;
		int itsHeight;
		bool BBEnabled;
		bool AutoRefreshEnabled;
		string itsTitle;
		std::stack<ColorPair> itsColors;
		Color itsColor;
		Color itsBaseColor;
		Color itsBgColor;
		Color itsBaseBgColor;
		Border itsBorder;
};

#endif
