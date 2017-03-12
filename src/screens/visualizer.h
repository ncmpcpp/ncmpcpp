/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_VISUALIZER_H
#define NCMPCPP_VISUALIZER_H

#include "config.h"

#ifdef ENABLE_VISUALIZER

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "curses/window.h"
#include "interfaces.h"
#include "screens/screen.h"

#ifdef HAVE_FFTW3_H
# include <fftw3.h>
#endif

struct Visualizer: Screen<NC::Window>, Tabbable
{
	Visualizer();

	virtual void switchTo() override;
	virtual void resize() override;

	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::Visualizer; }

	virtual void update() override;
	virtual void scroll(NC::Scroll) override { }

	virtual int windowTimeout() override;

	virtual void mouseButtonPressed(MEVENT) override { }

	virtual bool isLockable() override { return true; }
	virtual bool isMergable() override { return true; }

	// private members
	void ToggleVisualizationType();
	void SetFD();
	void ResetFD();
	void FindOutputID();
	void ResetAutoScaleMultiplier();

private:
	void DrawSoundWave(int16_t *, ssize_t, size_t, size_t);
	void DrawSoundWaveStereo(int16_t *, int16_t *, ssize_t, size_t);
	void DrawSoundWaveFill(int16_t *, ssize_t, size_t, size_t);
	void DrawSoundWaveFillStereo(int16_t *, int16_t *, ssize_t, size_t);
	void DrawSoundEllipse(int16_t *, ssize_t, size_t, size_t);
	void DrawSoundEllipseStereo(int16_t *, int16_t *, ssize_t, size_t);
#	ifdef HAVE_FFTW3_H
	void DrawFrequencySpectrum(int16_t *, ssize_t, size_t, size_t);
	void DrawFrequencySpectrumStereo(int16_t *, int16_t *, ssize_t, size_t);
#	endif // HAVE_FFTW3_H

	int m_output_id;
	boost::posix_time::ptime m_timer;

	int m_fifo;
	size_t m_samples;
	double m_auto_scale_multiplier;
#	ifdef HAVE_FFTW3_H
	size_t m_fftw_results;
	double *m_fftw_input;
	fftw_complex *m_fftw_output;
	fftw_plan m_fftw_plan;

	std::vector<double> m_freq_magnitudes;
#	endif // HAVE_FFTW3_H
};

extern Visualizer *myVisualizer;

#endif // ENABLE_VISUALIZER

#endif // NCMPCPP_VISUALIZER_H

/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
