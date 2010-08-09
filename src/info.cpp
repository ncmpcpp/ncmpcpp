/***************************************************************************
 *   Copyright (C) 2008-2010 by Andrzej Rybczak                            *
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

#include "info.h"
#include "curl_handle.h"

#ifdef HAVE_CURL_CURL_H
# include <fstream>
# ifdef WIN32
#  include <io.h>
# else
#  include <sys/stat.h>
# endif // WIN32
#endif

#include "browser.h"
#include "charset.h"
#include "global.h"
#include "media_library.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "status.h"
#include "tag_editor.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;
using Global::myOldScreen;

#ifdef HAVE_CURL_CURL_H
const std::string Info::Folder = home_path + HOME_FOLDER"artists";
bool Info::ArtistReady = 0;

#ifdef HAVE_PTHREAD_H
pthread_t *Info::Downloader = 0;
#endif // HAVE_PTHREAD_H

#endif // HAVE_CURL_CURL_H

Info *myInfo = new Info;

const Info::Metadata Info::Tags[] =
{
	{ "Title",		&MPD::Song::GetTitle,		&MPD::Song::SetTitle		},
	{ "Artist",		&MPD::Song::GetArtist,		&MPD::Song::SetArtist		},
	{ "Album Artist",	&MPD::Song::GetAlbumArtist,	&MPD::Song::SetAlbumArtist	},
	{ "Album",		&MPD::Song::GetAlbum,		&MPD::Song::SetAlbum		},
	{ "Year",		&MPD::Song::GetDate,		&MPD::Song::SetDate		},
	{ "Track",		&MPD::Song::GetTrack,		&MPD::Song::SetTrack		},
	{ "Genre",		&MPD::Song::GetGenre,		&MPD::Song::SetGenre		},
	{ "Composer",		&MPD::Song::GetComposer,	&MPD::Song::SetComposer		},
	{ "Performer",		&MPD::Song::GetPerformer,	&MPD::Song::SetPerformer	},
	{ "Disc",		&MPD::Song::GetDisc,		&MPD::Song::SetDisc		},
	{ "Comment",		&MPD::Song::GetComment,		&MPD::Song::SetComment		},
	{ 0,			0,				0				}
};

void Info::Init()
{
	w = new Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	isInitialized = 1;
}

void Info::Resize()
{
	w->Resize(COLS, MainHeight);
	w->MoveTo(0, MainStartY);
	hasToBeResized = 0;
}

std::basic_string<my_char_t> Info::Title()
{
	return TO_WSTRING(itsTitle);
}

#if defined(HAVE_CURL_CURL_H) && defined(HAVE_PTHREAD_H)
void Info::Update()
{
	if (!ArtistReady)
		return;
	pthread_join(*Downloader, 0);
	w->Flush();
	w->Refresh();
	delete Downloader;
	Downloader = 0;
	ArtistReady = 0;
}
#endif // HAVE_CURL_CURL_H && HAVE_PTHREAD_H

void Info::GetSong()
{
	if (myScreen == this)
	{
		myOldScreen->SwitchTo();
	}
	else
	{
		if (!isInitialized)
			Init();
		
		MPD::Song *s = myScreen->CurrentSong();
		
		if (!s)
			return;
		
		if (hasToBeResized)
			Resize();
		
		myOldScreen = myScreen;
		myScreen = this;
		Global::RedrawHeader = 1;
		itsTitle = "Song info";
		w->Clear();
		w->Reset();
		PrepareSong(*s);
		w->Window::Clear();
		w->Flush();
	}
}

#ifdef HAVE_CURL_CURL_H

void Info::GetArtist()
{
	if (myScreen == this)
	{
		myOldScreen->SwitchTo();
	}
	else
	{
		if (!isInitialized)
			Init();
		
#		ifdef HAVE_PTHREAD_H
		if (Downloader && !ArtistReady)
		{
			ShowMessage("Artist info is being downloaded...");
			return;
		}
		else if (ArtistReady)
			Update();
#		endif // HAVE_PTHREAD_H
		
		MPD::Song *s = myScreen->CurrentSong();
		
		if (!s && myScreen->ActiveWindow() != myLibrary->Artists)
			return;
		
		itsArtist = !s ? myLibrary->Artists->Current() : s->GetArtist();
		
		if (itsArtist.empty())
			return;
		
		if (hasToBeResized)
			Resize();
		myOldScreen = myScreen;
		myScreen = this;
		Global::RedrawHeader = 1;
		itsTitle = "Artist info - " + itsArtist;
		w->Clear();
		w->Reset();
		static_cast<Window &>(*w) << "Fetching artist info...";
		w->Window::Refresh();
#		ifdef HAVE_PTHREAD_H
		if (!Downloader)
#		endif // HAVE_PTHREAD_H
		{
			locale_to_utf(itsArtist);
			
			std::string file = itsArtist + ".txt";
			ToLower(file);
			EscapeUnallowedChars(file);
			
			itsFilenamePath = Folder + "/" + file;
			
			mkdir(Folder.c_str()
#			ifndef WIN32
			, 0755
#			endif // !WIN32
			);
			
			std::ifstream input(itsFilenamePath.c_str());
			if (input.is_open())
			{
				bool first = 1;
				std::string line;
				while (getline(input, line))
				{
					if (!first)
						*w << "\n";
					utf_to_locale(line);
					*w << line;
					first = 0;
				}
				input.close();
				w->SetFormatting(fmtBold, U("\n\nSimilar artists:\n"), fmtBoldEnd, 0);
				w->SetFormatting(Config.color2, U("\n * "), clEnd, 1);
				// below is used so format won't be removed using RemoveFormatting() by accident.
				w->ForgetFormatting();
				w->Flush();
			}
			else
			{
#				ifdef HAVE_PTHREAD_H
				Downloader = new pthread_t;
				pthread_create(Downloader, 0, PrepareArtist, this);
#				else
				PrepareArtist(this);
				w->Flush();
#				endif // HAVE_PTHREAD_H
			}
		}
	}
}

void *Info::PrepareArtist(void *screen_void_ptr)
{
	Info *screen = static_cast<Info *>(screen_void_ptr);
	
	std::string url = "http://ws.audioscrobbler.com/2.0/?method=artist.getinfo&artist=";
	url += Curl::escape(screen->itsArtist);
	url += "&api_key=d94e5b6e26469a2d1ffae8ef20131b79";
	
	std::string result;
	CURLcode code = Curl::perform(url, result);
	
	if (code != CURLE_OK)
	{
		*screen->w << "Error while fetching artist's info: " << curl_easy_strerror(code);
		ArtistReady = 1;
		pthread_exit(0);
	}
	
	size_t a, b;
	bool save = 1;
	
	a = result.find("status=\"failed\"");
	
	if (a != std::string::npos)
	{
		EscapeHtml(result);
		*screen->w << "Last.fm returned an error message: " << result;
		ArtistReady = 1;
		pthread_exit(0);
	}
	
	std::vector<std::string> similar;
	for (size_t i = result.find("<name>"); i != std::string::npos; i = result.find("<name>"))
	{
		result[i] = '.';
		size_t j = result.find("</name>");
		result[j] = '.';
		i += static_strlen("<name>");
		similar.push_back(result.substr(i, j-i));
		EscapeHtml(similar.back());
	}
	std::vector<std::string> urls;
	for (size_t i = result.find("<url>"); i != std::string::npos; i = result.find("<url>"))
	{
		result[i] = '.';
		size_t j = result.find("</url>");
		result[j] = '.';
		i += static_strlen("<url>");
		urls.push_back(result.substr(i, j-i));
	}
	
	bool parse_failed = 0;
	if ((a = result.find("<content>")) != std::string::npos)
	{
		a += static_strlen("<content>");
		if ((b = result.find("</content>")) == std::string::npos)
			parse_failed = 1;
	}
	else
		parse_failed = 1;
	if (parse_failed)
	{
		*screen->w << "Error: Fetched data could not be parsed";
		ArtistReady = 1;
		pthread_exit(0);
	}
	
	if (a == b)
	{
		result = "No description available for this artist.";
		save = 0;
	}
	else
	{
		a += static_strlen("<![CDATA[");
		b -= static_strlen("]]>");
		result = result.substr(a, b-a);
	}
	
	EscapeHtml(result);
	Trim(result);
	
	std::ostringstream filebuffer;
	if (save)
		filebuffer << result;
	utf_to_locale(result);
	*screen->w << result;
	
	if (save)
		filebuffer << "\n\nSimilar artists:\n";
	*screen->w << fmtBold << "\n\nSimilar artists:\n" << fmtBoldEnd;
	for (size_t i = 1; i < similar.size(); ++i)
	{
		if (save)
			filebuffer << "\n * " << similar[i] << " (" << urls[i] << ")";
		utf_to_locale(similar[i]);
		utf_to_locale(urls[i]);
		*screen->w << "\n" << Config.color2 << " * " << clEnd << similar[i] << " (" << urls[i] << ")";
	}
	
	if (save)
		filebuffer << "\n\n" << urls.front();
	utf_to_locale(urls.front());
	*screen->w << "\n\n" << urls.front();
	
	if (save)
	{
		std::ofstream output(screen->itsFilenamePath.c_str());
		if (output.is_open())
		{
			output << filebuffer.str();
			output.close();
		}
	}
	ArtistReady = 1;
	pthread_exit(0);
}
#endif // HVAE_CURL_CURL_H

void Info::PrepareSong(MPD::Song &s)
{
#	ifdef HAVE_TAGLIB_H
	std::string path_to_file;
	if (s.isFromDB())
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	TagLib::FileRef f(path_to_file.c_str());
	if (!f.isNull())
		s.SetComment(f.tag()->comment().to8Bit(1));
#	endif // HAVE_TAGLIB_H
	
	*w << fmtBold << Config.color1 << "Filename: " << fmtBoldEnd << Config.color2 << s.GetName() << "\n" << clEnd;
	*w << fmtBold << "Directory: " << fmtBoldEnd << Config.color2;
	ShowTag(*w, s.GetDirectory());
	*w << "\n\n" << clEnd;
	*w << fmtBold << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << "\n" << clEnd;
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		*w << fmtBold << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps\n" << clEnd;
		*w << fmtBold << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz\n" << clEnd;
		*w << fmtBold << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << "\n" << clDefault;
	}
#	endif // HAVE_TAGLIB_H
	*w << clDefault;
	
	for (const Metadata *m = Tags; m->Name; ++m)
	{
		*w << fmtBold << "\n" << m->Name << ": " << fmtBoldEnd;
		ShowTag(*w, s.GetTags(m->Get));
	}
}

