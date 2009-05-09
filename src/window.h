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

#ifndef _WINDOW_H
#define _WINDOW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "curses.h"

#include <deque>
#include <stack>
#include <vector>
#include <string>

#if defined(__GNUC__) && __GNUC__ >= 3
# define GNUC_UNUSED __attribute__((unused))
# define GNUC_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
# define GNUC_UNUSED
# define GNUC_PRINTF(a, b)
#endif

#ifdef USE_PDCURSES
# undef KEY_BACKSPACE
# define KEY_BACKSPACE 8
#endif // USE_PDCURSES

#ifdef _UTF8
# define my_char_t wchar_t
# define TO_STRING(x) ToString(x)
# define TO_WSTRING(x) ToWString(x)
#else
# define my_char_t char
# define TO_STRING(x) (x)
# define TO_WSTRING(x) (x)
#endif

std::string ToString(const std::wstring &);
std::wstring ToWString(const std::string &);

namespace NCurses
{
	enum Color { clDefault, clBlack, clRed, clGreen, clYellow, clBlue, clMagenta, clCyan, clWhite, clEnd };
	enum Format { fmtNone = 100, fmtBold, fmtBoldEnd, fmtReverse, fmtReverseEnd, fmtAltCharset, fmtAltCharsetEnd };
	enum Border { brNone, brBlack, brRed, brGreen, brYellow, brBlue, brMagenta, brCyan, brWhite };
	enum Where { wUp, wDown, wPageUp, wPageDown, wHome, wEnd };
	
	typedef void (*GetStringHelper)(const std::wstring &);
	
	void InitScreen(const char *, bool);
	void DestroyScreen();
	
	struct Colors
	{
		Colors(Color one, Color two = clDefault) : fg(one), bg(two) { }
		Color fg;
		Color bg;
	};
	
	struct XY
	{
		XY(int xx, int yy) : x(xx), y(yy) { }
		int x;
		int y;
	};
	
	class Window
	{
		public:
			Window(size_t, size_t, size_t, size_t, const std::string &, Color, Border);
			Window(const Window &);
			virtual ~Window();
			
			WINDOW *Raw() const { return itsWindow; }
			
			size_t GetWidth() const;
			size_t GetHeight() const;
			size_t GetStartX() const;
			size_t GetStartY() const;
			
			const std::string &GetTitle() const;
			Color GetColor() const;
			Border GetBorder() const;
			std::string GetString(const std::string &, size_t = -1, size_t = 0, bool = 0) const;
			std::string GetString(size_t length = -1, size_t width = 0, bool encrypted = 0) const { return GetString("", length, width, encrypted); }
			void GetXY(int &, int &);
			void GotoXY(int, int);
			int X() const;
			int Y() const;
			
			void SetGetStringHelper(GetStringHelper helper) { itsGetStringHelper = helper; }
			void SetColor(Color, Color = clDefault);
			void SetBaseColor(Color, Color = clDefault);
			void SetBorder(Border);
			void SetTimeout(int);
			void SetTitle(const std::string &);
			void CreateHistory();
			void DeleteHistory();
			
			void Hide(char = 32) const;
			void Bold(bool) const;
			void Reverse(bool) const;
			void AltCharset(bool) const;
			
			void Display();
			virtual void Refresh();
			
			virtual void MoveTo(size_t, size_t);
			virtual void Resize(size_t, size_t);
			virtual void Clear(bool = 1);
			
			void ReadKey(int &) const;
			void ReadKey() const;
			
			//void Write(bool, const char *, ...) const;
			//void WriteXY(int, int, bool, const char *, ...) const;
			
			void Scrollable(bool) const;
			virtual void Scroll(Where);
			
			Window &operator<<(int (*)(WINDOW *));
			Window &operator<<(Colors);
			Window &operator<<(Color);
			Window &operator<<(Format);
			Window &operator<<(XY);
			Window &operator<<(const char *);
			Window &operator<<(char);
			Window &operator<<(const wchar_t *);
			Window &operator<<(wchar_t);
			Window &operator<<(int);
			Window &operator<<(double);
			Window &operator<<(size_t);
			
			Window &operator<<(const std::string &);
			Window &operator<<(const std::wstring &);
			
			virtual Window *Clone() const { return new Window(*this); }
			virtual Window *EmptyClone() const;
			
			static size_t Length(const std::wstring &);
			
		protected:
			
			class BadSize { };
			
			void ShowBorder() const;
			void AdjustDimensions(size_t &, size_t &);
			
			virtual void Recreate();
			
			WINDOW *itsWindow;
			WINDOW *itsWinBorder;
			
			GetStringHelper itsGetStringHelper;
			
			size_t itsStartX;
			size_t itsStartY;
			size_t itsWidth;
			size_t itsHeight;
			
			int itsWindowTimeout;
			int itsX;
			int itsY;
			
			std::string itsTitle;
			std::stack<Colors> itsColors;
			
			Color itsColor;
			Color itsBaseColor;
			Color itsBgColor;
			Color itsBaseBgColor;
			
			Border itsBorder;
			
		private:
			std::deque<std::wstring> *itsHistory;
	};
}

#endif
