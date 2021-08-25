/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#ifndef NCMPCPP_SAMPLE_BUFFER_H
#define NCMPCPP_SAMPLE_BUFFER_H

#include <cstdint>
#include <vector>

struct SampleBuffer
{
	typedef std::vector<int16_t>::iterator Iterator;

	SampleBuffer() : m_offset(0) { }

	void put(Iterator begin, Iterator end);
	size_t get(size_t elems, std::vector<int16_t> &dest);

	void resize(size_t n);
	void clear();

	size_t size() const;
	const std::vector<int16_t> &buffer() const;

private:
	size_t m_offset;
	std::vector<int16_t> m_buffer;
};

#endif // NCMPCPP_SAMPLE_BUFFER_H
