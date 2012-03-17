/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
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

#include "xiphcomment.h"
#include "id3v2tag.h"
#include "textidentificationframe.h"
#include "fileref.h"
#include "mpegfile.h"
#include "vorbisfile.h"
#include "flacfile.h"

using std::cout;

bool framelist_empty(const TagLib::ID3v2::FrameList &fl)
{
    TagLib::ID3v2::FrameList::ConstIterator it = fl.begin();
    while (it != fl.end())
        if (!((*it++)->toString() == TagLib::String::null))
            return false;
    return true;
}

bool write_id3v2_aa(TagLib::ID3v2::Tag *tag, const TagLib::String &artist)
{
    using TagLib::ID3v2::TextIdentificationFrame;
    TagLib::ByteVector type = "TPE2";
    if (!framelist_empty(tag->frameList(type)))
        return false;
    TextIdentificationFrame *frame = new TextIdentificationFrame(type, TagLib::String::UTF8);
    frame->setText(artist);
    tag->addFrame(frame);
    return true;
}

bool write_xiphcomment_aa(TagLib::Ogg::XiphComment *tag, const TagLib::String &artist)
{
    if (tag->contains("ALBUM ARTIST"))
        return false;
    tag->addField("ALBUM ARTIST", artist, true);
    return true;
}

void convert(int n, char **files, bool dry_run)
{
    if (n == 0)
    {
        cout << "No files to convert, exiting.\n";
        return;
    }
    if (dry_run)
        cout << "Dry run mode enabled, pretending to modify files.\n";
    
    for (int i = 0; i < n; ++i)
    {
        cout << "Modifying " << files[i] << "... ";
        
        TagLib::FileRef f(files[i]);
        if (!f.isNull())
        {
            TagLib::String artist = f.tag()->artist();
            if (!(artist == TagLib::String::null))
            {
                TagLib::MPEG::File *mp3_f = 0;
                TagLib::Ogg::Vorbis::File *ogg_f = 0;
                TagLib::FLAC::File *flac_f = 0;
                
                bool success;
                if ((mp3_f = dynamic_cast<TagLib::MPEG::File *>(f.file())))
                {
                    success = write_id3v2_aa(mp3_f->ID3v2Tag(true), artist);
                }
                else if ((ogg_f = dynamic_cast<TagLib::Ogg::Vorbis::File *>(f.file())))
                {
                    success = write_xiphcomment_aa(ogg_f->tag(), artist);
                }
                else if ((flac_f = dynamic_cast<TagLib::FLAC::File *>(f.file())))
                {
                    success = write_xiphcomment_aa(flac_f->xiphComment(true), artist);
                }
                else
                {
                    cout << "Not mp3/ogg/flac file, skipping.\n";
                    continue;
                }
                
                if (success)
                {
                    if (!dry_run)
                        f.save();
                    cout << "Done.\n";
                }
                else
                    cout << "AlbumArtist is already here, skipping.\n";
            }
            else
                cout << "Artist not found, skipping.\n";
        }
        else
            cout << "Could not open file, skipping.\n";
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "This little program copies Artist tag (if present) to\n";
        cout << "AlbumArtist (if not present) for given mp3/ogg/flac files.\n\n";
        cout << "Usage: " << argv[0] << " [--dry-run] files\n";
    }
    else
    {
        bool dry_run = !strcmp(argv[1], "--dry-run");
        convert(argc-1-dry_run, &argv[1+dry_run], dry_run);
    }
    return 0;
}
