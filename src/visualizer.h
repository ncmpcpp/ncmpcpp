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

#ifndef _VISUALIZER_H
#define _VISUALIZER_H

#include "config.h"

#ifdef ENABLE_VISUALIZER

#include "window.h"
#include "screen.h"

#ifdef HAVE_FFTW3_H
# include <fftw3.h>
#endif

struct Visualizer : public Screen<NC::Window>
{
	Visualizer();
	
	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	
	virtual void update() OVERRIDE;
	virtual void scroll(NC::Where) OVERRIDE { }
	
	virtual void enterPressed() OVERRIDE { }
	virtual void spacePressed() OVERRIDE;
	virtual void mouseButtonPressed(MEVENT) OVERRIDE { }
	
	virtual bool isTabbable() OVERRIDE { return true; }
	virtual bool isMergable() OVERRIDE { return true; }
	
	// private members
	void SetFD();
	void ResetFD();
	void FindOutputID();
	
	static const int WindowTimeout;
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
private:
	void DrawSoundWave(int16_t *, ssize_t, size_t, size_t);
#	ifdef HAVE_FFTW3_H
	void DrawFrequencySpectrum(int16_t *, ssize_t, size_t, size_t);
#	endif // HAVE_FFTW3_H
	
	int m_output_id;
	timeval m_timer;
	
	int m_fifo;
	unsigned m_samples;
#	ifdef HAVE_FFTW3_H
	unsigned m_fftw_results;
	unsigned *m_freq_magnitudes;
	double *m_fftw_input;
	fftw_complex *m_fftw_output;
	fftw_plan m_fftw_plan;
#	endif // HAVE_FFTW3_H
};

extern Visualizer *myVisualizer;

#endif // ENABLE_VISUALIZER

#endif

