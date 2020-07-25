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

#include "screens/visualizer.h"

#ifdef ENABLE_VISUALIZER

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/math/constants/constants.hpp>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <fcntl.h>
#include <cassert>

#include "global.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "screens/screen_switcher.h"
#include "status.h"
#include "enums.h"

using Samples = std::vector<int16_t>;

using Global::MainStartY;
using Global::MainHeight;

Visualizer *myVisualizer;

namespace {

const int fps = 25;

// toColor: a scaling function for coloring. For numbers 0 to max this function
// returns a coloring from the lowest color to the highest, and colors will not
// loop from 0 to max.
const NC::FormattedColor &toColor(size_t number, size_t max, bool wrap = true)
{
	const auto colors_size = Config.visualizer_colors.size();
	const auto index = (number * colors_size) / max;
	return Config.visualizer_colors[
		wrap ? index % colors_size : std::min(index, colors_size-1)
	];
}

}

Visualizer::Visualizer()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border()))
{
	ResetFD();
#	ifdef HAVE_FFTW3_H
	read_samples = DFT_SIZE - DFT_PAD;
#	else
	read_samples = 44100 / fps;
#	endif // HAVE_FFTW3_H
	if (Config.visualizer_in_stereo)
		read_samples *= 2;
	sample_buffer.resize(read_samples);
	temp_sample_buffer.resize(read_samples);
	memset(sample_buffer.data(), 0, sample_buffer.size()*sizeof(int16_t));
	memset(temp_sample_buffer.data(), 0, sample_buffer.size()*sizeof(int16_t));

#	ifdef HAVE_FFTW3_H
	m_fftw_results = DFT_SIZE/2+1;
	m_freq_magnitudes.resize(m_fftw_results);
	m_fftw_input = static_cast<double *>(fftw_malloc(sizeof(double)*DFT_SIZE));
	memset(m_fftw_input, 0, sizeof(double)*DFT_SIZE);
	m_fftw_output = static_cast<fftw_complex *>(fftw_malloc(sizeof(fftw_complex)*m_fftw_results));
	m_fftw_plan = fftw_plan_dft_r2c_1d(DFT_SIZE, m_fftw_input, m_fftw_output, FFTW_ESTIMATE);
	dft_logspace.reserve(500);
#	endif // HAVE_FFTW3_H
}

void Visualizer::switchTo()
{
	SwitchTo::execute(this);
	w.clear();
	SetFD();
	m_timer = boost::posix_time::from_time_t(0);
	drawHeader();
	memset(sample_buffer.data(), 0, sample_buffer.size()*sizeof(int16_t));
#	ifdef HAVE_FFTW3_H
	GenLogspace();
#	endif // HAVE_FFTW3_H
}

void Visualizer::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
#	ifdef HAVE_FFTW3_H
	GenLogspace();
#	endif // HAVE_FFTW3_H
}

std::wstring Visualizer::title()
{
	return L"Music visualizer";
}

void Visualizer::update()
{
	if (m_fifo < 0)
		return;

	// PCM in format 44100:16:1 (for mono visualization) and
	// 44100:16:2 (for stereo visualization) is supported.
	const int buf_size = sizeof(int16_t)*read_samples;
	ssize_t data = read(m_fifo, temp_sample_buffer.data(), buf_size);
	if (data <= 0) // no data available in fifo
		return;

	{
		// create int8_t pointers for arithmetic
		int8_t *const sdata = (int8_t *)sample_buffer.data();
		int8_t *const temp_sdata = (int8_t *)temp_sample_buffer.data();
		int8_t *const sdata_end = sdata + buf_size;
		memmove(sdata, sdata + data, buf_size - data);
		memcpy(sdata_end - data, temp_sdata, data);
	}

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
		draw = &Visualizer::DrawSoundEllipse;
		drawStereo = &Visualizer::DrawSoundEllipseStereo;
	}
	else
	{
		draw = &Visualizer::DrawSoundWave;
		drawStereo = &Visualizer::DrawSoundWaveStereo;
	}

#	ifdef HAVE_FFTW3_H
	// don't scale for spectrum visualizer
	if (Config.visualizer_type != VisualizerType::Spectrum)
#	endif // HAVE_FFTW3_H
	{
		m_auto_scale_multiplier += 1.0/fps;
		for (auto &sample : sample_buffer)
		{
			double scale = std::numeric_limits<int16_t>::min();
			scale /= sample;
			scale = fabs(scale);
			if (scale < m_auto_scale_multiplier)
				m_auto_scale_multiplier = scale;
		}
		for (auto &sample : sample_buffer)
		{
			int32_t tmp = sample;
			if (m_auto_scale_multiplier <= 50.0) // limit the auto scale
				tmp *= m_auto_scale_multiplier;
			if (tmp < std::numeric_limits<int16_t>::min())
				sample = std::numeric_limits<int16_t>::min();
			else if (tmp > std::numeric_limits<int16_t>::max())
				sample = std::numeric_limits<int16_t>::max();
			else
				sample = tmp;
		}
	}

	w.clear();
	if (Config.visualizer_in_stereo)
	{
		auto chan_samples = read_samples/2;
		int16_t buf_left[chan_samples], buf_right[chan_samples];
		for (ssize_t i = 0, j = 0; i < read_samples; i += 2, ++j)
		{
			buf_left[j] = sample_buffer[i];
			buf_right[j] = sample_buffer[i+1];
		}
		size_t half_height = w.getHeight()/2;

		(this->*drawStereo)(buf_left, buf_right, chan_samples, half_height);
	}
	else
	{
		(this->*draw)(sample_buffer.data(), read_samples, 0, w.getHeight());
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

/**********************************************************************/

void Visualizer::DrawSoundWave(int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
{
	const size_t half_height = height/2;
	const size_t base_y = y_offset+half_height;
	const size_t win_width = w.getWidth();
	const int samples_per_column = samples/win_width;

	// too little samples
	if (samples_per_column == 0)
		return;

	auto draw_point = [&](size_t x, int32_t y) {
		auto c = toColor(std::abs(y), half_height, false);
		w << NC::XY(x, base_y+y)
		  << c
		  << Config.visualizer_chars[0]
		  << NC::FormattedColor::End<>(c);
	};

	int32_t point_y, prev_point_y = 0;
	for (size_t x = 0; x < win_width; ++x)
	{
		point_y = 0;
		// calculate mean from the relevant points
		for (int j = 0; j < samples_per_column; ++j)
			point_y += buf[x*samples_per_column+j];
		point_y /= samples_per_column;
		// normalize it to fit the screen
		point_y *= height / 65536.0;

		draw_point(x, point_y);

		// if the gap between two consecutive points is too big,
		// intermediate values are needed for the wave to be watchable.
		if (x > 0 && std::abs(prev_point_y-point_y) > 1)
		{
			const int32_t half = (prev_point_y+point_y)/2;
			if (prev_point_y < point_y)
			{
				for (auto y = prev_point_y; y < point_y; ++y)
					draw_point(x-(y < half), y);
			}
			else
			{
				for (auto y = prev_point_y; y > point_y; --y)
					draw_point(x-(y > half), y);
			}
		}
		prev_point_y = point_y;
	}
}

void Visualizer::DrawSoundWaveStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawSoundWave(buf_left, samples, 0, height);
	DrawSoundWave(buf_right, samples, height, w.getHeight() - height);
}

/**********************************************************************/

// DrawSoundWaveFill: This visualizer is very similar to DrawSoundWave, but
// instead of a single line the entire height is filled. In stereo mode, the top
// half of the screen is dedicated to the right channel, the bottom the left
// channel.
void Visualizer::DrawSoundWaveFill(int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
{
	// if right channel is drawn, bars descend from the top to the bottom
	const bool flipped = y_offset > 0;
	const size_t win_width = w.getWidth();
	const int samples_per_column = samples/win_width;

	// too little samples
	if (samples_per_column == 0)
		return;

	int32_t point_y;
	for (size_t x = 0; x < win_width; ++x)
	{
		point_y = 0;
		// calculate mean from the relevant points
		for (int j = 0; j < samples_per_column; ++j)
			point_y += buf[x*samples_per_column+j];
		point_y /= samples_per_column;
		// normalize it to fit the screen
		point_y = std::abs(point_y);
		point_y *= height / 32768.0;

		for (int32_t j = 0; j < point_y; ++j)
		{
			auto c = toColor(j, height);
			size_t y = flipped ? y_offset+j : y_offset+height-j-1;
			w << NC::XY(x, y)
			  << c
			  << Config.visualizer_chars[1]
			  << NC::FormattedColor::End<>(c);
		}
	}
}

void Visualizer::DrawSoundWaveFillStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawSoundWaveFill(buf_left, samples, 0, height);
	DrawSoundWaveFill(buf_right, samples, height, w.getHeight() - height);
}

/**********************************************************************/

// Draws the sound wave as an ellipse with origin in the center of the screen.
void Visualizer::DrawSoundEllipse(int16_t *buf, ssize_t samples, size_t, size_t height)
{
	const size_t half_width = w.getWidth()/2;
	const size_t half_height = height/2;

	// Make it so that the loop goes around the ellipse exactly once.
	const double deg_multiplier = 2*boost::math::constants::pi<double>()/samples;

	int32_t x, y;
	double radius, max_radius;
	for (ssize_t i = 0; i < samples; ++i)
	{
		x = half_width * std::cos(i*deg_multiplier);
		y = half_height * std::sin(i*deg_multiplier);
		max_radius = sqrt(x*x + y*y);

		// Calculate the distance of the sample from the center, where 0 is the
		// center of the ellipse and 1 is its border.
		radius = std::abs(buf[i]);
		radius /= 32768.0;

		// Appropriately scale the position.
		x *= radius;
		y *= radius;

		auto c = toColor(sqrt(x*x + y*y), max_radius, false);
		w << NC::XY(half_width + x, half_height + y)
		  << c
		  << Config.visualizer_chars[0]
		  << NC::FormattedColor::End<>(c);
	}
}

// DrawSoundEllipseStereo: This visualizer only works in stereo. The colors form
// concentric rings originating from the center (width/2, height/2). For any
// given point, the width is scaled with the left channel and height is scaled
// with the right channel. For example, if a song is entirely in the right
// channel, then it would just be a vertical line.
//
// Since every font/terminal is different, the visualizer is never a perfect
// circle. This visualizer assume the font height is twice the length of the
// font's width. If the font is skinner or wider than this, instead of a circle
// it will be an ellipse.
void Visualizer::DrawSoundEllipseStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t half_height)
{
	const size_t width = w.getWidth();
	const size_t left_half_width = width/2;
	const size_t right_half_width = width - left_half_width;
	const size_t top_half_height = half_height;
	const size_t bottom_half_height = w.getHeight() - half_height;

	// Makes the radius of each ring be approximately 2 cells wide.
	const int32_t radius = 2*Config.visualizer_colors.size();
	int32_t x, y;
	for (ssize_t i = 0; i < samples; ++i)
	{
		x = buf_left[i]/32768.0 * (buf_left[i] < 0 ? left_half_width : right_half_width);
		y = buf_right[i]/32768.0 * (buf_right[i] < 0 ? top_half_height : bottom_half_height);

		// The arguments to the toColor function roughly follow a circle equation
		// where the center is not centered around (0,0). For example (x - w)^2 +
		// (y-h)+2 = r^2 centers the circle around the point (w,h). Because fonts
		// are not all the same size, this will not always generate a perfect
		// circle.
		auto c = toColor(sqrt(x*x + 4*y*y), radius);
		w << NC::XY(left_half_width + x, top_half_height + y)
		  << c
		  << Config.visualizer_chars[1]
		  << NC::FormattedColor::End<>(c);
	}
}

/**********************************************************************/

#ifdef HAVE_FFTW3_H
void Visualizer::DrawFrequencySpectrum(int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
{
	// If right channel is drawn, bars descend from the top to the bottom.
	const bool flipped = y_offset > 0;

	// copy samples to fftw input array and apply Hamming window
	ApplyWindow(m_fftw_input, buf, samples);
	fftw_execute(m_fftw_plan);

	// Count magnitude of each frequency and normalize
	for (size_t i = 0; i < m_fftw_results; ++i)
		m_freq_magnitudes[i] = sqrt(
			m_fftw_output[i][0]*m_fftw_output[i][0]
		+	m_fftw_output[i][1]*m_fftw_output[i][1]
		) / (DFT_SIZE - DFT_PAD);

	const size_t win_width = w.getWidth();

	double prev_bar_height = 0;
	size_t cur_bin = 0;
	for (size_t x = 0; x < win_width; ++x)
	{
		double bar_height = 0;

		// accumulate bins
		size_t count = 0;
		// check right bound
		while (cur_bin < m_fftw_results && Bin2Hz(cur_bin) < dft_logspace[x])
		{
			// check left bound if not first index
			if (x == 0 || Bin2Hz(cur_bin) >= dft_logspace[x-1])
			{
				bar_height += m_freq_magnitudes[cur_bin];
				++count;
			}
			++cur_bin;
		}

		// average bins, using previous bin if not enough data
		if (x > 0 && count == 0)
		{
			bar_height = prev_bar_height;
		}
		else
		{
			bar_height /= count;
		}
		prev_bar_height = bar_height;

		// log scale bar heights
		bar_height = (20 * log10(bar_height) + DYNAMIC_RANGE + GAIN) / DYNAMIC_RANGE;
		// Scale bar height between 0 and height
		bar_height = bar_height > 0 ? bar_height * height : 0;
		bar_height = bar_height > height ? height : bar_height;

		for (size_t j = 0; j < bar_height; ++j)
		{
			size_t y = flipped ? y_offset+j : y_offset+height-j-1;
			auto c = toColor(j, height);
      std::wstring ch;
      if (Config.visualizer_use_frac_look) {
        if (j < bar_height-1) {
          ch = Config.visualizer_frac_chars[7];
        } else {
          ch = Config.visualizer_frac_chars[static_cast<int>(8*bar_height) % 8];
        }
      } else  {
        ch = Config.visualizer_chars[1];
      }
      w << NC::XY(x, y)
        << c
        << ch
        << NC::FormattedColor::End<>(c);
		}
	}
}

void Visualizer::DrawFrequencySpectrumStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawFrequencySpectrum(buf_left, samples, 0, height);
	DrawFrequencySpectrum(buf_right, samples, height, w.getHeight() - height);
}

void Visualizer::ApplyWindow(double *output, int16_t *input, ssize_t samples)
{
	// Use Blackman window for low sidelobes and fast sidelobe rolloff
	// don't care too much about mainlobe width
	const double alpha = 0.16;
	const double a0 = (1 - alpha) / 2;
	const double a1 = 0.5;
	const double a2 = alpha / 2;
	const double pi = boost::math::constants::pi<double>();
	for (unsigned i = 0; i < samples; ++i)
	{
		double window = a0 - a1*cos(2*pi*i/(DFT_SIZE-DFT_PAD-1)) + a2*cos(4*pi*i/(DFT_SIZE-DFT_PAD-1));
		output[i] = window * input[i] / INT16_MAX;
	}
}

double Visualizer::Bin2Hz(size_t bin)
{
	return bin*44100/DFT_SIZE;
}

// Generate log-scaled vector of frequencies from HZ_MIN to HZ_MAX
void Visualizer::GenLogspace()
{
	// Calculate number of extra bins needed between 0 HZ and HZ_MIN
	const size_t win_width = w.getWidth();
	const size_t left_bins = (log10(HZ_MIN) - win_width*log10(HZ_MIN)) / (log10(HZ_MIN) - log10(HZ_MAX));
	// Generate logspaced frequencies
	dft_logspace.resize(win_width);
	const double log_scale = log10(HZ_MAX) / (left_bins + dft_logspace.size() - 1);
	for (int i = left_bins; i < dft_logspace.size() + left_bins; ++i) {
		dft_logspace[i - left_bins] = pow(10, i * log_scale);
	}
}
#endif // HAVE_FFTW3_H

/**********************************************************************/

void Visualizer::ToggleVisualizationType()
{
	switch (Config.visualizer_type)
	{
		case VisualizerType::Wave:
			Config.visualizer_type = VisualizerType::WaveFilled;
			break;
		case VisualizerType::WaveFilled:
#			ifdef HAVE_FFTW3_H
			Config.visualizer_type = VisualizerType::Spectrum;
#			else
			Config.visualizer_type = VisualizerType::Ellipse;
#			endif // HAVE_FFTW3_H
			break;
#		ifdef HAVE_FFTW3_H
		case VisualizerType::Spectrum:
			Config.visualizer_type = VisualizerType::Ellipse;
			break;
#		endif // HAVE_FFTW3_H
		case VisualizerType::Ellipse:
			Config.visualizer_type = VisualizerType::Wave;
			break;
	}
	Statusbar::printf("Visualization type: %1%", Config.visualizer_type);
}

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
		for (MPD::OutputIterator out = Mpd.GetOutputs(), end; out != end; ++out)
		{
			if (out->name() == Config.visualizer_output_name)
			{
				m_output_id = out->id();
				break;
			}
		}
		if (m_output_id == -1)
			Statusbar::printf("There is no output named \"%s\"", Config.visualizer_output_name);
	}
}

void Visualizer::ResetAutoScaleMultiplier()
{
	m_auto_scale_multiplier = std::numeric_limits<double>::infinity();
}

#endif // ENABLE_VISUALIZER
