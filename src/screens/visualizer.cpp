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
#include <netdb.h>
#include <cassert>

#include "global.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "screens/screen_switcher.h"
#include "status.h"
#include "enums.h"
#include "utility/wide_string.h"

using Samples = std::vector<int16_t>;

using Global::MainStartY;
using Global::MainHeight;

Visualizer *myVisualizer;

namespace {

// toColor: a scaling function for coloring. For numbers 0 to max this function
// returns a coloring from the lowest color to the highest, and colors will not
// loop from 0 to max.
const NC::FormattedColor &toColor(size_t number, size_t max, bool wrap)
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
, m_output_id(-1)
, m_reset_output(false)
, m_source_fd(-1)
, m_sample_consumption_rate(5)
, m_sample_consumption_rate_up_ctr(0)
, m_sample_consumption_rate_dn_ctr(0)
#	ifdef HAVE_FFTW3_H
	,
  DFT_NONZERO_SIZE(2048 * (2*Config.visualizer_spectrum_dft_size + 4)),
  DFT_TOTAL_SIZE(1 << 15),
  DYNAMIC_RANGE(100-Config.visualizer_spectrum_gain),
  HZ_MIN(Config.visualizer_spectrum_hz_min),
  HZ_MAX(Config.visualizer_spectrum_hz_max),
  GAIN(Config.visualizer_spectrum_gain),
  SMOOTH_CHARS(ToWString("▁▂▃▄▅▆▇█"))
#endif
{
	InitDataSource();
	InitVisualization();
#	ifdef HAVE_FFTW3_H
	m_fftw_results = DFT_TOTAL_SIZE/2+1;
	m_freq_magnitudes.resize(m_fftw_results);
	m_fftw_input = static_cast<double *>(fftw_malloc(sizeof(double)*DFT_TOTAL_SIZE));
	memset(m_fftw_input, 0, sizeof(double)*DFT_TOTAL_SIZE);
	m_fftw_output = static_cast<fftw_complex *>(fftw_malloc(sizeof(fftw_complex)*m_fftw_results));
	m_fftw_plan = fftw_plan_dft_r2c_1d(DFT_TOTAL_SIZE, m_fftw_input, m_fftw_output, FFTW_ESTIMATE);
	m_dft_logspace.reserve(500);
	m_bar_heights.reserve(100);
#	endif // HAVE_FFTW3_H
}

void Visualizer::switchTo()
{
	SwitchTo::execute(this);
	Clear();
	m_reset_output = true;
	drawHeader();
#	ifdef HAVE_FFTW3_H
	GenLogspace();
	m_bar_heights.reserve(w.getWidth());
#	endif // HAVE_FFTW3_H
}

void Visualizer::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
	InitVisualization();
#	ifdef HAVE_FFTW3_H
	GenLogspace();
	m_bar_heights.reserve(w.getWidth());
#	endif // HAVE_FFTW3_H
}

std::wstring Visualizer::title()
{
	return L"Music visualizer";
}

void Visualizer::update()
{
	if (m_source_fd < 0)
		return;

	// Disable and enable FIFO to get rid of the difference between audio and
	// visualization.
	if (m_reset_output && m_output_id != -1)
	{
		Mpd.DisableOutput(m_output_id);
		usleep(50000);
		Mpd.EnableOutput(m_output_id);
		m_reset_output = false;
	}

	// PCM in format 44100:16:1 (for mono visualization) and
	// 44100:16:2 (for stereo visualization) is supported.
	ssize_t bytes_read = read(m_source_fd, m_incoming_samples.data(),
	                          sizeof(int16_t) * m_incoming_samples.size());
	if (bytes_read > 0)
	{
		const auto begin = m_incoming_samples.begin();
		const auto end = m_incoming_samples.begin() + bytes_read/sizeof(int16_t);

		if (Config.visualizer_autoscale)
		{
			m_auto_scale_multiplier += 1.0/Config.visualizer_fps;
			for (auto sample = begin; sample != end; ++sample)
			{
				double scale = std::numeric_limits<int16_t>::min();
				scale /= *sample;
				scale = fabs(scale);
				if (scale < m_auto_scale_multiplier)
					m_auto_scale_multiplier = scale;
			}
			for (auto sample = begin; sample != end; ++sample)
			{
				int32_t tmp = *sample;
				if (m_auto_scale_multiplier <= 50.0) // limit the auto scale
					tmp *= m_auto_scale_multiplier;
				if (tmp < std::numeric_limits<int16_t>::min())
					*sample = std::numeric_limits<int16_t>::min();
				else if (tmp > std::numeric_limits<int16_t>::max())
					*sample = std::numeric_limits<int16_t>::max();
				else
					*sample = tmp;
			}
		}
		m_buffered_samples.put(begin, end);
	}

	size_t requested_samples =
		44100.0 / Config.visualizer_fps * pow(1.1, m_sample_consumption_rate);
	if (Config.visualizer_in_stereo)
		requested_samples *= 2;

	//Statusbar::printf("Samples: %1%, %2%, %3%", m_buffered_samples.size(),
	//                  requested_samples, m_sample_consumption_rate);

	size_t new_samples = m_buffered_samples.get(requested_samples, m_rendered_samples);
	if (new_samples == 0)
		return;

	// A crude way to adjust the amount of samples consumed from the buffer
	// depending on how fast the rendering is.
	if (m_buffered_samples.size() > 0)
	{
		if (++m_sample_consumption_rate_up_ctr > 8)
		{
			m_sample_consumption_rate_up_ctr = 0;
			++m_sample_consumption_rate;
		}
	}
	else if (m_sample_consumption_rate > 0)
	{
		if (++m_sample_consumption_rate_dn_ctr > 4)
		{
			m_sample_consumption_rate_dn_ctr = 0;
			--m_sample_consumption_rate;
		}
		m_sample_consumption_rate_up_ctr = 0;
	}

	w.clear();
	if (Config.visualizer_in_stereo)
	{
		auto chan_samples = m_rendered_samples.size()/2;
		int16_t buf_left[chan_samples], buf_right[chan_samples];
		for (size_t i = 0, j = 0; i < m_rendered_samples.size(); i += 2, ++j)
		{
			buf_left[j] = m_rendered_samples[i];
			buf_right[j] = m_rendered_samples[i+1];
		}
		size_t half_height = w.getHeight()/2;

		(this->*drawStereo)(buf_left, buf_right, chan_samples, half_height);
	}
	else
	{
		(this->*draw)(m_rendered_samples.data(), m_rendered_samples.size(), 0, w.getHeight());
	}
	w.refresh();
}

int Visualizer::windowTimeout()
{
	if (m_source_fd >= 0 && Status::State::player() == MPD::psPlay)
		return 1000/Config.visualizer_fps;
	else
		return Screen<WindowType>::windowTimeout();
}

/**********************************************************************/

void Visualizer::DrawSoundWave(const int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
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

void Visualizer::DrawSoundWaveStereo(const int16_t *buf_left, const int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawSoundWave(buf_left, samples, 0, height);
	DrawSoundWave(buf_right, samples, height, w.getHeight() - height);
}

/**********************************************************************/

// DrawSoundWaveFill: This visualizer is very similar to DrawSoundWave, but
// instead of a single line the entire height is filled. In stereo mode, the top
// half of the screen is dedicated to the right channel, the bottom the left
// channel.
void Visualizer::DrawSoundWaveFill(const int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
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
			auto c = toColor(j, height, false);
			size_t y = flipped ? y_offset+j : y_offset+height-j-1;
			w << NC::XY(x, y)
			  << c
			  << Config.visualizer_chars[1]
			  << NC::FormattedColor::End<>(c);
		}
	}
}

void Visualizer::DrawSoundWaveFillStereo(const int16_t *buf_left, const int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawSoundWaveFill(buf_left, samples, 0, height);
	DrawSoundWaveFill(buf_right, samples, height, w.getHeight() - height);
}

/**********************************************************************/

// Draws the sound wave as an ellipse with origin in the center of the screen.
void Visualizer::DrawSoundEllipse(const int16_t *buf, ssize_t samples, size_t, size_t height)
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
void Visualizer::DrawSoundEllipseStereo(const int16_t *buf_left, const int16_t *buf_right, ssize_t samples, size_t half_height)
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
		auto c = toColor(sqrt(x*x + 4*y*y), radius, true);
		w << NC::XY(left_half_width + x, top_half_height + y)
		  << c
		  << Config.visualizer_chars[1]
		  << NC::FormattedColor::End<>(c);
	}
}

/**********************************************************************/

#ifdef HAVE_FFTW3_H
void Visualizer::DrawFrequencySpectrum(const int16_t *buf, ssize_t samples, size_t y_offset, size_t height)
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
		) / (DFT_NONZERO_SIZE);

	m_bar_heights.clear();

	const size_t win_width = w.getWidth();

	size_t cur_bin = 0;
	while (cur_bin < m_fftw_results && Bin2Hz(cur_bin) < m_dft_logspace[0])
		++cur_bin;
	for (size_t x = 0; x < win_width; ++x)
	{
		double bar_height = 0;

		// accumulate bins
		size_t count = 0;
		// check right bound
		while (cur_bin < m_fftw_results && Bin2Hz(cur_bin) < m_dft_logspace[x])
		{
			// check left bound if not first index
			if (x == 0 || Bin2Hz(cur_bin) >= m_dft_logspace[x-1])
			{
				bar_height += m_freq_magnitudes[cur_bin];
				++count;
			}
			++cur_bin;
		}

		if (count == 0)
			continue;

		// average bins
		bar_height /= count;

		// log scale bar heights
		bar_height = (20 * log10(bar_height) + DYNAMIC_RANGE + GAIN) / DYNAMIC_RANGE;
		// Scale bar height between 0 and height
		bar_height = bar_height > 0 ? bar_height * height : 0;
		bar_height = bar_height > height ? height : bar_height;

		m_bar_heights.emplace_back(x, bar_height);
	}

	size_t h_idx = 0;
	for (size_t x = 0; x < win_width; ++x)
	{
		const size_t i = m_bar_heights[h_idx].first;
		const double bar_height = m_bar_heights[h_idx].second;
		double h = 0;

		if (x == i) {
			// this data point exists
			h = bar_height;
			if (h_idx < m_bar_heights.size()-1)
				++h_idx;
		} else {
			// data point does not exist, need to interpolate
			h = Interpolate(x, h_idx);
		}

		for (size_t j = 0; j < h; ++j)
		{
			size_t y = flipped ? y_offset+j : y_offset+height-j-1;
			auto color = toColor(j, height, false);
			std::wstring ch;
	
			// select character to draw
			if (Config.visualizer_spectrum_smooth_look) {
				// smooth
				const size_t size = SMOOTH_CHARS.size();
				const size_t idx = static_cast<size_t>(size*h) % size;
				if (j < h-1 || idx == size-1) {
					// full height
					ch = SMOOTH_CHARS[size-1];
				} else {
					// fractional height
					if (flipped) {
						ch = SMOOTH_CHARS[size-idx-2];
						color = NC::FormattedColor(color.color(), {NC::Format::Reverse});
					} else {
						ch = SMOOTH_CHARS[idx];
					}
				}
			} else  {
				// default, non-smooth
				ch = Config.visualizer_chars[1];
			}

			// draw character on screen
			w << NC::XY(x, y)
			  << color
			  << ch
			  << NC::FormattedColor::End<>(color);
		}
	}
}

void Visualizer::DrawFrequencySpectrumStereo(const int16_t *buf_left, const int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawFrequencySpectrum(buf_left, samples, 0, height);
	DrawFrequencySpectrum(buf_right, samples, height, w.getHeight() - height);
}

double Visualizer::Interpolate(size_t x, size_t h_idx)
{
	const double x_next = m_bar_heights[h_idx].first;
	const double h_next = m_bar_heights[h_idx].second;

	double dh = 0;
	if (h_idx == 0) {
		// no data points on left, linear extrap
		if (h_idx < m_bar_heights.size()-1) {
			const double x_next2 = m_bar_heights[h_idx+1].first;
			const double h_next2 = m_bar_heights[h_idx+1].second;
			dh = (h_next2 - h_next) / (x_next2 - x_next);
		}
		return h_next - dh*(x_next-x);
	} else if (h_idx == 1) {
		// one data point on left, linear interp
		const double x_prev = m_bar_heights[h_idx-1].first;
		const double h_prev = m_bar_heights[h_idx-1].second;
		dh = (h_next - h_prev) / (x_next - x_prev);
		return h_next - dh*(x_next-x);
	} else if (h_idx < m_bar_heights.size()-1) {
		// two data points on both sides, cubic interp
		// see https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Interpolation_on_an_arbitrary_interval
		const double x_prev2 = m_bar_heights[h_idx-2].first;
		const double h_prev2 = m_bar_heights[h_idx-2].second;
		const double x_prev = m_bar_heights[h_idx-1].first;
		const double h_prev = m_bar_heights[h_idx-1].second;
		const double x_next2 = m_bar_heights[h_idx+1].first;
		const double h_next2 = m_bar_heights[h_idx+1].second;

		const double m0 = (h_prev - h_prev2) / (x_prev - x_prev2);
		const double m1 = (h_next2 - h_next) / (x_next2 - x_next);
		const double t = (x - x_prev) / (x_next - x_prev);
		const double h00 = 2*t*t*t - 3*t*t + 1;
		const double h10 = t*t*t - 2*t*t + t;
		const double h01 = -2*t*t*t + 3*t*t;
		const double h11 = t*t*t - t*t;

		return h00*h_prev + h10*(x_next-x_prev)*m0 + h01*h_next + h11*(x_next-x_prev)*m1;
	}

	// less than two data points on right, no interp, should never happen unless VERY low DFT size
	return h_next;
}

void Visualizer::ApplyWindow(double *output, const int16_t *input, ssize_t samples)
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
		double window = a0 - a1*cos(2*pi*i/(DFT_NONZERO_SIZE-1)) + a2*cos(4*pi*i/(DFT_NONZERO_SIZE-1));
		output[i] = window * input[i] / INT16_MAX;
	}
}

double Visualizer::Bin2Hz(size_t bin)
{
	return bin*44100/DFT_TOTAL_SIZE;
}

// Generate log-scaled vector of frequencies from HZ_MIN to HZ_MAX
void Visualizer::GenLogspace()
{
	// Calculate number of extra bins needed between 0 HZ and HZ_MIN
	const size_t win_width = w.getWidth();
	const size_t left_bins = (log10(HZ_MIN) - win_width*log10(HZ_MIN)) / (log10(HZ_MIN) - log10(HZ_MAX));
	// Generate logspaced frequencies
	m_dft_logspace.resize(win_width);
	const double log_scale = log10(HZ_MAX) / (left_bins + m_dft_logspace.size() - 1);
	for (size_t i = left_bins; i < m_dft_logspace.size() + left_bins; ++i) {
		m_dft_logspace[i - left_bins] = pow(10, i * log_scale);
	}
}
#endif // HAVE_FFTW3_H

void Visualizer::InitDataSource()
{
	if (!Config.visualizer_fifo_path.empty())
		m_source_location = Config.visualizer_fifo_path; // deprecated
	else
		m_source_location = Config.visualizer_data_source;

	// If there's a colon and a location doesn't start with '/' we have a UDP
	// sink. Otherwise assume it's a FIFO.
	auto colon = m_source_location.rfind(':');
	if (m_source_location[0] != '/' && colon != std::string::npos)
	{
		m_source_port = m_source_location.substr(colon+1);
		m_source_location.resize(colon);
	}
	else
		m_source_port.clear();
}

void Visualizer::InitVisualization()
{
	size_t rendered_samples = 0;
	switch (Config.visualizer_type)
	{
	case VisualizerType::Wave:
		// Guarantee integral amount of samples per column.
		rendered_samples = ceil(44100.0 / Config.visualizer_fps / w.getWidth());
		rendered_samples *= w.getWidth();
		// Slow the scolling 10 times to make it watchable.
		rendered_samples *= 10;
		draw = &Visualizer::DrawSoundWave;
		drawStereo = &Visualizer::DrawSoundWaveStereo;
		break;
	case VisualizerType::WaveFilled:
		// Guarantee integral amount of samples per column.
		rendered_samples = ceil(44100.0 / Config.visualizer_fps / w.getWidth());
		rendered_samples *= w.getWidth();
		// Slow the scolling 10 times to make it watchable.
		rendered_samples *= 10;
		draw = &Visualizer::DrawSoundWaveFill;
		drawStereo = &Visualizer::DrawSoundWaveFillStereo;
		break;
#	ifdef HAVE_FFTW3_H
	case VisualizerType::Spectrum:
		rendered_samples = DFT_NONZERO_SIZE;
		draw = &Visualizer::DrawFrequencySpectrum;
		drawStereo = &Visualizer::DrawFrequencySpectrumStereo;
		break;
#	endif // HAVE_FFTW3_H
	case VisualizerType::Ellipse:
		// Keep constant amount of samples on the screen regardless of fps.
		rendered_samples = 44100 / 30;
		draw = &Visualizer::DrawSoundEllipse;
		drawStereo = &Visualizer::DrawSoundEllipseStereo;
		break;
	}
	if (Config.visualizer_in_stereo)
		rendered_samples *= 2;
	m_rendered_samples.resize(rendered_samples);

	// Keep 500ms worth of samples in the incoming buffer.
	size_t buffered_samples = 44100.0 / 2;
	if (Config.visualizer_in_stereo)
		buffered_samples *= 2;
	m_incoming_samples.resize(buffered_samples);
	m_buffered_samples.resize(buffered_samples);
}

/**********************************************************************/

void Visualizer::Clear()
{
	w.clear();
	std::fill(m_rendered_samples.begin(), m_rendered_samples.end(), 0);

	// Discard any lingering data from the data source.
	if (m_source_fd >= 0)
	{
		ssize_t bytes_read;
		do
			bytes_read = read(m_source_fd, m_incoming_samples.data(),
			                  sizeof(int16_t) * m_incoming_samples.size());
		while (bytes_read > 0);
	}

}

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
	InitVisualization();
	Statusbar::printf("Visualization type: %1%", Config.visualizer_type);
}

void Visualizer::OpenDataSource()
{
	if (m_source_fd >= 0)
		return;

	if (!m_source_port.empty())
	{
		addrinfo hints, *res;
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = PF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		int errcode = getaddrinfo(m_source_location.c_str(), m_source_port.c_str(),
		                          &hints, &res);
		if (errcode != 0)
		{
			Statusbar::printf("Couldn't resolve \"%1%:%2%\": %3%",
			                  m_source_location, m_source_port, gai_strerror(errcode));
			return;
		}

		for (auto addr = res; addr != nullptr; addr = addr->ai_next)
		{
			m_source_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
			if (m_source_fd >= 0)
			{
				// No SOCK_NONBLOCK on MacOS
				int socket_flags = fcntl(m_source_fd, F_GETFL, 0);
				fcntl(m_source_fd, F_SETFL, socket_flags | O_NONBLOCK);

				errcode = bind(m_source_fd, res->ai_addr, res->ai_addrlen);
				if (errcode < 0)
				{
					std::cerr << "Binding a socket failed: " << strerror(errno) << std::endl;
					CloseDataSource();
				}
				else
					break;
			}
			else
				std::cerr << "Creation of socket failed: " << strerror(errno) << std::endl;
		}

		freeaddrinfo(res);
	}
	else
	{
		m_source_fd = open(m_source_location.c_str(), O_RDONLY | O_NONBLOCK);
		if (m_source_fd < 0)
			Statusbar::printf("Couldn't open \"%1%\" for reading PCM data: %2%",
			                  m_source_location, strerror(errno));
	}
}

void Visualizer::CloseDataSource()
{
	if (m_source_fd >= 0)
		close(m_source_fd);
	m_source_fd = -1;
}

void Visualizer::FindOutputID()
{
	m_output_id = -1;
	// Look for the output only if its name is specified and we're fetching
	// samples from a FIFO.
	if (!Config.visualizer_output_name.empty() && m_source_port.empty())
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
	m_auto_scale_multiplier = 1;
}

#endif // ENABLE_VISUALIZER
