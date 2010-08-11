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

#ifndef _LYRICS_FETCHER_H
#define _LYRICS_FETCHER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_CURL_CURL_H

#include <string>

struct LyricsFetcher
{
	typedef std::pair<bool, std::string> Result;
	
	virtual const char *name() = 0;
	virtual Result fetch(const std::string &artist, const std::string &title);
	
	protected:
		virtual const char *getURL() = 0;
		virtual const char *getOpenTag() = 0;
		virtual const char *getCloseTag() = 0;
		
		virtual bool notLyrics(GNUC_UNUSED const std::string &data) { return false; }
		virtual void postProcess(std::string &data);
		
		bool getContent(const char *open_tag, const char *close_tag, std::string &data);
		
		static const char msgNotFound[];
};

struct LyrcComArFetcher : public LyricsFetcher
{
	virtual const char *name() { return "lyrc.com.ar"; }
	
	protected:
		virtual const char *getURL() { return "http://lyrc.com.ar/tema1es.php?artist=%artist%&songname=%title%"; }
		virtual const char *getOpenTag() { return "</table>"; }
		virtual const char *getCloseTag() { return "<p>"; }
};

struct LyricwikiFetcher : public LyricsFetcher
{
	virtual const char *name() { return "lyricwiki.com"; }
	virtual Result fetch(const std::string &artist, const std::string &title);
	
	protected:
		virtual const char *getURL() { return "http://lyrics.wikia.com/api.php?action=lyrics&fmt=xml&func=getSong&artist=%artist%&song=%title%"; }
		virtual const char *getOpenTag() { return "<url>"; }
		virtual const char *getCloseTag() { return "</url>"; }
		
		virtual bool notLyrics(const std::string &data);
};

struct LyricsflyFetcher : public LyricsFetcher
{
	virtual const char *name() { return "lyricsfly.com"; }
	
	protected:
		virtual const char *getURL() { return "http://api.lyricsfly.com/api/api.php?i=1b76e55254f5f22ae-temporary.API.access&a=%artist%&t=%title%"; }
		virtual const char *getOpenTag() { return "<tx>"; }
		virtual const char *getCloseTag() { return "</tx>"; }
		
		virtual void postProcess(std::string &data);
};

/**********************************************************************/

struct GoogleLyricsFetcher : public LyricsFetcher
{
	virtual Result fetch(const std::string &artist, const std::string &title);
	
	protected:
		virtual const char *getSiteKeyword() = 0;
		virtual const char *getURL() { return URL; }
		
		virtual bool notLyrics(const std::string &data);
		virtual bool isURLOk(const std::string &url);
	private:
		const char *URL;
};

struct LyricstimeFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() { return "lyricstime.com"; }
	
	protected:
		virtual const char *getSiteKeyword() { return "lyricstime"; }
		virtual const char *getOpenTag() { return "<div id=\"songlyrics\" >"; }
		virtual const char *getCloseTag() { return "</div>"; }
		
		virtual bool isURLOk(const std::string &url);
		
		virtual void postProcess(std::string &data);
};

struct MetrolyricsFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() { return "metrolyrics.com"; }
	
	protected:
		virtual const char *getSiteKeyword() { return "metrolyrics"; }
		virtual const char *getOpenTag() { return "<div id=\"lyrics\">"; }
		virtual const char *getCloseTag() { return "</div>"; }
		
		virtual bool isURLOk(const std::string &url);
		
		virtual void postProcess(std::string &data);
};

struct LyricsmaniaFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() { return "lyricsmania.com"; }
	
	protected:
		virtual const char *getSiteKeyword() { return "lyricsmania"; }
		virtual const char *getOpenTag() { return "</strong> :<br />"; }
		virtual const char *getCloseTag() { return "&#91; <a"; }
		
		virtual void postProcess(std::string &data);
};

struct SonglyricsFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() { return "songlyrics.com"; }
	
	protected:
		virtual const char *getSiteKeyword() { return "songlyrics"; }
		virtual const char *getOpenTag() { return "-6000px;\">"; }
		virtual const char *getCloseTag() { return "</p>"; }
		
		virtual void postProcess(std::string &data);
};

struct LyriczzFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() { return "lyriczz.com"; }
	
	protected:
		virtual const char *getSiteKeyword() { return "lyriczz"; }
		virtual const char *getOpenTag() { return "border=0 /></a>"; }
		virtual const char *getCloseTag() { return "<a href"; }
		
		//virtual void postProcess(std::string &data);
};

extern LyricsFetcher *lyricsPlugins[];

#endif // HAVE_CURL_CURL_H

#endif
