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

#include "window.h"

Window::Window(int startx, int starty, int width, int height, const string &title, Color color, Border border) :
	itsWindow(0),
	itsWinBorder(0),
	itsGetStringHelper(0),
	itsStartX(startx),
	itsStartY(starty),
	itsWidth(width),
	itsHeight(height),
	itsWindowTimeout(-1),
	BBEnabled(1),
	AutoRefreshEnabled(1),
	itsTitle(title),
	itsColor(color),
	itsBaseColor(color),
	itsBgColor(clDefault),
	itsBaseBgColor(clDefault),
	itsBorder(border)
{
	if (itsStartX < 0) itsStartX = 0;
	if (itsStartY < 0) itsStartY = 0;
	
	if (itsBorder != brNone)
	{
		itsWinBorder = newpad(itsHeight, itsWidth);
		wattron(itsWinBorder, COLOR_PAIR(itsBorder));
		box(itsWinBorder, 0, 0);
		itsStartX++;
		itsStartY++;
		itsWidth -= 2;
		itsHeight -= 2;
	}
	if (!itsTitle.empty())
	{
		itsStartY += 2;
		itsHeight -= 2;
	}
	
	if (itsWidth > 0 && itsHeight > 0)
		itsWindow = newwin(itsHeight, itsWidth, itsStartY, itsStartX);
	else
		itsWindow = newwin(0, 0, 0, 0);
	
	SetColor(itsColor);
	keypad(itsWindow, 1);
}

Window::Window(const Window &w)
{
	itsWindow = dupwin(w.itsWindow);
	itsWinBorder = dupwin(w.itsWinBorder);
	itsGetStringHelper = w.itsGetStringHelper;
	itsStartX = w.itsStartX;
	itsStartY = w.itsStartY;
	itsWidth = w.itsWidth;
	itsHeight = w.itsHeight;
	BBEnabled = w.BBEnabled;
	AutoRefreshEnabled = w.AutoRefreshEnabled;
	itsTitle = w.itsTitle;
	itsColors = w.itsColors;
	itsColor = w.itsColor;
	itsBaseColor = w.itsBaseColor;
	itsBgColor = w.itsBgColor;
	itsBaseBgColor = w.itsBaseBgColor;
	itsBorder = w.itsBorder;
}

Window::~Window()
{
	delwin(itsWindow);
	delwin(itsWinBorder);
}

void Window::SetColor(Color col, Color background)
{
	if (col != clDefault)
		wattron(itsWindow, COLOR_PAIR(background*8+col));
	else
		wattroff(itsWindow, COLOR_PAIR(itsColor));
	itsColor = col;
	itsBgColor = background;
}

void Window::SetBaseColor(Color col, Color background)
{
	itsBaseColor = col;
	itsBaseBgColor = background;
}

void Window::SetBorder(Border border)
{
	if (border == brNone && itsBorder != brNone)
	{
		delwin(itsWinBorder);
		itsStartX--;
		itsStartY--;
		itsHeight += 2;
		itsWidth += 2;
		Recreate();
	}
	else if (border != brNone && itsBorder == brNone)
	{
		itsWinBorder = newpad(itsHeight, itsWidth);
		wattron(itsWinBorder, COLOR_PAIR(border));
		box(itsWinBorder,0,0);
		itsStartX++;
		itsStartY++;
		itsHeight -= 2;
		itsWidth -= 2;
		Recreate();
	}
	else
	{
		wattron(itsWinBorder,COLOR_PAIR(border));
		box(itsWinBorder, 0, 0);
	}
	itsBorder = border;
}

void Window::SetTitle(const string &newtitle)
{
	if (itsTitle == newtitle)
	{
		return;
	}
	else if (!newtitle.empty() && itsTitle.empty())
	{
		itsStartY += 2;
		itsHeight -= 2;
		Recreate();
	}
	else if (newtitle.empty() && !itsTitle.empty())
	{
		itsStartY -= 2;
		itsHeight += 2;
		Recreate();
	}
	itsTitle = newtitle;
}

void Window::Recreate()
{
	delwin(itsWindow);
	itsWindow = newwin(itsHeight, itsWidth, itsStartY, itsStartX);
	SetTimeout(itsWindowTimeout);
	SetColor(itsColor, itsBgColor);
	keypad(itsWindow, 1);
}

void Window::MoveTo(int newx, int newy)
{
	if (newx < 0 || newy < 0 || (newx == itsStartX && newy == itsStartY))
		return;
	else
	{
		itsStartX = newx;
		itsStartY = newy;
		if (itsBorder != brNone)
		{
			itsStartX++;
			itsStartY++;
		}
		if (!itsTitle.empty())
			itsStartY += 2;
		mvwin(itsWindow, itsStartY, itsStartX);
	}
}

void Window::Resize(int width, int height)
{
	if (itsBorder != brNone)
	{
		delwin(itsWinBorder);
		itsWinBorder = newpad(height, width);
		wattron(itsWinBorder, COLOR_PAIR(itsBorder));
		box(itsWinBorder, 0, 0);
		width -= 2;
		height -= 2;
	}
	if (!itsTitle.empty())
		height -= 2;
	
	if (height > 0 && width > 0 && wresize(itsWindow, height, width) == OK)
	{
		itsHeight = height;
		itsWidth = width;
	}
}

void Window::ShowBorder() const
{
	if (itsBorder != brNone)
	{
		refresh();
		prefresh(itsWinBorder, 0, 0, GetStartY(), GetStartX(), itsStartY+itsHeight, itsStartX+itsWidth);
	}
	if (!itsTitle.empty())
	{
		if (itsBorder != brNone)
			attron(COLOR_PAIR(itsBorder));
		else
			attron(COLOR_PAIR(itsBaseColor));
		mvhline(itsStartY-1, itsStartX, 0, itsWidth);
		attron(A_BOLD);
		mvhline(itsStartY-2, itsStartX, 32, itsWidth); // clear title line
		mvaddstr(itsStartY-2, itsStartX, itsTitle.c_str());
		attroff(COLOR_PAIR(itsBorder) | A_BOLD);
	}
	refresh();
}

void Window::Display(bool stub)
{
	ShowBorder();
	Refresh(stub);
}

void Window::Refresh(bool)
{
	wrefresh(itsWindow);
}

void Window::Clear(bool)
{
	werase(itsWindow);
	wrefresh(itsWindow);
}

void Window::Hide(char x) const
{
	for (int i = 0; i < GetHeight(); i++)
		mvhline(i+GetStartY(), GetStartX(), x, GetWidth());
	refresh();
}

void Window::Bold(bool bold) const
{
	bold ? wattron(itsWindow, A_BOLD) : wattroff(itsWindow, A_BOLD);
}

void Window::Reverse(bool reverse) const
{
	reverse ? wattron(itsWindow, A_REVERSE) : wattroff(itsWindow, A_REVERSE);
}

void Window::AltCharset(bool alt) const
{
	alt ? wattron(itsWindow, A_ALTCHARSET) : wattroff(itsWindow, A_ALTCHARSET);
}

void Window::Delay(bool delay) const
{
	nodelay(itsWindow, !delay);
}

void Window::SetTimeout(int timeout)
{
	itsWindowTimeout = timeout;
	wtimeout(itsWindow, timeout);
}

void Window::ReadKey(int &input) const
{
	input = wgetch(itsWindow);
}

void Window::ReadKey() const
{
	wgetch(itsWindow);
}

void Window::Write(int limit, const string &str, bool clrtoeol)
{
	if (BBEnabled)
	{
		bool collect = false;
		string color, tmp;
		for (string::const_iterator it = str.begin(); it != str.end() && limit > 0; it++)
		{
			if (*it == '[' && (*(it+1) == '.' || *(it+1) == '/'))
				collect = 1;
			
			if (!collect)
			{
				tmp += *it;
				limit--;
			}
			else
			{
				if (*it != '[')
				{
					color += *it;
					if (color.length() > 10) collect = 0; // longest bbcode is 10 chars long
				}
				else
				{
					limit -= color.length();
					tmp += color;
					color = *it;
				}
			}
		
			if (*it == ']' || it+1 == str.end())
				collect = 0;
			
			if (!collect && !color.empty())
			{
				waddstr(itsWindow,tmp.c_str());
				tmp.clear();
				
				if (isdigit(color[2]))
				{
					int x, y;
					getyx(itsWindow, y, x);
					Coordinates coords = IntoCoordinates(color);
					wmove(itsWindow, coords.second == -1 ? y : coords.second, coords.first);
					limit -= coords.first-x;
				}
				else if (IsValidColor(color))
				{
					ColorPair colors = IntoColor(color);
					SetColor(colors.first, colors.second);
				}
				else
				{
					limit -= color.length();
					tmp += limit > 0 ? color : color.substr(0, color.length()+limit);
				}
				color.clear();
			}
		}
		if (!tmp.empty()) waddstr(itsWindow,tmp.c_str());
	}
	else
		waddstr(itsWindow,str.c_str());
	
	if (clrtoeol)
		wclrtoeol(itsWindow);
	if (AutoRefreshEnabled)
		wrefresh(itsWindow);
}

#ifdef UTF8_ENABLED
void Window::Write(int limit, const wstring &str, bool clrtoeol)
{
	if (BBEnabled)
	{
		bool collect = false;
		wstring color, tmp;
		for (wstring::const_iterator it = str.begin(); it != str.end() && limit > 0; it++)
		{
			if (*it == '[' && (*(it+1) == '.' || *(it+1) == '/'))
				collect = 1;
			
			if (!collect)
			{
				tmp += *it;
				limit -= wcwidth(*it);
			}
			else
			{
				if (*it != '[')
				{
					color += *it;
					if (color.length() > 10) collect = 0; // longest bbcode is 10 chars long
				}
				else
				{
					limit -= Length(color);
					tmp += color;
					color = *it;
				}
			}
			
			if (*it == ']' || it+1 == str.end())
				collect = 0;
			
			if (!collect && !color.empty())
			{
				wprintw(itsWindow, "%ls", tmp.c_str());
				tmp.clear();
				
				if (isdigit(color[2]))
				{
					int x, y;
					getyx(itsWindow, y, x);
					Coordinates coords = IntoCoordinates(ToString(color));
					wmove(itsWindow, coords.second < 0 ? y : coords.second, coords.first);
					limit -= coords.first-x;
				}
				else if (IsValidColor(ToString(color)))
				{
					ColorPair colors = IntoColor(ToString(color));
					SetColor(colors.first, colors.second);
				}
				else
				{
					limit -= Length(color);
					tmp += limit > 0 ? color : color.substr(0, color.length()+limit);
				}
				color.clear();
			}
		}
		if (!tmp.empty()) wprintw(itsWindow, "%ls", tmp.c_str());
	}
	else
		wprintw(itsWindow, "%ls", str.c_str());
	
	if (clrtoeol)
		wclrtoeol(itsWindow);
	if (AutoRefreshEnabled)
		wrefresh(itsWindow);
}

void Window::WriteXY(int x, int y, int limit, const wstring &str, bool cleartoeol)
{
	wmove(itsWindow, y, x);
	Write(limit, str, cleartoeol);
}
#endif

void Window::WriteXY(int x, int y, int limit, const string &str, bool cleartoeol)
{
	wmove(itsWindow, y, x);
	Write(limit, str, cleartoeol);
}

string Window::GetString(const string &base, unsigned int length, int width) const
{
	int input, beginning, maxbeginning, minx, x, y, maxx;
	getyx(itsWindow, y, x);
	minx = maxx = x;
	
	width--;
	if (width == -1)
		width = itsWidth-x-1;
	if (width < 0)
		return "";
	
	curs_set(1);
	wstring tmp = ToWString(base);
	
	string tmp_in;
	wchar_t wc_in;
	
	mbstate_t state;
	memset(&state, 0, sizeof(state));
	
	maxbeginning = beginning = tmp.length() < width ? 0 : tmp.length()-width;
	maxx += tmp.length() < width ? tmp.length() : width;
	x = maxx;
	
	do
	{
		maxbeginning = tmp.length() < width ? 0 : tmp.length()-width;
		maxx = minx + (tmp.length() < width ? tmp.length() : width);
		
		if (beginning > maxbeginning)
			beginning = maxbeginning;
		
		mvwhline(itsWindow, y, minx, 32, width+1);
		mvwprintw(itsWindow, y, minx, "%ls", tmp.substr(beginning, width+1).c_str());
		
		if (itsGetStringHelper)
			itsGetStringHelper();
		
		wmove(itsWindow, y, x);
		input = wgetch(itsWindow);
		
		switch (input)
		{
			case ERR:
				continue;
			case KEY_UP:
			case KEY_DOWN:
				break;
			case KEY_RIGHT:
			{
				if (x < maxx)
					x++;
				else if (beginning < maxbeginning)
					beginning++;
				break;
			}
			case KEY_BACKSPACE: case 127:
			{
				if (x <= minx && !beginning)
					break;
			}
			case KEY_LEFT:
			{
				if (x > minx)
					x--;
				else if (beginning > 0)
					beginning--;
				if (input != KEY_BACKSPACE && input != 127)
					break; // backspace = left & delete.
			}
			case KEY_DC:
			{
				if ((x-minx)+beginning == tmp.length())
					break;
				tmp.erase(tmp.begin()+(x-minx)+beginning);
				if (beginning && beginning == maxbeginning && x < maxx)
					x++;
				break;
			}
			case KEY_HOME:
			{
				x = minx;
				beginning = 0;
				break;
			}
			case KEY_END:
			{
				x = maxx;
				beginning = maxbeginning;
				break;
			}
			case 10:
				break;
			default:
			{
				if (tmp.length() >= length)
					break;
				
				tmp_in += input;
				if ((int)mbrtowc(&wc_in, tmp_in.c_str(), MB_CUR_MAX, &state) < 0)
					break;
				
				if ((x-minx)+beginning >= tmp.length())
				{
					tmp.push_back(wc_in);
					if (!beginning)
						x++;
					beginning++;
				}
				else
				{
					tmp.insert(tmp.begin()+(x-minx)+beginning, wc_in);
					if (x < maxx)
						x++;
					else if (beginning < maxbeginning)
						beginning++;
				}
				tmp_in.clear();
			}
		}
	}
	while (input != 10);
	curs_set(0);
	return ToString(tmp);
}

void Window::Scrollable(bool scrollable) const
{
	scrollok(itsWindow, scrollable);
	idlok(itsWindow, scrollable);
}

void Window::GetXY(int &x, int &y) const
{
	getyx(itsWindow, y, x);
}

void Window::GotoXY(int x, int y) const
{
	wmove(itsWindow, y, x);
}

int Window::GetWidth() const
{
	if (itsBorder != brNone)
		return itsWidth+2;
	else
		return itsWidth;
}

int Window::GetHeight() const
{
	int height = itsHeight;
	if (itsBorder != brNone)
		height += 2;
	if (!itsTitle.empty())
		height += 2;
	return height;
}

int Window::GetStartX() const
{
	if (itsBorder != brNone)
		return itsStartX-1;
	else
		return itsStartX;
}

int Window::GetStartY() const
{
	int starty = itsStartY;
	if (itsBorder != brNone)
		starty--;
	if (!itsTitle.empty())
		starty -= 2;
	return starty;
}

string Window::GetTitle() const
{
	return itsTitle;
}

Color Window::GetColor() const
{
	return itsColor;
}

Border Window::GetBorder() const
{
	return itsBorder;
}

void Window::EnableColors()
{
	if (has_colors())
	{
		start_color();
		use_default_colors();
		int num = 1;
		for (int i = -1; i < 8; i++)
			for (int j = 0; j < 8; j++)
				init_pair(num++, j, i);
	}
}

Window * Window::EmptyClone() const
{
	return new Window(GetStartX(),GetStartY(),GetWidth(),GetHeight(),itsTitle,itsBaseColor,itsBorder);
}

/*char * ToString(const wchar_t *ws)
{
	string s;
	for (int i = 0; i < wcslen(ws); i++)
	{
		char *c = new char[MB_CUR_MAX+1]();
		if (wctomb(c, ws[i]) > 0)
			s += c;
		delete [] c;
	}
	char *result = strdup(s.c_str());
	return result;
}

wchar_t * ToWString(const char *s)
{
	wchar_t *ws = new wchar_t[strlen(s)+1]();
	mbstowcs(ws, s, strlen(s));
	return ws;
}*/

string ToString(const wstring &ws)
{
	string s;
	const wchar_t *c_ws = ws.c_str();
	mbstate_t mbs;
	memset(&mbs, 0, sizeof(mbs));
	int len = wcsrtombs(NULL, &c_ws, 0, &mbs);
	
	if (len <= 0)
		return s;
	
	char *c_s = new char[len+1]();
	wcsrtombs(c_s, &c_ws, len, &mbs);
	c_s[len] = 0;
	s = c_s;
	delete [] c_s;
	return s;
}

wstring ToWString(const string &s)
{
	wstring ws;
	const char *c_s = s.c_str();
	mbstate_t mbs;
	memset(&mbs, 0, sizeof(mbs));
	int len = mbsrtowcs(NULL, &c_s, 0, &mbs);
	
	if (len <= 0)
		return ws;
	
	wchar_t *c_ws = new wchar_t[len+1]();
	mbsrtowcs(c_ws, &c_s, len, &mbs);
	c_ws[len] = 0;
	ws = c_ws;
	delete [] c_ws;
	return ws;
}

Coordinates Window::IntoCoordinates(const string &s)
{
	Coordinates result;
	unsigned int sep = s.find(",");
	if (sep != string::npos)
	{
		result.first = atoi(s.substr(2, sep-2).c_str());
		result.second = atoi(s.substr(sep+1).c_str());
	}
	else
	{
		result.first = atoi(s.substr(2).c_str());
		result.second = -1;
	}
	return result;
}

string Window::OmitBBCodes(const string &str)
{
	bool collect = false;
	string tmp, color, result;
	for (string::const_iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it == '[' && (*(it+1) == '.' || *(it+1) == '/'))
			collect = 1;
		
		if (!collect)
			tmp += *it;
		else
		{
			if (*it != '[')
				color += *it;
			else
			{
				tmp += color;
				color = *it;
			}
		}
		
		if (*it == ']' || it+1 == str.end())
			collect = 0;
		
		if (!collect && !color.empty())
		{
			result += tmp;
			tmp.clear();
			if (!isdigit(tmp[2]) && !IsValidColor(color))
				tmp += color;
			color.clear();
		}
	}
	if (!tmp.empty()) result += tmp;
	return result;
}

size_t Window::RealLength(const string &s)
{
	if (s.empty())
		return 0;
	
	bool collect = false;
	int length = 0;
	
#	ifdef UTF8_ENABLED
	wstring ws = ToWString(s);
	wstring tmp;
#	else
	const string &ws = s;
	string tmp;
#	endif
	
	for (int i = 0; i < ws.length(); i++, length++)
	{
		if (ws[i] == '[' && (ws[i+1] == '.' || ws[i+1] == '/'))
			collect = 1;
		
		if (collect)
		{
			if (ws[i] != '[')
				tmp += ws[i];
			else
				tmp = ws[i];
		}
		
		if (ws[i] == ']')
			collect = 0;
		
		if (!collect && !tmp.empty())
		{
			if (isdigit(tmp[2]) || IsValidColor(TO_STRING(tmp)))
			{
#				ifdef UTF8_ENABLED
				length -= Length(tmp);
#				else
				length -= tmp.length();
#				endif
			}
			tmp.clear();
		}
	}
	return length;
}

size_t Window::Length(const wstring &ws)
{
	size_t length = 0;
	for (wstring::const_iterator it = ws.begin(); it != ws.end(); it++)
		length += wcwidth(*it);
	return length;
}

