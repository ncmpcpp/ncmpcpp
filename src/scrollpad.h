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

#ifndef _SCROLLPAD_H
#define _SCROLLPAD_H

#include "window.h"
#include "strbuffer.h"

namespace NCurses
{
	class Scrollpad: public Window
	{
		public:
			Scrollpad(size_t, size_t, size_t, size_t, const std::string &, Color, Border);
			Scrollpad(const Scrollpad &);
			virtual ~Scrollpad() { }
			
			void Flush();
			bool SetFormatting(short, const std::basic_string<my_char_t> &, short, bool for_each = 1);
			void RemoveFormatting(short value) { itsBuffer.RemoveFormatting(value); }
			std::basic_string<my_char_t> Content() { return itsBuffer.Str(); }
			
			virtual void Refresh();
			virtual void Scroll(Where);
			
			virtual void Resize(size_t, size_t);
			virtual void Clear(bool = 1);
			
			template <typename T> Scrollpad &operator<<(const T &t)
			{
				itsBuffer << t;
				return *this;
			}
			
			Scrollpad &operator<<(std::ostream &(*os)(std::ostream &));
			
#			ifdef _UTF8
			bool SetFormatting(short vb, const std::string &s, short ve, bool for_each = 1) { return SetFormatting(vb, ToWString(s), ve, for_each); }
			Scrollpad &operator<<(const std::string &s);
#			endif // _UTF8
			
			virtual Scrollpad *Clone() const { return new Scrollpad(*this); }
			virtual Scrollpad *EmptyClone() const;
			
		protected:
			virtual void Recreate();
			
			basic_buffer<my_char_t> itsBuffer;
			
			int itsBeginning;
			
			size_t itsRealHeight;
	};
}

#endif

