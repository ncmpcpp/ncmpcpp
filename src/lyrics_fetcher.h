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

#ifndef NCMPCPP_LYRICS_FETCHER_H
#define NCMPCPP_LYRICS_FETCHER_H

#include "config.h"

#include <memory>
#include <string>

struct LyricsFetcher
{
	typedef std::pair<bool, std::string> Result;

	virtual ~LyricsFetcher() { }

	virtual const char *name() const = 0;
	virtual Result fetch(const std::string &artist, const std::string &title);
	
protected:
	virtual const char *urlTemplate() const = 0;
	virtual const char *regex() const = 0;
	
	virtual bool notLyrics(const std::string &) const { return false; }
	virtual void postProcess(std::string &data) const;
	
	std::vector<std::string> getContent(const char *regex, const std::string &data);
	
	static const char msgNotFound[];
};

typedef std::unique_ptr<LyricsFetcher> LyricsFetcher_;

typedef std::vector<LyricsFetcher_> LyricsFetchers;

std::istream &operator>>(std::istream &is, LyricsFetcher_ &fetcher);

/**********************************************************************/

struct GoogleLyricsFetcher : public LyricsFetcher
{
	virtual Result fetch(const std::string &artist, const std::string &title);
	
protected:
	virtual const char *urlTemplate() const { return URL; }
	virtual const char *siteKeyword() const { return name(); }
	
	virtual bool isURLOk(const std::string &url);
	
private:
	const char *URL;
};

struct JustSomeLyricsFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() const override { return "justsomelyrics.com"; }
	
protected:
	virtual const char *regex() const override { return "<div class=\"content.*?</div>(.*?)See also"; }
};

struct GeniusFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() const override { return "genius.com"; }

protected:
	virtual const char *regex() const override { return "<div.*?class=\"(?:lyrics|Lyrics__Container).*?>(.*?)</div>"; }
};

struct JahLyricsFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() const override { return "jah-lyrics.com"; }

protected:
	virtual const char *regex() const override { return "<div class=\"song-header\">.*?</div>(.*?)<p class=\"disclaimer\">"; }
};

struct PLyricsFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() const override { return "plyrics.com"; }

protected:
	virtual const char *regex() const override { return "<!-- start of lyrics -->(.*?)<!-- end of lyrics -->"; }
};

struct TekstowoFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() const override { return "tekstowo.pl"; }

protected:
	virtual const char *regex() const override { return "<div class=\"inner-text\">(.*?)</div>"; }
};

struct ZeneszovegFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() const override { return "zeneszoveg.hu"; }

protected:
	virtual const char *regex() const override { return "<div id=\"tartalom_slide_content\"> (.*?)<style>"; }
};

struct InternetLyricsFetcher : public GoogleLyricsFetcher
{
	virtual const char *name() const override { return "the Internet"; }
	virtual Result fetch(const std::string &artist, const std::string &title) override;
	
protected:
	virtual const char *siteKeyword() const override { return nullptr; }
	virtual const char *regex() const override { return ""; }
	
	virtual bool isURLOk(const std::string &url) override;
	
private:
	std::string URL;
};

#endif // NCMPCPP_LYRICS_FETCHER_H
