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

#include <fstream>

#include "id3v2tag.h"
#include "textidentificationframe.h"
#include "mpegfile.h"

#include "helpers.h"
#include "status_checker.h"

using namespace MPD;

extern ncmpcpp_keys Key;

extern Connection *Mpd;
extern Menu<Song> *mPlaylist;

extern Menu<Buffer> *mTagEditor;
extern Window *wFooter;
extern Window *wPrev;

namespace
{
	const string patterns_list_file = config_dir + "patterns.list";
	vector<string> patterns_list;
	
	void GetPatternList()
	{
		if (patterns_list.empty())
		{
			std::ifstream input(patterns_list_file.c_str());
			if (input.is_open())
			{
				string line;
				while (getline(input, line))
				{
					if (!line.empty())
						patterns_list.push_back(line);
				}
				input.close();
			}
		}
	}
	
	void SavePatternList()
	{
		std::ofstream output(patterns_list_file.c_str());
		if (output.is_open())
		{
			for (vector<string>::const_iterator it = patterns_list.begin(); it != patterns_list.end() && it != patterns_list.begin()+30; it++)
				output << *it << std::endl;
			output.close();
		}
	}
	
	SongSetFunction IntoSetFunction(char c)
	{
		switch (c)
		{
			case 'a':
				return &Song::SetArtist;
			case 't':
				return &Song::SetTitle;
			case 'b':
				return &Song::SetAlbum;
			case 'y':
				return &Song::SetYear;
			case 'n':
				return &Song::SetTrack;
			case 'g':
				return &Song::SetGenre;
			case 'c':
				return &Song::SetComposer;
			case 'p':
				return &Song::SetPerformer;
			case 'd':
				return &Song::SetDisc;
			case 'C':
				return &Song::SetComment;
			default:
				return NULL;
		}
	}

	string GenerateFilename(const Song &s, string &pattern)
	{
		string result = s.toString(pattern);
		EscapeUnallowedChars(result);
		return result;
	}

	string ParseFilename(Song &s, string mask, bool preview)
	{
		std::stringstream result;
		vector<string> separators;
		vector< std::pair<char, string> > tags;
		string file = s.GetName().substr(0, s.GetName().find_last_of("."));
		
		try
		{
			for (size_t i = mask.find("%"); i != string::npos; i = mask.find("%"))
			{
				tags.push_back(make_pair(mask.at(i+1), ""));
				mask = mask.substr(i+2);
				i = mask.find("%");
				if (!mask.empty())
					separators.push_back(mask.substr(0, i));
			}
			int i = 0;
			for (vector<string>::const_iterator it = separators.begin(); it != separators.end(); it++, i++)
			{
				int j = file.find(*it);
				tags.at(i).second = file.substr(0, j);
				file = file.substr(j+it->length());
			}
			if (!file.empty())
				tags.at(i).second = file;
		}
		catch (std::out_of_range)
		{
			return "Error while parsing filename!";
		}
		
		for (vector< std::pair<char, string> >::iterator it = tags.begin(); it != tags.end(); it++)
		{
			for (string::iterator j = it->second.begin(); j != it->second.end(); j++)
				if (*j == '_')
					*j = ' ';
			
			if (!preview)
			{
				SongSetFunction set = IntoSetFunction(it->first);
				if (set)
					(s.*set)(it->second);
			}
			else
				result << "%" << it->first << ": " << it->second << "\n";
		}
		return result.str();
	}
}

SongSetFunction IntoSetFunction(mpd_TagItems tag)
{
	switch (tag)
	{
		case MPD_TAG_ITEM_ARTIST:
			return &Song::SetArtist;
		case MPD_TAG_ITEM_ALBUM:
			return &Song::SetAlbum;
		case MPD_TAG_ITEM_TITLE:
			return &Song::SetTitle;
		case MPD_TAG_ITEM_TRACK:
			return &Song::SetTrack;
		case MPD_TAG_ITEM_GENRE:
			return &Song::SetGenre;
		case MPD_TAG_ITEM_DATE:
			return &Song::SetYear;
		case MPD_TAG_ITEM_COMPOSER:
			return &Song::SetComposer;
		case MPD_TAG_ITEM_PERFORMER:
			return &Song::SetPerformer;
		case MPD_TAG_ITEM_COMMENT:
			return &Song::SetComment;
		case MPD_TAG_ITEM_DISC:
			return &Song::SetDisc;
		case MPD_TAG_ITEM_FILENAME:
			return &Song::SetNewName;
		default:
			return NULL;
	}
}

string FindSharedDir(Menu<Song> *menu)
{
	SongList list;
	for (size_t i = 0; i < menu->Size(); i++)
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
		size_t slash = result.find_last_of("/");
		result = slash != string::npos ? result.substr(0, slash) : "/";
	}
	return result;
}

void DisplayTag(const Song &s, void *data, Menu<Song> *menu)
{
	switch (static_cast<Menu<string> *>(data)->Choice())
	{
		case 0:
			*menu << ShowTag(s.GetTitle());
			return;
		case 1:
			*menu << ShowTag(s.GetArtist());
			return;
		case 2:
			*menu << ShowTag(s.GetAlbum());
			return;
		case 3:
			*menu << ShowTag(s.GetYear());
			return;
		case 4:
			*menu << ShowTag(s.GetTrack());
			return;
		case 5:
			*menu << ShowTag(s.GetGenre());
			return;
		case 6:
			*menu << ShowTag(s.GetComposer());
			return;
		case 7:
			*menu << ShowTag(s.GetPerformer());
			return;
		case 8:
			*menu << ShowTag(s.GetDisc());
			return;
		case 9:
			*menu << ShowTag(s.GetComment());
			return;
		case 11:
			if (s.GetNewName().empty())
				*menu << s.GetName();
			else
				*menu << s.GetName() << Config.color2 << " -> " << clEnd << s.GetNewName();
			return;
		default:
			return;
	}
}

void ReadTagsFromFile(mpd_Song *s)
{
	TagLib::FileRef f(s->file);
	if (f.isNull())
		return;
	
	TagLib::MPEG::File *mpegf = dynamic_cast<TagLib::MPEG::File *>(f.file());

	s->artist = !f.tag()->artist().isEmpty() ? str_pool_get(f.tag()->artist().toCString(UNICODE)) : 0;
	s->title = !f.tag()->title().isEmpty() ? str_pool_get(f.tag()->title().toCString(UNICODE)) : 0;
	s->album = !f.tag()->album().isEmpty() ? str_pool_get(f.tag()->album().toCString(UNICODE)) : 0;
	s->track = f.tag()->track() ? str_pool_get(IntoStr(f.tag()->track()).c_str()) : 0;
	s->date = f.tag()->year() ? str_pool_get(IntoStr(f.tag()->year()).c_str()) : 0;
	s->genre = !f.tag()->genre().isEmpty() ? str_pool_get(f.tag()->genre().toCString(UNICODE)) : 0;
	if (mpegf)
	{
		s->composer = !mpegf->ID3v2Tag()->frameListMap()["TCOM"].isEmpty()
		? (!mpegf->ID3v2Tag()->frameListMap()["TCOM"].front()->toString().isEmpty()
		   ? str_pool_get(mpegf->ID3v2Tag()->frameListMap()["TCOM"].front()->toString().toCString(UNICODE))
		   : 0)
		: 0;
		s->performer = !mpegf->ID3v2Tag()->frameListMap()["TOPE"].isEmpty()
		? (!mpegf->ID3v2Tag()->frameListMap()["TOPE"].front()->toString().isEmpty()
		   ? str_pool_get(mpegf->ID3v2Tag()->frameListMap()["TOPE"].front()->toString().toCString(UNICODE))
		   : 0)
		: 0;
		s->disc = !mpegf->ID3v2Tag()->frameListMap()["TPOS"].isEmpty()
		? (!mpegf->ID3v2Tag()->frameListMap()["TPOS"].front()->toString().isEmpty()
		   ? str_pool_get(mpegf->ID3v2Tag()->frameListMap()["TPOS"].front()->toString().toCString(UNICODE))
		   : 0)
		: 0;
	}
	s->comment = !f.tag()->comment().isEmpty() ? str_pool_get(f.tag()->comment().toCString(UNICODE)) : 0;
	s->time = f.audioProperties()->length();
}

bool GetSongTags(Song &s)
{
	string path_to_file;
	if (s.IsFromDB())
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	
	TagLib::FileRef f(path_to_file.c_str());
	if (f.isNull())
		return false;
	s.SetComment(f.tag()->comment().to8Bit(UNICODE));
	
	string ext = s.GetFile();
	ext = ext.substr(ext.find_last_of(".")+1);
	ToLower(ext);
	
	mTagEditor->Clear();
	mTagEditor->Reset();
	
	mTagEditor->ResizeBuffer(23);
	
	for (size_t i = 0; i < 7; i++)
		mTagEditor->Static(i, 1);
	
	mTagEditor->IntoSeparator(7);
	mTagEditor->IntoSeparator(18);
	mTagEditor->IntoSeparator(20);
	
	if (ext != "mp3")
		for (size_t i = 14; i <= 16; i++)
			mTagEditor->Static(i, 1);
	
	mTagEditor->Highlight(8);
	
	mTagEditor->at(0) << fmtBold << Config.color1 << "Song name: " << fmtBoldEnd << Config.color2 << s.GetName() << clEnd;
	mTagEditor->at(1) << fmtBold << Config.color1 << "Location in DB: " << fmtBoldEnd << Config.color2 << ShowTag(s.GetDirectory()) << clEnd;
	mTagEditor->at(3) << fmtBold << Config.color1 << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << clEnd;
	mTagEditor->at(4) << fmtBold << Config.color1 << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps" << clEnd;
	mTagEditor->at(5) << fmtBold << Config.color1 << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz" << clEnd;
	mTagEditor->at(6) << fmtBold << Config.color1 << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << clDefault;
	
	mTagEditor->at(8) << fmtBold << "Title:" << fmtBoldEnd << ' ' << ShowTag(s.GetTitle());
	mTagEditor->at(9) << fmtBold << "Artist:" << fmtBoldEnd << ' ' << ShowTag(s.GetArtist());
	mTagEditor->at(10) << fmtBold << "Album:" << fmtBoldEnd << ' ' << ShowTag(s.GetAlbum());
	mTagEditor->at(11) << fmtBold << "Year:" << fmtBoldEnd << ' ' << ShowTag(s.GetYear());
	mTagEditor->at(12) << fmtBold << "Track:" << fmtBoldEnd << ' ' << ShowTag(s.GetTrack());
	mTagEditor->at(13) << fmtBold << "Genre:" << fmtBoldEnd << ' ' << ShowTag(s.GetGenre());
	mTagEditor->at(14) << fmtBold << "Composer:" << fmtBoldEnd << ' ' << ShowTag(s.GetComposer());
	mTagEditor->at(15) << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(s.GetPerformer());
	mTagEditor->at(16) << fmtBold << "Disc:" << fmtBoldEnd << ' ' << ShowTag(s.GetDisc());
	mTagEditor->at(17) << fmtBold << "Comment:" << fmtBoldEnd << ' ' << ShowTag(s.GetComment());

	mTagEditor->at(19) << fmtBold << "Filename:" << fmtBoldEnd << ' ' << s.GetName();

	mTagEditor->at(21) << "Save";
	mTagEditor->at(22) << "Cancel";
	return true;
}

bool WriteTags(Song &s)
{
	using namespace TagLib;
	string path_to_file;
	bool file_is_from_db = s.IsFromDB();
	if (file_is_from_db)
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	FileRef f(path_to_file.c_str());
	if (!f.isNull())
	{
		f.tag()->setTitle(TO_WSTRING(s.GetTitle()));
		f.tag()->setArtist(TO_WSTRING(s.GetArtist()));
		f.tag()->setAlbum(TO_WSTRING(s.GetAlbum()));
		f.tag()->setYear(StrToInt(s.GetYear()));
		f.tag()->setTrack(StrToInt(s.GetTrack()));
		f.tag()->setGenre(TO_WSTRING(s.GetGenre()));
		f.tag()->setComment(TO_WSTRING(s.GetComment()));
		f.save();
		
		string ext = s.GetFile();
		ext = ext.substr(ext.find_last_of(".")+1);
		ToLower(ext);
		if (ext == "mp3")
		{
			MPEG::File file(path_to_file.c_str());
			ID3v2::Tag *tag = file.ID3v2Tag();
			String::Type encoding = UNICODE ? String::UTF8 : String::Latin1;
			ByteVector Composer("TCOM");
			ByteVector Performer("TOPE");
			ByteVector Disc("TPOS");
			ID3v2::Frame *ComposerFrame = new ID3v2::TextIdentificationFrame(Composer, encoding);
			ID3v2::Frame *PerformerFrame = new ID3v2::TextIdentificationFrame(Performer, encoding);
			ID3v2::Frame *DiscFrame = new ID3v2::TextIdentificationFrame(Disc, encoding);
			ComposerFrame->setText(TO_WSTRING(s.GetComposer()));
			PerformerFrame->setText(TO_WSTRING(s.GetPerformer()));
			DiscFrame->setText(TO_WSTRING(s.GetDisc()));
			tag->removeFrames(Composer);
			tag->addFrame(ComposerFrame);
			tag->removeFrames(Performer);
			tag->addFrame(PerformerFrame);
			tag->removeFrames(Disc);
			tag->addFrame(DiscFrame);
			file.save();
		}
		if (!s.GetNewName().empty())
		{
			string old_name;
			if (file_is_from_db)
				old_name += Config.mpd_music_dir;
			old_name += s.GetFile();
			string new_name;
			if (file_is_from_db)
				new_name += Config.mpd_music_dir;
			new_name += s.GetDirectory() + "/" + s.GetNewName();
			if (rename(old_name.c_str(), new_name.c_str()) == 0 && !file_is_from_db)
			{
				if (wPrev == mPlaylist)
				{
					// if we rename local file, it won't get updated
					// so just remove it from playlist and add again
					size_t pos = mPlaylist->Choice();
					Mpd->QueueDeleteSong(pos);
					Mpd->CommitQueue();
					int id = Mpd->AddSong("file://" + new_name);
					if (id >= 0)
					{
						s = mPlaylist->Back();
						Mpd->Move(s.GetPosition(), pos);
					}
				}
				else // only mBrowser
					s.SetFile(new_name);
			}
		}
		return true;
	}
	else
		return false;
}

void __deal_with_filenames(SongList &v)
{
	int width = 30;
	int height = 6;
	
	GetPatternList();
	
	Menu<string> *Main = new Menu<string>((COLS-width)/2, (LINES-height)/2, width, height, "", Config.main_color, Config.window_border);
	Main->SetTimeout(ncmpcpp_window_timeout);
	Main->SetItemDisplayer(GenericDisplayer);
	Main->AddOption("Get tags from filename");
	Main->AddOption("Rename files");
	Main->AddSeparator();
	Main->AddOption("Cancel");
	Main->Display();
	
	int input = 0;
	while (!Keypressed(input, Key.Enter))
	{
		TraceMpdStatus();
		Main->Refresh();
		Main->ReadKey(input);
		if (Keypressed(input, Key.Down))
			Main->Scroll(wDown);
		else if (Keypressed(input, Key.Up))
			Main->Scroll(wUp);
	}
	
	width = COLS*0.9;
	height = LINES*0.8;
	bool exit = 0;
	bool preview = 1;
	size_t choice = Main->Choice();
	size_t one_width = width/2;
	size_t two_width = width-one_width;
	
	delete Main;
	
	Main = 0;
	Scrollpad *Helper = 0;
	Scrollpad *Legend = 0;
	Scrollpad *Preview = 0;
	Window *Active = 0;
	
	if (choice != 3)
	{
		Legend = new Scrollpad((COLS-width)/2+one_width, (LINES-height)/2, two_width, height, "Legend", Config.main_color, Config.window_border);
		Legend->SetTimeout(ncmpcpp_window_timeout);
		*Legend << "%a - artist\n";
		*Legend << "%t - title\n";
		*Legend << "%b - album\n";
		*Legend << "%y - year\n";
		*Legend << "%n - track number\n";
		*Legend << "%g - genre\n";
		*Legend << "%c - composer\n";
		*Legend << "%p - performer\n";
		*Legend << "%d - disc\n";
		*Legend << "%C - comment\n\n";
		*Legend << fmtBold << "Files:\n" << fmtBoldEnd;
		for (SongList::const_iterator it = v.begin(); it != v.end(); it++)
			*Legend << Config.color2 << " * " << clEnd << (*it)->GetName() << "\n";
		Legend->Flush();
		
		Preview = Legend->EmptyClone();
		Preview->SetTitle("Preview");
		Preview->SetTimeout(ncmpcpp_window_timeout);
		
		Main = new Menu<string>((COLS-width)/2, (LINES-height)/2, one_width, height, "", Config.main_color, Config.active_window_border);
		Main->SetTimeout(ncmpcpp_window_timeout);
		Main->SetItemDisplayer(GenericDisplayer);
		
		if (!patterns_list.empty())
			Config.pattern = patterns_list.front();
		Main->AddOption("Pattern: " + Config.pattern);
		Main->AddOption("Preview");
		Main->AddOption("Legend");
		Main->AddSeparator();
		Main->AddOption("Proceed");
		Main->AddOption("Cancel");
		if (!patterns_list.empty())
		{
			Main->AddSeparator();
			Main->AddOption("Recent patterns", 1, 1, 0);
			Main->AddSeparator();
			for (vector<string>::const_iterator it = patterns_list.begin(); it != patterns_list.end(); it++)
				Main->AddOption(*it);
		}
		
		Active = Main;
		Helper = Legend;
		
		Main->SetTitle(!choice ? "Get tags from filename" : "Rename files");
		Main->Display();
		Helper->Display();
		
		while (!exit)
		{
			TraceMpdStatus();
			Active->Refresh();
			Active->ReadKey(input);
				
			if (Keypressed(input, Key.Up))
				Active->Scroll(wUp);
			else if (Keypressed(input, Key.Down))
				Active->Scroll(wDown);
			else if (Keypressed(input, Key.PageUp))
				Active->Scroll(wPageUp);
			else if (Keypressed(input, Key.PageDown))
				Active->Scroll(wPageDown);
			else if (Keypressed(input, Key.Home))
				Active->Scroll(wHome);
			else if (Keypressed(input, Key.End))
				Active->Scroll(wEnd);
			else if (Keypressed(input, Key.Enter) && Active == Main)
			{
				switch (Main->RealChoice())
				{
					case 0:
					{
						LockStatusbar();
						Statusbar() << "Pattern: ";
						string new_pattern = wFooter->GetString(Config.pattern);
						UnlockStatusbar();
						if (!new_pattern.empty())
						{
							Config.pattern = new_pattern;
							Main->at(0) = "Pattern: ";
							Main->at(0) += Config.pattern;
						}
						break;
					}
					case 3: // save
						preview = 0;
					case 1:
					{
						bool success = 1;
						ShowMessage("Parsing...");
						Preview->Clear();
						for (SongList::iterator it = v.begin(); it != v.end(); it++)
						{
							Song &s = **it;
							if (!choice)
							{
								if (preview)
								{
									*Preview << fmtBold << s.GetName() << ":\n" << fmtBoldEnd;
									*Preview << ParseFilename(s, Config.pattern, preview) << "\n";
								}
								else
									ParseFilename(s, Config.pattern, preview);
							}
							else
							{
								const string &file = s.GetName();
								int last_dot = file.find_last_of(".");
								string extension = file.substr(last_dot);
								basic_buffer<my_char_t> new_file;
								new_file << TO_WSTRING(GenerateFilename(s, Config.pattern));
								if (new_file.Str().empty())
								{
									if (preview)
										new_file << clRed << "!EMPTY!" << clEnd;
									else
									{
										ShowMessage("File '%s' would have an empty name!", s.GetName().c_str());
										success = 0;
										break;
									}
								}
								if (!preview)
									s.SetNewName(TO_STRING(new_file.Str()) + extension);
								*Preview << file << Config.color2 << " -> " << clEnd << new_file << extension << "\n\n";
							}
						}
						if (!success)
						{
							preview = 1;
							continue;
						}
						else
						{
							for (size_t i = 0; i < patterns_list.size(); i++)
							{
								if (patterns_list[i] == Config.pattern)
								{
									patterns_list.erase(patterns_list.begin()+i);
									i--;
								}
							}
							patterns_list.insert(patterns_list.begin(), Config.pattern);
						}
						ShowMessage("Operation finished!");
						if (preview)
						{
							Helper = Preview;
							Helper->Flush();
							Helper->Display();
							break;
						}
					}
					case 4:
					{
						exit = 1;
						break;
					}
					case 2:
					{
						Helper = Legend;
						Helper->Display();
						break;
					}
					default:
					{
						Config.pattern = Main->Current();
						Main->at(0) = "Pattern: " + Config.pattern;
					}
				}
			}
			else if (Keypressed(input, Key.VolumeUp) && Active == Main)
			{
				Active->SetBorder(Config.window_border);
				Active->Display();
				Active = Helper;
				Active->SetBorder(Config.active_window_border);
				Active->Display();
			}
			else if (Keypressed(input, Key.VolumeDown) && Active == Helper)
			{
				Active->SetBorder(Config.window_border);
				Active->Display();
				Active = Main;
				Active->SetBorder(Config.active_window_border);
				Active->Display();
			}
		}
	}
	SavePatternList();
	delete Main;
	delete Legend;
	delete Preview;
}

#endif

