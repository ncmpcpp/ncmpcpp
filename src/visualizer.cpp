/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <fcntl.h>

#include "global.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "screen_switcher.h"
#include "status.h"
#include "enums.h"

using Global::MainStartY;
using Global::MainHeight;

Visualizer *myVisualizer;

namespace {

const int fps = 25;

}

Visualizer::Visualizer()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border::None))
{
	ResetFD();
	m_samples = 44100/fps;
	if (Config.visualizer_in_stereo)
		m_samples *= 2;
#	ifdef HAVE_FFTW3_H
	m_fftw_results = m_samples/2+1;
	m_freq_magnitudes.resize(m_fftw_results);
	m_fftw_input = static_cast<double *>(fftw_malloc(sizeof(double)*m_samples));
	m_fftw_output = static_cast<fftw_complex *>(fftw_malloc(sizeof(fftw_complex)*m_fftw_results));
	m_fftw_plan = fftw_plan_dft_r2c_1d(m_samples, m_fftw_input, m_fftw_output, FFTW_ESTIMATE);
#	endif // HAVE_FFTW3_H
}

void Visualizer::switchTo()
{
	SwitchTo::execute(this);
	w.clear();
	SetFD();
	m_timer = boost::posix_time::from_time_t(0);
	drawHeader();
}

void Visualizer::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::wstring Visualizer::title()
{
	return L"Music visualizer";
}

void Visualizer::update()
{
	if (m_fifo < 0)
		return;

	// PCM in format 44100:16:1 (for mono visualization) and 44100:16:2 (for stereo visualization) is supported
	int16_t buf[m_samples];
	ssize_t data = read(m_fifo, buf, sizeof(buf));
	if (data < 0) // no data available in fifo
		return;

	if (m_output_id != -1 && Global::Timer - m_timer > Config.visualizer_sync_interval)
	{
		Mpd.DisableOutput(m_output_id);
		usleep(50000);
		Mpd.EnableOutput(m_output_id);
		m_timer = Global::Timer;
	}

	void (Visualizer::*draw)(int16_t *, ssize_t, size_t, size_t);
	void (Visualizer::*drawStereo)(int16_t *, int16_t *, ssize_t, size_t);
#	ifdef HAVE_FFTW3_H
	if (Config.visualizer_type == VisualizerType::Spectrum)
	{
		draw = &Visualizer::DrawFrequencySpectrum;
		drawStereo = &Visualizer::DrawFrequencySpectrumStereo;
	}
	else
#	endif // HAVE_FFTW3_H
	if (Config.visualizer_type == VisualizerType::WaveFilled)
	{
		draw = &Visualizer::DrawSoundWaveFill;
		drawStereo = &Visualizer::DrawSoundWaveFillStereo;
	}
	else if (Config.visualizer_type == VisualizerType::Ellipse)
	{
		//Ellipse only works with stereo
		draw = &Visualizer::DrawSoundWave;
		drawStereo = &Visualizer::DrawSoundEllipseStereo;
	}
	else
	{
		draw = &Visualizer::DrawSoundWave;
		drawStereo = &Visualizer::DrawSoundWaveStereo;
	}

	const ssize_t samples_read = data/sizeof(int16_t);
	std::for_each(buf, buf+samples_read, [](int16_t &sample) {
		int32_t tmp = sample * Config.visualizer_sample_multiplier;
		if (tmp < std::numeric_limits<int16_t>::min())
			sample = std::numeric_limits<int16_t>::min();
		else if (tmp > std::numeric_limits<int16_t>::max())
			sample = std::numeric_limits<int16_t>::max();
		else
			sample = tmp;
	});

	w.clear();
	if (Config.visualizer_in_stereo)
	{
		auto chan_samples = samples_read/2;
		int16_t buf_left[chan_samples], buf_right[chan_samples];
		for (ssize_t i = 0, j = 0; i < samples_read; i += 2, ++j)
		{
			buf_left[j] = buf[i];
			buf_right[j] = buf[i+1];
		}
		size_t half_height = w.getHeight()/2;

		(this->*drawStereo)(buf_left, buf_right, chan_samples, half_height);
	}
	else
	{
		(this->*draw)(buf, samples_read, 0, w.getHeight());
	}
	w.refresh();
}

int Visualizer::windowTimeout()
{
	if (m_fifo >= 0 && Status::State::player() == MPD::psPlay)
		return 1000/fps;
	else
		return Screen<WindowType>::windowTimeout();
}

void Visualizer::spacePressed()
{
	std::string visualizerName;
	if (Config.visualizer_type == VisualizerType::Wave)
	{
		Config.visualizer_type = VisualizerType::WaveFilled;
		visualizerName = "sound wave filled";
	}
	else if (Config.visualizer_type == VisualizerType::WaveFilled && Config.visualizer_in_stereo)
	{
		Config.visualizer_type = VisualizerType::Ellipse;
		visualizerName = "sound ellipse";
	}
#	ifdef HAVE_FFTW3_H
	else if (Config.visualizer_type == VisualizerType::Ellipse || Config.visualizer_type == VisualizerType::WaveFilled)
	{
		Config.visualizer_type = VisualizerType::Spectrum;
		visualizerName = "frequency spectrum";
	}
#	endif // HAVE_FFTW3_H
	else
	{
		Config.visualizer_type = VisualizerType::Wave;
		visualizerName = "sound wave";
	}

	Statusbar::printf("Visualization type: %1%", visualizerName.c_str());
}

// toColor: a scaling function for coloring. For numbers 0 to max this function returns
// a coloring from the lowest color to the highest, and colors will not loop from 0 to max.
NC::Color Visualizer::toColor( size_t number, size_t max )
{
	const int colorMapSize = Config.visualizer_colors.size();
	const int normalizedNumber = ( ( number * colorMapSize ) / max ) % colorMapSize;
	return Config.visualizer_colors[normalizedNumber];
}

void Visualizer::DrawSoundWaveStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawSoundWave(buf_left, samples, 0, height);
	DrawSoundWave(buf_right, samples, height + 1, height);
}

void Visualizer::DrawSoundWaveFillStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawSoundWaveFill(buf_left, samples, 0, height);
	DrawSoundWaveFill(buf_right, samples, height + 1, height);
}

// DrawSoundEllipseStereo: This visualizer only works in stereo. The colors form concentric
// rings originating from the center (width/2, height/2). For any given point, the width is
// scaled with the left channel and height is scaled with the right channel. For example,
// if a song is entirely in the right channel, then it would just be a vertical line.
//
// Since every font/terminal is different, the visualizer is never a perfect circle. This
// visualizer assume the font height is twice the length of the font's width. If the font
// is skinner or wider than this, instead of a circle it will be an ellipse.
void Visualizer::DrawSoundEllipseStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	const long width = w.getWidth()/2;

	// Makes the radius of the color circle proportional to max of height or width.
	// Divide by colors size so that there are multiple color rings instead of just a few.
	const long scaledRadius = std::max(pow(width,2), pow(height,2))/pow(Config.visualizer_colors.size(),2);
	for (size_t i = 0; i < samples; ++i)
	{
		long x = width + ((double) buf_left[i] * 2 * ((double)width / 65536.0));
		long y = height + ((double) buf_right[i] * 2 * ((double)height / 65536.0));

		// The arguments to the toColor function roughly follow a circle equation where
		// the center is not centered around (0,0). For example (x - w)^2 + (y-h)+2 = r^2
		// centers the circle around the point (w,h). Because fonts are not all the same
		// size, this will not always generate a perfect circle.
		w << toColor(pow((x - width)*1, 2) + pow((y - ((long)height)) * 2,2), scaledRadius)
		<< NC::XY(x, y)
		<< Config.visualizer_chars[1]
		<< NC::Color::End;
	}
}

// DrawSoundWaveFill: This visualizer is very similar to DrawSoundWave, but instead of
// a single line the entire height is filled. In stereo mode, the top half of the screen
// is dedicated to the right channel, the bottom the left channel.
void Visualizer::DrawSoundWaveFill(int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
{
	const int samples_per_col = samples/w.getWidth();
	const int half_height = height/2;
	const size_t win_width = w.getWidth();
	const bool left = y_offset > 0;
	int x = 0;
	for (size_t i = 0; i < win_width; ++i)
	{
		double point_pos = 0;
		for (int j = 0; j < samples_per_col; ++j)
			point_pos += buf[i*samples_per_col+j];
		point_pos /= samples_per_col;
		point_pos /= std::numeric_limits<int16_t>::max();
		point_pos *= half_height;
		for (int k = 0; k < point_pos * 2; k += 1)
		{
			x = left ? height + k : height - k;
			if ( x > 0 && x < w.getHeight() && (i-(k < half_height + point_pos)) > 0 && (i-(k < half_height + point_pos)) < w.getWidth() )
			{
				w << toColor( k, height )
				<< NC::XY(i-(k < half_height + point_pos), x)
				<< Config.visualizer_chars[1]
				<< NC::Color::End;
			}
		}
	}
}

void Visualizer::DrawSoundWave(int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
{
	const int samples_per_col = samples/w.getWidth();
	const int half_height = height/2;
	double prev_point_pos = 0;
	const size_t win_width = w.getWidth();
	for (size_t i = 0; i < win_width; ++i)
	{
		double point_pos = 0;
		for (int j = 0; j < samples_per_col; ++j)
			point_pos += buf[i*samples_per_col+j];
		point_pos /= samples_per_col;
		point_pos /= std::numeric_limits<int16_t>::max();
		point_pos *= half_height;
		point_pos  = std::round(point_pos);

		w << NC::XY(i, y_offset+half_height+point_pos)
		<< Config.visualizer_colors[std::min(size_t(std::abs(point_pos) / (double)half_height *
											Config.visualizer_colors.size()), Config.visualizer_colors.size() - 1)]
		<< Config.visualizer_chars[0]
		<< NC::Color::End;

		if (i && abs(prev_point_pos-point_pos) > 2)
		{
			// if gap is too big. intermediate values are needed
			// since without them all we see are blinking points
			const int breakpoint = std::max(prev_point_pos, point_pos);
			const int half = (prev_point_pos+point_pos)/2;
			for (int k = std::min(prev_point_pos, point_pos)+1; k < breakpoint; k += 2)
				w << NC::XY(i-(k < half), y_offset+half_height+k)
				<< Config.visualizer_colors[std::min(size_t(std::abs(k) / (double)half_height *
													Config.visualizer_colors.size()), Config.visualizer_colors.size() - 1)]
				<< Config.visualizer_chars[0]
				<< NC::Color::End;
		}
		prev_point_pos = point_pos;
	}
}

#ifdef HAVE_FFTW3_H
void Visualizer::DrawFrequencySpectrumStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawFrequencySpectrum(buf_left, samples, 0, height);
	DrawFrequencySpectrum(buf_right, samples, height, w.getHeight() - height);
}

void Visualizer::DrawFrequencySpectrum(int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
{
	// if right channel is drawn, bars descend from the top to the bottom
	const bool flipped = y_offset > 0;

	// copy samples to fftw input array
	for (unsigned i = 0; i < m_samples; ++i)
		m_fftw_input[i] = i < samples ? buf[i] : 0;
	fftw_execute(m_fftw_plan);

	// count magnitude of each frequency and scale it to fit the screen
	for (size_t i = 0; i < m_fftw_results; ++i)
		m_freq_magnitudes[i] = sqrt(
			m_fftw_output[i][0]*m_fftw_output[i][0]
		+	m_fftw_output[i][1]*m_fftw_output[i][1]
		)/2e4*height;

	const size_t win_width = w.getWidth();
	// cut bandwidth a little to achieve better look
	const double bins_per_bar = m_fftw_results/win_width * 7/10;
	double bar_height;
	size_t bar_bound_height;
	for (size_t x = 0; x < win_width; ++x)
	{
		bar_height = 0;
		for (int j = 0; j < bins_per_bar; ++j)
			bar_height += m_freq_magnitudes[x*bins_per_bar+j];
		// buff higher frequencies
		bar_height *= log2(2 + x);
		// moderately normalize the heights
		bar_height = pow(bar_height, 0.5);

		bar_bound_height = std::min(std::size_t(bar_height/bins_per_bar), height);
		for (size_t j = 0; j < bar_bound_height; ++j)
		{
			size_t y = flipped ? y_offset+j : y_offset+height-j-1;
			w << NC::XY(x, y)
			<< toColor(j, height)
			<< Config.visualizer_chars[1]
			<< NC::Color::End;
		}
	}
}
#endif // HAVE_FFTW3_H

void Visualizer::SetFD()
{
	if (m_fifo < 0 && (m_fifo = open(Config.visualizer_fifo_path.c_str(), O_RDONLY | O_NONBLOCK)) < 0)
		Statusbar::printf("Couldn't open \"%1%\" for reading PCM data: %2%",
			Config.visualizer_fifo_path, strerror(errno)
		);
}

void Visualizer::ResetFD()
{
	m_fifo = -1;
}

void Visualizer::FindOutputID()
{
	m_output_id = -1;
	if (!Config.visualizer_output_name.empty())
	{
		size_t idx = 0;
		Mpd.GetOutputs([this, &idx](MPD::Output output) {
			if (output.name() == Config.visualizer_output_name)
				m_output_id = idx;
			++idx;
		});
		if (m_output_id == -1)
			Statusbar::printf("There is no output named \"%s\"", Config.visualizer_output_name);
	}
}

#endif // ENABLE_VISUALIZER

/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
