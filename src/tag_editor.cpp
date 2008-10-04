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

#include "id3v2tag.h"
#include "textidentificationframe.h"
#include "mpegfile.h"

#include "helpers.h"
#include "status_checker.h"

extern ncmpcpp_config Config;
extern ncmpcpp_keys Key;

extern Menu<string> *mTagEditor;
extern Window *wFooter;

string IntoStr(mpd_TagItems tag)
{
	switch (tag)
	{
		case MPD_TAG_ITEM_ARTIST:
			return "Artist";
		case MPD_TAG_ITEM_ALBUM:
			return "Album";
		case MPD_TAG_ITEM_TITLE:
			return "Title";
		case MPD_TAG_ITEM_TRACK:
			return "Track";
		case MPD_TAG_ITEM_GENRE:
			return "Genre";
		case MPD_TAG_ITEM_DATE:
			return "Year";
		case MPD_TAG_ITEM_COMPOSER:
			return "Composer";
		case MPD_TAG_ITEM_PERFORMER:
			return "Performer";
		case MPD_TAG_ITEM_COMMENT:
			return "Comment";
		case MPD_TAG_ITEM_DISC:
			return "Disc";
		case MPD_TAG_ITEM_FILENAME:
			return "Filename";
		default:
			return "";
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

string DisplayTag(const Song &s, void *data, const Menu<Song> *null)
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
			return s.GetComposer();
		case 7:
			return s.GetPerformer();
		case 8:
			return s.GetDisc();
		case 9:
			return s.GetComment();
		case 11:
			return s.GetNewName().empty() ? s.GetName() : s.GetName() + " [." + Config.color2 + "]->[/" + Config.color2 + "] " + s.GetNewName();
		default:
			return "";
	}
}

bool GetSongTags(Song &s)
{
	string path_to_file = Config.mpd_music_dir + s.GetFile();
	
	TagLib::FileRef f(path_to_file.c_str());
	if (f.isNull())
		return false;
	s.SetComment(f.tag()->comment().to8Bit(UNICODE));
	
	string ext = s.GetFile();
	ext = ext.substr(ext.find_last_of(".")+1);
	
	mTagEditor->Clear();
	mTagEditor->Reset();
	
	mTagEditor->AddOption("[.b][." + Config.color1 + "]Song name: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + s.GetName() + "[/" + Config.color2 + "]", 0, 1);
	mTagEditor->AddOption("[.b][." + Config.color1 + "]Location in DB: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + s.GetDirectory() + "[/" + Config.color2 + "]", 0, 1);
	mTagEditor->AddOption("", 0, 1);
	mTagEditor->AddOption("[.b][." + Config.color1 + "]Length: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + s.GetLength() + "[/" + Config.color2 + "]", 0, 1);
	mTagEditor->AddOption("[.b][." + Config.color1 + "]Bitrate: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + IntoStr(f.audioProperties()->bitrate()) + " kbps[/" + Config.color2 + "]", 0, 1);
	mTagEditor->AddOption("[.b][." + Config.color1 + "]Sample rate: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + IntoStr(f.audioProperties()->sampleRate()) + " Hz[/" + Config.color2 + "]", 0, 1);
	mTagEditor->AddOption("[.b][." + Config.color1 + "]Channels: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + (string)(f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") + "[/" + Config.color2 + "]", 0, 1);
	
	mTagEditor->AddSeparator();
	
	mTagEditor->AddOption("[.b]Title:[/b] " + s.GetTitle());
	mTagEditor->AddOption("[.b]Artist:[/b] " + s.GetArtist());
	mTagEditor->AddOption("[.b]Album:[/b] " + s.GetAlbum());
	mTagEditor->AddOption("[.b]Year:[/b] " + s.GetYear());
	mTagEditor->AddOption("[.b]Track:[/b] " + s.GetTrack());
	mTagEditor->AddOption("[.b]Genre:[/b] " + s.GetGenre());
	mTagEditor->AddOption("[.b]Composer:[/b] " + s.GetComposer(), 0, ext != "mp3");
	mTagEditor->AddOption("[.b]Performer:[/b] " + s.GetPerformer(), 0, ext != "mp3");
	mTagEditor->AddOption("[.b]Disc:[/b] " + s.GetDisc(), 0, ext != "mp3");
	mTagEditor->AddOption("[.b]Comment:[/b] " + s.GetComment());
	mTagEditor->AddSeparator();
	mTagEditor->AddOption("[.b]Filename:[/b] " + s.GetName());
	mTagEditor->AddSeparator();
	mTagEditor->AddOption("Save");
	mTagEditor->AddOption("Cancel");
	return true;
}

bool WriteTags(Song &s)
{
	using namespace TagLib;
	string path_to_file = Config.mpd_music_dir + s.GetFile();
	FileRef f(path_to_file.c_str());
	if (!f.isNull())
	{
		s.GetEmptyFields(1);
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
		s.GetEmptyFields(0);
		if (!s.GetNewName().empty())
		{
			string old_name = Config.mpd_music_dir + s.GetFile();
			string new_name = Config.mpd_music_dir + s.GetDirectory() + "/" + s.GetNewName();
			rename(old_name.c_str(), new_name.c_str());
		}
		return true;
	}
	else
		return false;
}

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
		string result = Window::OmitBBCodes(DisplaySong(s, &pattern));
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
			for (int i = mask.find("%"); i != string::npos; i = mask.find("%"))
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

void __deal_with_filenames(SongList &v)
{
	int width = 30;
	int height = 6;
	
	GetPatternList();
	
	Menu<string> *Main = new Menu<string>((COLS-width)/2, (LINES-height)/2, width, height, "", Config.main_color, Config.window_border);
	Main->SetTimeout(ncmpcpp_window_timeout);
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
			Main->Go(wDown);
		else if (Keypressed(input, Key.Up))
			Main->Go(wUp);
	}
	
	width = COLS*0.9;
	height = LINES*0.8;
	bool exit = 0;
	bool preview = 1;
	int choice = Main->GetChoice();
	int one_width = width/2;
	int two_width = width-one_width;
	
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
		Legend->Add("%a - artist\n");
		Legend->Add("%t - title\n");
		Legend->Add("%b - album\n");
		Legend->Add("%y - year\n");
		Legend->Add("%n - track number\n");
		Legend->Add("%g - genre\n");
		Legend->Add("%c - composer\n");
		Legend->Add("%p - performer\n");
		Legend->Add("%d - disc\n");
		Legend->Add("%C - comment\n\n");
		Legend->Add("[.b]Files:[/b]\n");
		for (SongList::const_iterator it = v.begin(); it != v.end(); it++)
			Legend->Add("[." + Config.color2 + "]*[/" + Config.color2 + "] " + (*it)->GetName() + "\n");
		
		Preview = static_cast<Scrollpad *>(Legend->EmptyClone());
		Preview->SetTitle("Preview");
		Preview->SetTimeout(ncmpcpp_window_timeout);
		
		Main = new Menu<string>((COLS-width)/2, (LINES-height)/2, one_width, height, "", Config.main_color, Config.active_window_border);
		Main->SetTimeout(ncmpcpp_window_timeout);
		
		if (!patterns_list.empty())
			Config.pattern = patterns_list.front();
		Main->AddOption("[.b]Pattern:[/b] " + Config.pattern);
		Main->AddOption("Preview");
		Main->AddOption("Legend");
		Main->AddSeparator();
		Main->AddOption("Proceed");
		Main->AddOption("Cancel");
		if (!patterns_list.empty())
		{
			Main->AddSeparator();
			Main->AddOption("Recent patterns", 1, 1, 0, lCenter);
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
				Active->Go(wUp);
			else if (Keypressed(input, Key.Down))
				Active->Go(wDown);
			else if (Keypressed(input, Key.PageUp))
				Active->Go(wPageUp);
			else if (Keypressed(input, Key.PageDown))
				Active->Go(wPageDown);
			else if (Keypressed(input, Key.Home))
				Active->Go(wHome);
			else if (Keypressed(input, Key.End))
				Active->Go(wEnd);
			else if (Keypressed(input, Key.Enter) && Active == Main)
			{
				switch (Main->GetRealChoice())
				{
					case 0:
					{
						LockStatusbar();
						wFooter->WriteXY(0, Config.statusbar_visibility, "[.b]Pattern:[/b] ", 1);
						string new_pattern = wFooter->GetString(Config.pattern);
						UnlockStatusbar();
						if (!new_pattern.empty())
						{
							Config.pattern = new_pattern;
							Main->UpdateOption(0, "[.b]Pattern:[/b] " + Config.pattern);
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
									Preview->Add("[.b]" + s.GetName() + ":[/b]\n");
									Preview->Add(ParseFilename(s, Config.pattern, preview) + "\n");
								}
								else
									ParseFilename(s, Config.pattern, preview);
							}
							else
							{
								const string &file = s.GetName();
								int last_dot = file.find_last_of(".");
								string extension = file.substr(last_dot);
								s.GetEmptyFields(1);
								string new_file = GenerateFilename(s, Config.pattern);
								if (new_file.empty())
								{
									if (preview)
										new_file = "[.red]!EMPTY![/red]";
									else
									{
										ShowMessage("File '" + s.GetName() + "' would have an empty name!");
										success = 0;
										break;
									}
								}
								if (!preview)
									s.SetNewName(new_file + extension);
								Preview->Add(file + " [." + Config.color2 + "]->[/" + Config.color2 + "] " + new_file + extension + "\n\n");
								s.GetEmptyFields(0);
							}
						}
						if (!success)
						{
							preview = 1;
							continue;
						}
						else
						{
							for (int i = 0; i < patterns_list.size(); i++)
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
						Config.pattern = Main->GetOption();
						Main->UpdateOption(0, "[.b]Pattern:[/b] " + Config.pattern);
					}
				}
			}
			else if (Keypressed(input, Key.VolumeUp) && Active == Main)
			{
				Active->SetBorder(Config.window_border);
				Active->Display(1);
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
				Active->Display(1);
			}
		}
	}
	SavePatternList();
	delete Main;
	delete Legend;
	delete Preview;
}

#endif

