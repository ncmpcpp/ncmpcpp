/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#include <cassert>
#include "utility/type_conversions.h"

NC::Color stringToColor(const std::string &color)
{
	NC::Color result = NC::Color::Default;
	if (color == "black")
		result = NC::Color::Black;
	else if (color == "red")
		result = NC::Color::Red;
	else if (color == "green")
		result = NC::Color::Green;
	else if (color == "yellow")
		result = NC::Color::Yellow;
	else if (color == "blue")
		result = NC::Color::Blue;
	else if (color == "magenta")
		result = NC::Color::Magenta;
	else if (color == "cyan")
		result = NC::Color::Cyan;
	else if (color == "white")
		result = NC::Color::White;
	return result;
}

VisualizerType stringToVisualizerType(const std::string &visualizerType)
{
	VisualizerType result = VisualizerType::Wave;
	if (visualizerType == "wave")
		result = VisualizerType::Wave;
	else if (visualizerType == "spectrum")
		result = VisualizerType::Spectrum;
	else if (visualizerType == "wave_filled")
		result = VisualizerType::WaveFilled;
	return result;
}

NC::Border stringToBorder(const std::string &border)
{
	NC::Border result = NC::Border::None;
	if (border == "black")
		result = NC::Border::Black;
	else if (border == "red")
		result = NC::Border::Red;
	else if (border == "green")
		result = NC::Border::Green;
	else if (border == "yellow")
		result = NC::Border::Yellow;
	else if (border == "blue")
		result = NC::Border::Blue;
	else if (border == "magenta")
		result = NC::Border::Magenta;
	else if (border == "cyan")
		result = NC::Border::Cyan;
	else if (border == "white")
		result = NC::Border::White;
	return result;
}

std::string tagTypeToString(mpd_tag_type tag)
{
	switch (tag)
	{
		case MPD_TAG_ARTIST:
			return "Artist";
		case MPD_TAG_ALBUM:
			return "Album";
		case MPD_TAG_ALBUM_ARTIST:
			return "Album Artist";
		case MPD_TAG_TITLE:
			return "Title";
		case MPD_TAG_TRACK:
			return "Track";
		case MPD_TAG_GENRE:
			return "Genre";
		case MPD_TAG_DATE:
			return "Date";
		case MPD_TAG_COMPOSER:
			return "Composer";
		case MPD_TAG_PERFORMER:
			return "Performer";
		case MPD_TAG_COMMENT:
			return "Comment";
		case MPD_TAG_DISC:
			return "Disc";
		default:
			return "";
	}
}

MPD::MutableSong::SetFunction tagTypeToSetFunction(mpd_tag_type tag)
{
	switch (tag)
	{
		case MPD_TAG_ARTIST:
			return &MPD::MutableSong::setArtist;
		case MPD_TAG_ALBUM:
			return &MPD::MutableSong::setAlbum;
		case MPD_TAG_ALBUM_ARTIST:
			return &MPD::MutableSong::setAlbumArtist;
		case MPD_TAG_TITLE:
			return &MPD::MutableSong::setTitle;
		case MPD_TAG_TRACK:
			return &MPD::MutableSong::setTrack;
		case MPD_TAG_GENRE:
			return &MPD::MutableSong::setGenre;
		case MPD_TAG_DATE:
			return &MPD::MutableSong::setDate;
		case MPD_TAG_COMPOSER:
			return &MPD::MutableSong::setComposer;
		case MPD_TAG_PERFORMER:
			return &MPD::MutableSong::setPerformer;
		case MPD_TAG_COMMENT:
			return &MPD::MutableSong::setComment;
		case MPD_TAG_DISC:
			return &MPD::MutableSong::setDisc;
		default:
			return 0;
	}
}

mpd_tag_type charToTagType(char c)
{
	switch (c)
	{
		case 'a':
			return MPD_TAG_ARTIST;
		case 'A':
			return MPD_TAG_ALBUM_ARTIST;
		case 't':
			return MPD_TAG_TITLE;
		case 'b':
			return MPD_TAG_ALBUM;
		case 'y':
			return MPD_TAG_DATE;
		case 'n':
			return MPD_TAG_TRACK;
		case 'g':
			return MPD_TAG_GENRE;
		case 'c':
			return MPD_TAG_COMPOSER;
		case 'p':
			return MPD_TAG_PERFORMER;
		case 'd':
			return MPD_TAG_DISC;
		case 'C':
			return MPD_TAG_COMMENT;
		default:
			assert(false);
			return MPD_TAG_ARTIST;
	}
}

MPD::Song::GetFunction charToGetFunction(char c)
{
	switch (c)
	{
		case 'l':
			return &MPD::Song::getLength;
		case 'D':
			return &MPD::Song::getDirectory;
		case 'f':
			return &MPD::Song::getName;
		case 'a':
			return &MPD::Song::getArtist;
		case 'A':
			return &MPD::Song::getAlbumArtist;
		case 't':
			return &MPD::Song::getTitle;
		case 'b':
			return &MPD::Song::getAlbum;
		case 'y':
			return &MPD::Song::getDate;
		case 'n':
			return &MPD::Song::getTrackNumber;
		case 'N':
			return &MPD::Song::getTrack;
		case 'g':
			return &MPD::Song::getGenre;
		case 'c':
			return &MPD::Song::getComposer;
		case 'p':
			return &MPD::Song::getPerformer;
		case 'd':
			return &MPD::Song::getDisc;
		case 'C':
			return &MPD::Song::getComment;
		case 'P':
			return &MPD::Song::getPriority;
		default:
			return 0;
	}
}

std::string itemTypeToString(MPD::ItemType type)
{
	std::string result;
	switch (type)
	{
		case MPD::itDirectory:
			result = "directory";
			break;
		case MPD::itSong:
			result = "song";
			break;
		case MPD::itPlaylist:
			result = "playlist";
			break;
	}
	return result;
}
