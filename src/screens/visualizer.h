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

#ifndef NCMPCPP_VISUALIZER_H
#define NCMPCPP_VISUALIZER_H

#include "config.h"

#ifdef ENABLE_VISUALIZER

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "curses/window.h"
#include "interfaces.h"
#include "screens/screen.h"
#include "utility/sample_buffer.h"

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

	void Clear();
	void OpenDataSource();
	void CloseDataSource();

	void ToggleVisualizationType();
	void FindOutputID();
	void ResetAutoScaleMultiplier();

private:
	void DrawSoundWave(const int16_t *, ssize_t, size_t, size_t);
	void DrawSoundWaveStereo(const int16_t *, const int16_t *, ssize_t, size_t);
	void DrawSoundWaveFill(const int16_t *, ssize_t, size_t, size_t);
	void DrawSoundWaveFillStereo(const int16_t *, const int16_t *, ssize_t, size_t);
	void DrawSoundEllipse(const int16_t *, ssize_t, size_t, size_t);
	void DrawSoundEllipseStereo(const int16_t *, const int16_t *, ssize_t, size_t);
#	ifdef HAVE_FFTW3_H
	void DrawFrequencySpectrum(const int16_t *, ssize_t, size_t, size_t);
	void DrawFrequencySpectrumStereo(const int16_t *, const int16_t *, ssize_t, size_t);
	void ApplyWindow(double *, const int16_t *, ssize_t);
	void GenLogspace();
	void GenLinspace();
	void GenFreqSpace();
	double Bin2Hz(size_t);
	double InterpolateCubic(size_t, size_t);
	double InterpolateLinear(size_t, size_t);
#	endif // HAVE_FFTW3_H

	void InitDataSource();
	void InitVisualization();

	void (Visualizer::*draw)(const int16_t *, ssize_t, size_t, size_t);
	void (Visualizer::*drawStereo)(const int16_t *, const int16_t *, ssize_t, size_t);

	int m_output_id;
	bool m_reset_output;

	int m_source_fd;
	std::string m_source_location;
	std::string m_source_port;

	std::vector<int16_t> m_rendered_samples;
	std::vector<int16_t> m_incoming_samples;
	SampleBuffer m_buffered_samples;
	size_t m_sample_consumption_rate;
	size_t m_sample_consumption_rate_up_ctr;
	size_t m_sample_consumption_rate_dn_ctr;

	double m_auto_scale_multiplier;
#	ifdef HAVE_FFTW3_H
	size_t m_fftw_results;
	double *m_fftw_input;
	fftw_complex *m_fftw_output;
	fftw_plan m_fftw_plan;
	const uint32_t DFT_NONZERO_SIZE;
	const uint32_t DFT_TOTAL_SIZE;
	const double DYNAMIC_RANGE;
	const double HZ_MIN;
	const double HZ_MAX;
	const double GAIN;
	const std::wstring SMOOTH_CHARS;
	const std::wstring SMOOTH_CHARS_FLIPPED;
	std::vector<double> m_dft_freqspace;
	std::vector<std::pair<size_t, double>> m_bar_heights;

	std::vector<double> m_freq_magnitudes;
#	endif // HAVE_FFTW3_H
};

extern Visualizer *myVisualizer;

#endif // ENABLE_VISUALIZER

#endif // NCMPCPP_VISUALIZER_H

/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
