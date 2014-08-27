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

#include "tags.h"

#ifdef HAVE_TAGLIB_H

// taglib includes
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <fileref.h>
#include <flacfile.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <tag.h>
#include <textidentificationframe.h>
#include <xiphcomment.h>

#include "global.h"
#include "settings.h"
#include "utility/wide_string.h"

namespace {//

TagLib::StringList tagList(const MPD::MutableSong &s, MPD::Song::GetFunction f)
{
	TagLib::StringList result;
	unsigned idx = 0;
	for (std::string value; !(value = (s.*f)(idx)).empty(); ++idx)
		result.append(ToWString(value));
	return result;
}

void readCommonTags(MPD::MutableSong &s, TagLib::Tag *tag)
{
	s.setTitle(tag->title().to8Bit(true));
	s.setArtist(tag->artist().to8Bit(true));
	s.setAlbum(tag->album().to8Bit(true));
	s.setDate(boost::lexical_cast<std::string>(tag->year()));
	s.setTrack(boost::lexical_cast<std::string>(tag->track()));
	s.setGenre(tag->genre().to8Bit(true));
	s.setComment(tag->comment().to8Bit(true));
}

void readID3v1Tags(MPD::MutableSong &s, TagLib::ID3v1::Tag *tag)
{
	readCommonTags(s, tag);
}

void readID3v2Tags(MPD::MutableSong &s, TagLib::ID3v2::Tag *tag)
{
	auto readFrame = [&s](const TagLib::ID3v2::FrameList &list, MPD::MutableSong::SetFunction f) {
		unsigned idx = 0;
		for (auto it = list.begin(); it != list.end(); ++it, ++idx)
		{
			if (auto textFrame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame *>(*it))
			{
				auto values = textFrame->fieldList();
				for (auto value = values.begin(); value != values.end(); ++value, ++idx)
					(s.*f)(value->to8Bit(true), idx);
			}
			else
				(s.*f)((*it)->toString().to8Bit(true), idx);
		}
	};
	auto &frames = tag->frameListMap();
	readFrame(frames["TIT2"], &MPD::MutableSong::setTitle);
	readFrame(frames["TPE1"], &MPD::MutableSong::setArtist);
	readFrame(frames["TPE2"], &MPD::MutableSong::setAlbumArtist);
	readFrame(frames["TALB"], &MPD::MutableSong::setAlbum);
	readFrame(frames["TDRC"], &MPD::MutableSong::setDate);
	readFrame(frames["TRCK"], &MPD::MutableSong::setTrack);
	readFrame(frames["TCON"], &MPD::MutableSong::setGenre);
	readFrame(frames["TCOM"], &MPD::MutableSong::setComposer);
	readFrame(frames["TPE3"], &MPD::MutableSong::setPerformer);
	readFrame(frames["TPOS"], &MPD::MutableSong::setDisc);
	readFrame(frames["COMM"], &MPD::MutableSong::setComment);
}

void readXiphComments(MPD::MutableSong &s, TagLib::Ogg::XiphComment *tag)
{
	auto readField = [&s](const TagLib::StringList &list, MPD::MutableSong::SetFunction f) {
		unsigned idx = 0;
		for (auto it = list.begin(); it != list.end(); ++it, ++idx)
			(s.*f)(it->to8Bit(true), idx);
	};
	auto &fields = tag->fieldListMap();
	readField(fields["TITLE"], &MPD::MutableSong::setTitle);
	readField(fields["ARTIST"], &MPD::MutableSong::setArtist);
	readField(fields["ALBUMARTIST"], &MPD::MutableSong::setAlbumArtist);
	readField(fields["ALBUM"], &MPD::MutableSong::setAlbum);
	readField(fields["DATE"], &MPD::MutableSong::setDate);
	readField(fields["TRACKNUMBER"], &MPD::MutableSong::setTrack);
	readField(fields["GENRE"], &MPD::MutableSong::setGenre);
	readField(fields["COMPOSER"], &MPD::MutableSong::setComposer);
	readField(fields["PERFORMER"], &MPD::MutableSong::setPerformer);
	readField(fields["DISCNUMBER"], &MPD::MutableSong::setDisc);
	readField(fields["COMMENT"], &MPD::MutableSong::setComment);
}

void clearID3v1Tags(TagLib::ID3v1::Tag *tag)
{
	tag->setTitle(TagLib::String::null);
	tag->setArtist(TagLib::String::null);
	tag->setAlbum(TagLib::String::null);
	tag->setYear(0);
	tag->setTrack(0);
	tag->setGenre(TagLib::String::null);
	tag->setComment(TagLib::String::null);
}

void writeCommonTags(const MPD::MutableSong &s, TagLib::Tag *tag)
{
	tag->setTitle(ToWString(s.getTitle()));
	tag->setArtist(ToWString(s.getArtist()));
	tag->setAlbum(ToWString(s.getAlbum()));
	try {
		tag->setYear(boost::lexical_cast<TagLib::uint>(s.getDate()));
	} catch (boost::bad_lexical_cast &) {
		std::cerr << "writeCommonTags: couldn't write 'year' tag to '" << s.getURI() << "' as it's not a positive integer\n";
	}
	try {
		tag->setTrack(boost::lexical_cast<TagLib::uint>(s.getTrack()));
	} catch (boost::bad_lexical_cast &) {
		std::cerr << "writeCommonTags: couldn't write 'track' tag to '" << s.getURI() << "' as it's not a positive integer\n";
	}
	tag->setGenre(ToWString(s.getGenre()));
	tag->setComment(ToWString(s.getComment()));
}

void writeID3v2Tags(const MPD::MutableSong &s, TagLib::ID3v2::Tag *tag)
{
	auto writeID3v2 = [&](const TagLib::ByteVector &type, const TagLib::StringList &list) {
		tag->removeFrames(type);
		if (!list.isEmpty())
		{
			auto frame = new TagLib::ID3v2::TextIdentificationFrame(type, TagLib::String::UTF8);
			frame->setText(list);
			tag->addFrame(frame);
		}
	};
	writeID3v2("TIT2", tagList(s, &MPD::Song::getTitle));
	writeID3v2("TPE1", tagList(s, &MPD::Song::getArtist));
	writeID3v2("TPE2", tagList(s, &MPD::Song::getAlbumArtist));
	writeID3v2("TALB", tagList(s, &MPD::Song::getAlbum));
	writeID3v2("TDRC", tagList(s, &MPD::Song::getDate));
	writeID3v2("TRCK", tagList(s, &MPD::Song::getTrack));
	writeID3v2("TCON", tagList(s, &MPD::Song::getGenre));
	writeID3v2("TCOM", tagList(s, &MPD::Song::getComposer));
	writeID3v2("TPE3", tagList(s, &MPD::Song::getPerformer));
	writeID3v2("TPOS", tagList(s, &MPD::Song::getDisc));
	writeID3v2("COMM", tagList(s, &MPD::Song::getComment));
}

void writeXiphComments(const MPD::MutableSong &s, TagLib::Ogg::XiphComment *tag)
{
	auto writeXiph = [&](const TagLib::String &type, const TagLib::StringList &list) {
		tag->removeField(type);
		for (auto it = list.begin(); it != list.end(); ++it)
			tag->addField(type, *it, false);
	};
	// remove field previously used as album artist
	tag->removeField("ALBUM ARTIST");
	// remove field TRACK, some taggers use it as TRACKNUMBER
	tag->removeField("TRACK");
	// remove field DISC, some taggers use it as DISCNUMBER
	tag->removeField("DISC");
	writeXiph("TITLE", tagList(s, &MPD::Song::getTitle));
	writeXiph("ARTIST", tagList(s, &MPD::Song::getArtist));
	writeXiph("ALBUMARTIST", tagList(s, &MPD::Song::getAlbumArtist));
	writeXiph("ALBUM", tagList(s, &MPD::Song::getAlbum));
	writeXiph("DATE", tagList(s, &MPD::Song::getDate));
	writeXiph("TRACKNUMBER", tagList(s, &MPD::Song::getTrack));
	writeXiph("GENRE", tagList(s, &MPD::Song::getGenre));
	writeXiph("COMPOSER", tagList(s, &MPD::Song::getComposer));
	writeXiph("PERFORMER", tagList(s, &MPD::Song::getPerformer));
	writeXiph("DISCNUMBER", tagList(s, &MPD::Song::getDisc));
	writeXiph("COMMENT", tagList(s, &MPD::Song::getComment));
}

Tags::ReplayGainInfo getReplayGain(TagLib::Ogg::XiphComment *tag)
{
	auto first_or_empty = [](const TagLib::StringList &list) {
		std::string result;
		if (!list.isEmpty())
			result = list.front().to8Bit(true);
		return result;
	};
	auto &fields = tag->fieldListMap();
	return Tags::ReplayGainInfo(
		first_or_empty(fields["REPLAYGAIN_REFERENCE_LOUDNESS"]),
		first_or_empty(fields["REPLAYGAIN_TRACK_GAIN"]),
		first_or_empty(fields["REPLAYGAIN_TRACK_PEAK"]),
		first_or_empty(fields["REPLAYGAIN_ALBUM_GAIN"]),
		first_or_empty(fields["REPLAYGAIN_ALBUM_PEAK"])
	);
}

}

namespace Tags {//

bool extendedSetSupported(const TagLib::File *f)
{
	return dynamic_cast<const TagLib::MPEG::File *>(f)
	||     dynamic_cast<const TagLib::Ogg::Vorbis::File *>(f)
	||     dynamic_cast<const TagLib::FLAC::File *>(f);
}

ReplayGainInfo readReplayGain(TagLib::File *f)
{
	ReplayGainInfo result;
	if (auto ogg_file = dynamic_cast<TagLib::Ogg::Vorbis::File *>(f))
	{
		if (auto xiph = ogg_file->tag())
			result = getReplayGain(xiph);
	}
	else if (auto flac_file = dynamic_cast<TagLib::FLAC::File *>(f))
	{
		if (auto xiph = flac_file->xiphComment())
			result = getReplayGain(xiph);
	}
	return result;
}

void read(MPD::MutableSong &s)
{
	TagLib::FileRef f(s.getURI().c_str());
	if (f.isNull())
		return;
	
	s.setDuration(f.audioProperties()->length());
	
	if (auto mpeg_file = dynamic_cast<TagLib::MPEG::File *>(f.file()))
	{
		if (auto id3v1 = mpeg_file->ID3v1Tag())
			readID3v1Tags(s, id3v1);
		if (auto id3v2 = mpeg_file->ID3v2Tag())
			readID3v2Tags(s, id3v2);
	}
	else if (auto ogg_file = dynamic_cast<TagLib::Ogg::Vorbis::File *>(f.file()))
	{
		if (auto xiph = ogg_file->tag())
			readXiphComments(s, xiph);
	}
	else if (auto flac_file = dynamic_cast<TagLib::FLAC::File *>(f.file()))
	{
		if (auto xiph = flac_file->xiphComment())
			readXiphComments(s, xiph);
	}
	else
		readCommonTags(s, f.tag());
}

bool write(MPD::MutableSong &s)
{
	std::string old_name;
	if (s.isFromDatabase())
		old_name += Config.mpd_music_dir;
	old_name += s.getURI();
	
	TagLib::FileRef f(old_name.c_str());
	if (f.isNull())
		return false;
	
	if (auto mp3_file = dynamic_cast<TagLib::MPEG::File *>(f.file()))
	{
		clearID3v1Tags(mp3_file->ID3v1Tag());
		writeID3v2Tags(s, mp3_file->ID3v2Tag(true));
	}
	else if (auto ogg_file = dynamic_cast<TagLib::Ogg::Vorbis::File *>(f.file()))
	{
		writeXiphComments(s, ogg_file->tag());
	}
	else if (auto flac_file = dynamic_cast<TagLib::FLAC::File *>(f.file()))
	{
		writeXiphComments(s, flac_file->xiphComment(true));
	}
	else
		writeCommonTags(s, f.tag());
	
	if (!f.save())
		return false;
	
	if (!s.getNewName().empty())
	{
		std::string new_name;
		if (s.isFromDatabase())
			new_name += Config.mpd_music_dir;
		new_name += s.getDirectory() + "/" + s.getNewName();
		if (std::rename(old_name.c_str(), new_name.c_str()) == 0 && !s.isFromDatabase())
		{
			// FIXME
			/*if (myTinyTagEditor == myPlaylist)
			{
				// if we rename local file, it won't get updated
				// so just remove it from playlist and add again
				size_t pos = myPlaylist->main().choice();
				Mpd.StartCommandsList();
				Mpd.Delete(pos);
				int id = Mpd.AddSong("file://" + new_name);
				if (id >= 0)
				{
					s = myPlaylist->main().back().value();
					Mpd.Move(s.getPosition(), pos);
				}
				Mpd.CommitCommandsList();
			}
			else // only myBrowser->main()
				myBrowser->GetDirectory(myBrowser->CurrentDir());*/
		}
	}
	return true;
}

}

#endif // HAVE_TAGLIB_H
