/***************************************************************************
 *   Copyright (C) 2008-2013 by Andrzej Rybczak                            *
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

#include <cstring>
#include <iostream>

#include <fileref.h>
#include <flacfile.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <textidentificationframe.h>
#include <id3v2tag.h>
#include <xiphcomment.h>

enum class CopyResult { Success, NoArtist, AlbumArtistAlreadyInPlace };

bool is_framelist_empty(const TagLib::ID3v2::FrameList &list)
{
	for (auto it = list.begin(); it != list.end(); ++it)
		if ((*it)->toString() != TagLib::String::null)
			return false;
	return true;
}

CopyResult copy_album_artist(TagLib::ID3v2::Tag *tag)
{
	typedef TagLib::ID3v2::TextIdentificationFrame TextFrame;
	
	TagLib::ByteVector album_artist = "TPE2";
	if (!is_framelist_empty(tag->frameList(album_artist)))
		return CopyResult::AlbumArtistAlreadyInPlace;
	
	auto artists = tag->frameList("TPE1");
	if (artists.isEmpty())
		return CopyResult::NoArtist;
	
	for (auto it = artists.begin(); it != artists.end(); ++it)
	{
		// this cast should always succeed.
		auto &textIt = dynamic_cast<TextFrame &>(**it);
		auto frame = new TextFrame(album_artist, TagLib::String::UTF8);
		frame->setText(textIt.fieldList());
		tag->addFrame(frame);
	}
	
	return CopyResult::Success;
}

CopyResult copy_album_artist(TagLib::Ogg::XiphComment *tag)
{
	if (tag->contains("ALBUM ARTIST") || tag->contains("ALBUMARTIST"))
		return CopyResult::AlbumArtistAlreadyInPlace;
	
	auto artists = tag->fieldListMap()["ARTIST"];
	if (artists.isEmpty())
		return CopyResult::NoArtist;
	
	for (auto it = artists.begin(); it != artists.end(); ++it)
		tag->addField("ALBUMARTIST", *it, false);
	
	return CopyResult::Success;
}

void convert(int n, char **files, bool dry_run)
{
	if (n == 0)
	{
		std::cout << "No files to convert, exiting.\n";
		return;
	}
	if (dry_run)
		std::cout << "Dry run mode enabled, pretending to modify files.\n";
	
	for (int i = 0; i < n; ++i)
	{
		std::cout << "Modifying " << files[i] << "... ";
		
		TagLib::FileRef f(files[i]);
		if (!f.isNull())
		{
			CopyResult result;
			if (auto mp3_f = dynamic_cast<TagLib::MPEG::File *>(f.file()))
			{
				result = copy_album_artist(mp3_f->ID3v2Tag(true));
			}
			else if (auto ogg_f = dynamic_cast<TagLib::Ogg::Vorbis::File *>(f.file()))
			{
				result = copy_album_artist(ogg_f->tag());
			}
			else if (auto flac_f = dynamic_cast<TagLib::FLAC::File *>(f.file()))
			{
				result = copy_album_artist(flac_f->xiphComment(true));
			}
			else
			{
				std::cout << "Not mp3/ogg/flac file, skipping.\n";
				continue;
			}
			
			switch (result)
			{
				case CopyResult::Success:
					if (!dry_run)
						f.save();
					std::cout << "Done.\n";
					break;
				case CopyResult::NoArtist:
					std::cout << "Artist not found, skipping.\n";
					break;
				case CopyResult::AlbumArtistAlreadyInPlace:
					std::cout << "AlbumArtist is already there, skipping.\n";
					break;
			}
		}
		else
			std::cout << "Could not open file, skipping.\n";
	}
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cout << "This little script copies Artist tag (if present) to\n";
		std::cout << "AlbumArtist (if not present) for given mp3/ogg/flac files.\n";
		std::cout << "\n";
		std::cout << "Usage: " << argv[0] << " [--dry-run] files\n";
		std::cout << "\n";
		std::cout << "Note: to run it recursively for all your files, you can use:\n";
		std::cout << "$ find DIRECTORY \\( -name \"*.flac\" -o -name \"*.mp3\" -o -name \"*.ogg\" \\) -exec ./artist_to_albumartist [--dry-run] {} \\;\n";
	}
	else
	{
		bool dry_run = !strcmp(argv[1], "--dry-run");
		convert(argc-1-dry_run, &argv[1+dry_run], dry_run);
	}
	return 0;
}
