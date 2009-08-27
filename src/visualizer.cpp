/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include "visualizer.h"

#ifdef ENABLE_VISUALIZER

#include "global.h"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <fcntl.h>

using Global::myScreen;
using Global::MainStartY;
using Global::MainHeight;

Visualizer *myVisualizer = new Visualizer;

const unsigned Visualizer::Samples = 2048;
const unsigned Visualizer::FFTResults = Samples/2+1;

void Visualizer::Init()
{
	w = new Window(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	w->SetTimeout(Config.visualizer_fifo_path.empty() ? ncmpcpp_window_timeout : 40 /* this gives us 25 fps */);
	
	itsFifo = -1;
	itsFreqsMagnitude = new unsigned[FFTResults];
	itsInput = static_cast<double *>(fftw_malloc(sizeof(double)*Samples));
	itsOutput = static_cast<fftw_complex *>(fftw_malloc(sizeof(fftw_complex)*FFTResults));
	itsPlan = fftw_plan_dft_r2c_1d(Samples, itsInput, itsOutput, FFTW_ESTIMATE);
	
	isInitialized = 1;
}

void Visualizer::SwitchTo()
{
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (hasToBeResized)
		Resize();
	
	myScreen = this;
	w->Clear();
	
	if (itsFifo < 0 && (itsFifo = open(Config.visualizer_fifo_path.c_str(), O_RDONLY | O_NONBLOCK)) < 0)
		ShowMessage("Couldn't open fifo for reading PCM data: %s", strerror(errno));
	
	Global::RedrawHeader = 1;
}

void Visualizer::Resize()
{
	w->Resize(COLS, MainHeight);
	w->MoveTo(0, MainStartY);
	hasToBeResized = 0;
}

std::basic_string<my_char_t> Visualizer::Title()
{
	return U("Music visualizer");
}

void Visualizer::Update()
{
	if (itsFifo < 0)
		return;
	
	// if mpd is stopped, clear the screen
	if (Mpd.GetState() < MPD::psPlay)
	{
		w->Clear();
		return;
	}
	
	// it supports only PCM in format 44100:16:1
	static int16_t buf[Samples];
	ssize_t data = read(itsFifo, buf, sizeof(buf));
	if (data < 0) // no data available in fifo
		return;
	
	// zero old values
	std::fill(buf+data/sizeof(int16_t), buf+Samples, 0);
	for (unsigned i = 0; i < Samples; ++i)
		itsInput[i] = buf[i];
	
	fftw_execute(itsPlan);
	
	// count magnitude of each frequency and scale it to fit the screen
	for (unsigned i = 0; i < FFTResults; ++i)
		itsFreqsMagnitude[i] = sqrt(itsOutput[i][0]*itsOutput[i][0] + itsOutput[i][1]*itsOutput[i][1])/1e5*LINES/5;
	
	w->Clear(0);
	const int freqs_per_col = FFTResults/COLS /* cut bandwidth a little to achieve better look */ * 4/5;
	for (int i = 0; i < COLS; ++i)
	{
		size_t bar_height = 0;
		for (int j = 0; j < freqs_per_col; ++j)
			bar_height += itsFreqsMagnitude[i*freqs_per_col+j];
		bar_height = std::min(bar_height/freqs_per_col, Global::MainHeight);
		mvwvline(w->Raw(), Global::MainHeight-bar_height, i, 0, bar_height);
	}
	w->Refresh();
}

#endif // ENABLE_VISUALIZER

