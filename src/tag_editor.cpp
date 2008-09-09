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

#include "tag_editor.h"

#ifdef HAVE_TAGLIB_H

extern ncmpcpp_config Config;
extern Menu<string> *mTagEditor;

string FindSharedDir(Menu<Song> *menu)
{
	SongList list;
	for (int i = 0; i < menu->Size(); i++)
		list.push_back(&menu->at(i));
	return FindSharedDir(list);
}

string FindSharedDir(const SongList &v)
{
	string result;
	if (!v.empty())
	{
		result = v.front()->GetFile();
		for (SongList::const_iterator it = v.begin()+1; it != v.end(); it++)
		{
			int i = 1;
			while (result.substr(0, i) == (*it)->GetFile().substr(0, i))
				i++;
			result = result.substr(0, i);
		}
		int slash = result.find_last_of("/");
		result = slash != string::npos ? result.substr(0, slash) : "/";
	}
	return result;
}

string DisplayTag(const Song &s, void *data)
{
	switch (static_cast<Menu<string> *>(data)->GetChoice())
	{
		case 0:
			return s.GetTitle();
		case 1:
			return s.GetArtist();
		case 2:
			return s.GetAlbum();
		case 3:
			return s.GetYear();
		case 4:
			return s.GetTrack();
		case 5:
			return s.GetGenre();
		case 6:
			return s.GetComment();
		case 8:
			return s.GetShortFilename();
		default:
			return "";
	}
}

bool GetSongTags(Song &s)
{
	string path_to_file = Config.mpd_music_dir + "/" + s.GetFile();
	
	TagLib::FileRef f(path_to_file.c_str());
	if (f.isNull())
		return false;
	s.SetComment(f.tag()->comment().to8Bit(UNICODE));
	
	mTagEditor->Clear();
	mTagEditor->Reset();
	
	mTagEditor->AddStaticOption("[.b][.white]Song name: [/white][.green][/b]" + s.GetShortFilename() + "[/green]");
	mTagEditor->AddStaticOption("[.b][.white]Location in DB: [/white][.green][/b]" + s.GetDirectory() + "[/green]");
	mTagEditor->AddStaticOption("");
	mTagEditor->AddStaticOption("[.b][.white]Length: [/white][.green][/b]" + s.GetLength() + "[/green]");
	mTagEditor->AddStaticOption("[.b][.white]Bitrate: [/white][.green][/b]" + IntoStr(f.audioProperties()->bitrate()) + " kbps[/green]");
	mTagEditor->AddStaticOption("[.b][.white]Sample rate: [/white][.green][/b]" + IntoStr(f.audioProperties()->sampleRate()) + " Hz[/green]");
	mTagEditor->AddStaticOption("[.b][.white]Channels: [/white][.green][/b]" + (string)(f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") + "[/green]");
	
	mTagEditor->AddSeparator();
	
	mTagEditor->AddOption("[.b]Title:[/b] " + s.GetTitle());
	mTagEditor->AddOption("[.b]Artist:[/b] " + s.GetArtist());
	mTagEditor->AddOption("[.b]Album:[/b] " + s.GetAlbum());
	mTagEditor->AddOption("[.b]Year:[/b] " + s.GetYear());
	mTagEditor->AddOption("[.b]Track:[/b] " + s.GetTrack());
	mTagEditor->AddOption("[.b]Genre:[/b] " + s.GetGenre());
	mTagEditor->AddOption("[.b]Comment:[/b] " + s.GetComment());
	mTagEditor->AddSeparator();
	mTagEditor->AddOption("Save");
	mTagEditor->AddOption("Cancel");
	return true;
}

bool WriteTags(Song &s)
{
	string path_to_file = Config.mpd_music_dir + "/" + s.GetFile();
	TagLib::FileRef f(path_to_file.c_str());
	if (!f.isNull())
	{
		s.GetEmptyFields(1);
		f.tag()->setTitle(s.GetTitle());
		f.tag()->setArtist(TO_WSTRING(s.GetArtist()));
		f.tag()->setAlbum(TO_WSTRING(s.GetAlbum()));
		f.tag()->setYear(StrToInt(s.GetYear()));
		f.tag()->setTrack(StrToInt(s.GetTrack()));
		f.tag()->setGenre(TO_WSTRING(s.GetGenre()));
		f.tag()->setComment(TO_WSTRING(s.GetComment()));
		s.GetEmptyFields(0);
		f.save();
		return true;
	}
	else
		return false;
}

#endif

