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

ColorPair Window::into_color(const string &str)
{
	ColorPair colors;
	
	if (str == "[/]")
	{
		Bold(0);
		Reverse(0);
		AltCharset(0);
		if (!itsColors.empty())
			itsColors.pop();
	}
	else if (str[1] == '/')
	{
		if (str == "[/a]")
			AltCharset(0);
		else if (str == "[/b]")
			Bold(0);
		else if (str == "[/r]")
			Reverse(0);
		else if (str.length() > 4) // /green etc.
		{
			if (!itsColors.empty())
				itsColors.pop();
		}
	}
	else
	{
		if (str == "[a]")
			AltCharset(1);
		else if (str == "[b]")
			Bold(1);
		else if (str == "[r]")
			Reverse(1);
		else if (str == "[red]")
		{
			colors.first = clRed;
			colors.second = itsBaseBgColor;
			itsColors.push(colors);
		}
		else if (str == "[black]")
		{
			colors.first = clBlack;
			colors.second = itsBaseBgColor;
			itsColors.push(colors);
		}
		else if (str == "[blue]")
		{
			colors.first = clBlue;
			colors.second = itsBaseBgColor;
			itsColors.push(colors);
		}
		else if (str == "[green]")
		{
			colors.first = clGreen;
			colors.second = itsBaseBgColor;
			itsColors.push(colors);
		}
		else if (str == "[yellow]")
		{
			colors.first = clYellow;
			colors.second = itsBaseBgColor;
			itsColors.push(colors);
		}
		else if (str == "[magenta]")
		{
			colors.first = clMagenta;
			colors.second = itsBaseBgColor;
			itsColors.push(colors);
		}
		else if (str == "[cyan]")
		{
			colors.first = clCyan;
			colors.second = itsBaseBgColor;
			itsColors.push(colors);
		}
		else if (str == "[white]")
		{
			colors.first = clWhite;
			colors.second = itsBaseBgColor;
			itsColors.push(colors);
		}
		
		/*if (str == "[black_red]")
		{
			colors.first = clBlack;
			colors.second = clRed;
		}
		if (str == "[black_blue]")
		{
			colors.first = clBlack;
			colors.second = clBlue;
		}
		if (str == "[black_green]")
		{
			colors.first = clBlack;
			colors.second = clGreen;
		}
		if (str == "[black_yellow]")
		{
			colors.first = clBlack;
			colors.second = clYellow;
		}
		if (str == "[black_magenta]")
		{
			colors.first = clBlack;
			colors.second = clMagenta;
		}
		if (str == "[black_cyan]")
		{
			colors.first = clBlack;
			colors.second = clCyan;
		}
		if (str == "[black_white]")
		{
			colors.first = clBlack;
			colors.second = clWhite;
		}
		if (str == "[red_black]")
		{
			colors.first = clRed;
			colors.second = clBlack;
		}
		if (str == "[red_blue]")
		{
			colors.first = clRed;
			colors.second = clBlue;
		}
		if (str == "[red_green]")
		{
			colors.first = clRed;
			colors.second = clGreen;
		}
		if (str == "[red_yellow]")
		{
			colors.first = clRed;
			colors.second = clYellow;
		}
		if (str == "[red_magenta]")
		{
			colors.first = clRed;
			colors.second = clMagenta;
		}
		if (str == "[red_cyan]")
		{
			colors.first = clRed;
			colors.second = clCyan;
		}
		if (str == "[red_white]")
		{
			colors.first = clRed;
			colors.second = clWhite;
		}
		if (str == "[red_white]")
		{
			colors.first = clRed;
			colors.second = clWhite;
		}
		if (str == "[blue_black]")
		{
			colors.first = clBlue;
			colors.second = clBlack;
		}
		if (str == "[blue_red]")
		{
			colors.first = clBlue;
			colors.second = clRed;
		}
		if (str == "[blue_green]")
		{
			colors.first = clBlue;
			colors.second = clGreen;
		}
		if (str == "[blue_yellow]")
		{
			colors.first = clBlue;
			colors.second = clYellow;
		}
		if (str == "[blue_magenta]")
		{
			colors.first = clBlue;
			colors.second = clMagenta;
		}
		if (str == "[blue_cyan]")
		{
			colors.first = clBlue;
			colors.second = clCyan;
		}
		if (str == "[blue_white]")
		{
			colors.first = clBlue;
			colors.second = clWhite;
		}
		if (str == "[green_black]")
		{
			colors.first = clGreen;
			colors.second = clBlack;
		}
		if (str == "[green_red]")
		{
			colors.first = clGreen;
			colors.second = clRed;
		}
		if (str == "[green_blue]")
		{
			colors.first = clGreen;
			colors.second = clBlue;
		}
		if (str == "[green_yellow]")
		{
			colors.first = clGreen;
			colors.second = clYellow;
		}
		if (str == "[green_magenta]")
		{
			colors.first = clGreen;
			colors.second = clMagenta;
		}
		if (str == "[green_cyan]")
		{
			colors.first = clGreen;
			colors.second = clCyan;
		}
		if (str == "[green_white]")
		{
			colors.first = clGreen;
			colors.second = clWhite;
		}
		if (str == "[yellow_black]")
		{
			colors.first = clYellow;
			colors.second = clBlack;
		}
		if (str == "[yellow_red]")
		{
			colors.first = clYellow;
			colors.second = clRed;
		}
		if (str == "[yellow_blue]")
		{
			colors.first = clYellow;
			colors.second = clBlue;
		}
		if (str == "[yellow_green]")
		{
			colors.first = clYellow;
			colors.second = clGreen;
		}
		if (str == "[yellow_magenta]")
		{
			colors.first = clYellow;
			colors.second = clMagenta;
		}
		if (str == "[yellow_cyan]")
		{
			colors.first = clYellow;
			colors.second = clCyan;
		}
		if (str == "[yellow_white]")
		{
			colors.first = clYellow;
			colors.second = clWhite;
		}
		if (str == "[magenta_black]")
		{
			colors.first = clMagenta;
			colors.second = clBlack;
		}
		if (str == "[magenta_red]")
		{
			colors.first = clMagenta;
			colors.second = clRed;
		}
		if (str == "[magenta_blue]")
		{
			colors.first = clMagenta;
			colors.second = clBlue;
		}
		if (str == "[magenta_green]")
		{
			colors.first = clMagenta;
			colors.second = clGreen;
		}
		if (str == "[magenta_yellow]")
		{
			colors.first = clMagenta;
			colors.second = clYellow;
		}
		if (str == "[magenta_cyan]")
		{
			colors.first = clMagenta;
			colors.second = clCyan;
		}
		if (str == "[magenta_white]")
		{
			colors.first = clMagenta;
			colors.second = clWhite;
		}
		if (str == "[cyan_black]")
		{
			colors.first = clCyan;
			colors.second = clBlack;
		}
		if (str == "[cyan_red]")
		{
			colors.first = clCyan;
			colors.second = clRed;
		}
		if (str == "[cyan_blue]")
		{
			colors.first = clCyan;
			colors.second = clBlue;
		}
		if (str == "[cyan_green]")
		{
			colors.first = clCyan;
			colors.second = clGreen;
		}
		if (str == "[cyan_yellow]")
		{
			colors.first = clCyan;
			colors.second = clYellow;
		}
		if (str == "[cyan_magenta]")
		{
			colors.first = clCyan;
			colors.second = clMagenta;
		}
		if (str == "[cyan_white]")
		{
			colors.first = clCyan;
			colors.second = clWhite;
		}
		if (str == "[white_black]")
		{
			colors.first = clWhite;
			colors.second = clBlack;
		}
		if (str == "[white_red]")
		{
			colors.first = clWhite;
			colors.second = clRed;
		}
		if (str == "[white_blue]")
		{
			colors.first = clWhite;
			colors.second = clBlue;
		}
		if (str == "[white_green]")
		{
			colors.first = clWhite;
			colors.second = clGreen;
		}
		if (str == "[white_yellow]")
		{
			colors.first = clWhite;
			colors.second = clYellow;
		}
		if (str == "[white_magenta]")
		{
			colors.first = clWhite;
			colors.second = clMagenta;
		}
		if (str == "[white_cyan]")
		{
			colors.first = clWhite;
			colors.second = clCyan;
		}*/
	}
	
	if (itsColors.empty())
	{
		colors.first = itsBaseColor;
		colors.second = itsBaseBgColor;
		return colors;
	}
	else
		return itsColors.top();
}

bool is_valid_color(const string &str)
{
	return  str == "[/]" ||
		str == "[b]" ||
		str == "[/b]" ||
		str == "[r]" ||
		str == "[/r]" ||
		str == "[a]" ||
		str == "[/a]" ||
		
		str == "[black]" ||
		str == "[/black]" ||
		str == "[red]" ||
		str == "[/red]" ||
		str == "[blue]" ||
		str == "[/blue]" ||
		str == "[green]" ||
		str == "[/green]" ||
		str == "[yellow]" ||
		str == "[/yellow]" ||
		str == "[magenta]" ||
		str == "[/magenta]" ||
		str == "[cyan]" ||
		str == "[/cyan]" ||
		str == "[white]" ||
		str == "[/white]";
			
		/*str == "[black_red]"||
		str == "[/black_red]" ||
		str == "[black_blue]"||
		str == "[/black_blue]" ||
		str == "[black_green]"||
		str == "[/black_green]" ||
		str == "[black_yellow]"||
		str == "[/black_yellow]" ||
		str == "[black_magenta]"||
		str == "[/black_magenta]" ||
		str == "[black_cyan]"||
		str == "[/black_cyan]" ||
		str == "[black_white]"||
		str == "[/black_white]" ||
			
		str == "[red_black]"||
		str == "[/red_black]" ||
		str == "[red_blue]"||
		str == "[/red_blue]" ||
		str == "[red_green]"||
		str == "[/red_green]" ||
		str == "[red_yellow]"||
		str == "[/red_yellow]" ||
		str == "[red_magenta]"||
		str == "[/red_magenta]" ||
		str == "[red_cyan]"||
		str == "[/red_cyan]" ||
		str == "[red_white]"||
		str == "[/red_white]" ||

		str == "[blue_black]"||
		str == "[/blue_black]" ||
		str == "[blue_red]"||
		str == "[/blue_red]" ||
		str == "[blue_green]"||
		str == "[/blue_green]" ||
		str == "[blue_yellow]"||
		str == "[/blue_yellow]" ||
		str == "[blue_magenta]"||
		str == "[/blue_magenta]" ||
		str == "[blue_cyan]"||
		str == "[/blue_cyan]" ||
		str == "[blue_white]"||
		str == "[/blue_white]" ||

		str == "[green_black]"||
		str == "[/green_black]" ||
		str == "[green_red]"||
		str == "[/green_red]" ||
		str == "[green_blue]"||
		str == "[/green_blue]" ||
		str == "[green_yellow]"||
		str == "[/green_yellow]" ||
		str == "[green_magenta]"||
		str == "[/green_magenta]" ||
		str == "[green_cyan]"||
		str == "[/green_cyan]" ||
		str == "[green_white]"||
		str == "[/green_white]" ||

		str == "[yellow_black]"||
		str == "[/yellow_black]" ||
		str == "[yellow_red]"||
		str == "[/yellow_red]" ||
		str == "[yellow_blue]"||
		str == "[/yellow_blue]" ||
		str == "[yellow_green]"||
		str == "[/yellow_green]" ||
		str == "[yellow_magenta]"||
		str == "[/yellow_magenta]" ||
		str == "[yellow_cyan]"||
		str == "[/yellow_cyan]" ||
		str == "[yellow_white]"||
		str == "[/yellow_white]" ||

		str == "[magenta_black]"||
		str == "[/magenta_black]" ||
		str == "[magenta_red]"||
		str == "[/magenta_red]" ||
		str == "[magenta_blue]"||
		str == "[/magenta_blue]" ||
		str == "[magenta_green]"||
		str == "[/magenta_green]" ||
		str == "[magenta_yellow]"||
		str == "[/magenta_yellow]" ||
		str == "[magenta_cyan]"||
		str == "[/magenta_cyan]" ||
		str == "[magenta_white]"||
		str == "[/magenta_white]" ||

		str == "[cyan_black]"||
		str == "[/cyan_black]" ||
		str == "[cyan_red]"||
		str == "[/cyan_red]" ||
		str == "[cyan_blue]"||
		str == "[/cyan_blue]" ||
		str == "[cyan_green]"||
		str == "[/cyan_green]" ||
		str == "[cyan_yellow]"||
		str == "[/cyan_yellow]" ||
		str == "[cyan_magenta]"||
		str == "[/cyan_magenta]" ||
		str == "[cyan_white]"||
		str == "[/cyan_white]" ||

		str == "[white_black]"||
		str == "[/white_black]" ||
		str == "[white_red]"||
		str == "[/white_red]" ||
		str == "[white_blue]"||
		str == "[/white_blue]" ||
		str == "[white_green]"||
		str == "[/white_green]" ||
		str == "[white_yellow]"||
		str == "[/white_yellow]" ||
		str == "[white_magenta]"||
		str == "[/white_magenta]" ||
		str == "[white_cyan]"||
		str == "[/white_cyan]";*/
}

