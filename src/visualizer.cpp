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
#include <boost/math/constants/constants.hpp>
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

// toColor: a scaling function for coloring. For numbers 0 to max this function returns
// a coloring from the lowest color to the highest, and colors will not loop from 0 to max.
const NC::Color &toColor(size_t number, size_t max, bool wrap = true)
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

	// PCM in format 44100:16:1 (for mono visualization) and
	// 44100:16:2 (for stereo visualization) is supported.
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
		draw = &Visualizer::DrawSoundEllipse;
		drawStereo = &Visualizer::DrawSoundEllipseStereo;
	}
	else if (Config.visualizer_type == VisualizerType::Lorenz)
	{
		draw = &Visualizer::DrawSoundLorenz;
		drawStereo = &Visualizer::DrawSoundLorenzStereo;
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
			Config.visualizer_type = VisualizerType::Lorenz;
			break;
		case VisualizerType::Lorenz:
			Config.visualizer_type = VisualizerType::Wave;
			break;
	}
	Statusbar::printf("Visualization type: %1%", Config.visualizer_type);
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
		w << NC::XY(x, base_y+y)
		<< toColor(std::abs(y), half_height, false)
		<< Config.visualizer_chars[0]
		<< NC::Color::End;
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

// DrawSoundWaveFill: This visualizer is very similar to DrawSoundWave, but instead of
// a single line the entire height is filled. In stereo mode, the top half of the screen
// is dedicated to the right channel, the bottom the left channel.
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
			size_t y = flipped ? y_offset+j : y_offset+height-j-1;
			w << NC::XY(x, y)
			<< toColor(j, height)
			<< Config.visualizer_chars[1]
			<< NC::Color::End;
		}
	}
}

void Visualizer::DrawSoundWaveFillStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawSoundWaveFill(buf_left, samples, 0, height);
	DrawSoundWaveFill(buf_right, samples, height, w.getHeight() - height);
}

/**********************************************************************/

// draws the sound wave as an ellipse with origin in the center of the screen
void Visualizer::DrawSoundEllipse(int16_t *buf, ssize_t samples, size_t, size_t height)
{
	const size_t half_width = w.getWidth()/2;
	const size_t half_height = height/2;

	// make it so that the loop goes around the ellipse exactly once
	const double deg_multiplier = 2*boost::math::constants::pi<double>()/samples;

	int32_t x, y;
	double radius, max_radius;
	for (ssize_t i = 0; i < samples; ++i)
	{
		x = half_width * std::cos(i*deg_multiplier);
		y = half_height * std::sin(i*deg_multiplier);
		max_radius = sqrt(x*x + y*y);

		// calculate the distance of the sample from the center,
		// where 0 is the center of the ellipse and 1 is its border
		radius = std::abs(buf[i]);
		radius /= 32768.0;

		// appropriately scale the position
		x *= radius;
		y *= radius;

		w << NC::XY(half_width + x, half_height + y)
		<< toColor(sqrt(x*x + y*y), max_radius, false)
		<< Config.visualizer_chars[0]
		<< NC::Color::End;
	}
}

// DrawSoundEllipseStereo: This visualizer only works in stereo. The colors form concentric
// rings originating from the center (width/2, height/2). For any given point, the width is
// scaled with the left channel and height is scaled with the right channel. For example,
// if a song is entirely in the right channel, then it would just be a vertical line.
//
// Since every font/terminal is different, the visualizer is never a perfect circle. This
// visualizer assume the font height is twice the length of the font's width. If the font
// is skinner or wider than this, instead of a circle it will be an ellipse.
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

		// The arguments to the toColor function roughly follow a circle equation where
		// the center is not centered around (0,0). For example (x - w)^2 + (y-h)+2 = r^2
		// centers the circle around the point (w,h). Because fonts are not all the same
		// size, this will not always generate a perfect circle.
		w << toColor(sqrt(x*x + 4*y*y), radius)
		<< NC::XY(left_half_width + x, top_half_height + y)
		<< Config.visualizer_chars[1]
		<< NC::Color::End;
	}
}

void Visualizer::DrawSoundLorenz(int16_t *buf, ssize_t samples, size_t, size_t height)
{
	DrawSoundLorenzStereo(buf, buf, samples, height/2);
}

double rotation_count_left = 0;
double rotation_count_right = 0;

void Visualizer::DrawSoundLorenzStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t half_height)
{
	double lorenz_h = 0.01;
	double lorenz_a = 10.0;
	double lorenz_c = 8.0 / 3.0;

	rotation_count_left = rotation_count_left >= 1000.0 ? 0 : rotation_count_left;
	rotation_count_right = rotation_count_right >= 1000.0 ? 0 : rotation_count_right;

	double win_width = w.getWidth();
	double height = half_height;
	int i=0;
	double x0,y0,z0,x1,y1,z1;

	x0 = 0.1;
	y0 = 0;
	z0 = 0;

	double average_left = 0, average_right = 0;
	for (i=0;i<samples;i++) {
		average_left += std::abs(buf_left[i]);
		average_right += std::abs(buf_right[i]);
	}

	average_left = average_left / samples * 2.0;
	average_right = average_right / samples;

	double rotation_interval_left = ( average_left * ( 30.0 / 65536.0 ) );
	double rotation_interval_right = ( average_right * ( 30.0 / 65536.0 ) );
	double average = (average_left + average_right)/2.0;

	//lorenz_b will range from 11.7 to 64.4. Below 10 the lorenz is pretty much just a disc and after 64.4 the size increases dramatically.
	//The equation was generated using linear curve fitting http://www.wolframalpha.com/input/?i=quadratic+fit+%7B10%2C1%7D%2C%7B18000%2C32%7D%2C%7B45000%2C48%7D%2C%7B65536%2C64.4%7D
	double lorenz_b =  7.1429+0.000908845 * average;

	//Calculate the center of the lorenz. Described here under the heading Equilibria http://www.me.rochester.edu/courses/ME406/webexamp5/loreq.pdf
	double z_center = -1 + lorenz_b;
	double equilbria = sqrt(lorenz_c * lorenz_b - lorenz_c);

	//Calculate the scaling factor. The bounds for the complete lorenz are given here http://www.me.rochester.edu/courses/ME406/webexamp5/loreq.pdf
	//Approximately the first 100 points of the lorenz are usually outliers from the main body of the lorenz, so multiplying by 1.25
	//adjusts the bounds so that the main body takes up most of the height of the screen.
	//Only consider max y coordinate since both max z and max x will be much smaller than the max y.
	double scaling_multiplier = 1.25 * (height)/sqrt(lorenz_c * lorenz_b * lorenz_b - pow(z_center-lorenz_b,2)/pow(lorenz_b,2));

	double rotation_angle_x = ( rotation_count_left * 2.0 * boost::math::constants::pi<double>() ) / 1000.0;
	double rotation_angle_y = ( rotation_count_right * 2.0 * boost::math::constants::pi<double>() ) / 1000.0;

	double deg_multiplier_cos_x = std::cos(rotation_angle_x);
	double deg_multiplier_sin_x = std::sin(rotation_angle_x);

	double deg_multiplier_cos_y = std::cos(rotation_angle_y);
	double deg_multiplier_sin_y = std::sin(rotation_angle_y);

	double x, y, z;

	for (i=0;i< samples;i++) {
		x1 = x0 + lorenz_h * lorenz_a * (y0 - x0);
		y1 = y0 + lorenz_h * (x0 * (lorenz_b - z0) - y0);
		z1 = z0 + lorenz_h * (x0 * y0 - lorenz_c * z0);
		x0 = x1;
		y0 = y1;
		z0 = z1;

		//color points based on distance from equiliria. This must be done before rotation
		double distance_p1 = sqrt( pow(x0 - equilbria, 2) + pow(y0 - equilbria, 2) + pow(z0 - z_center, 2) );
		double distance_p2 = sqrt( pow(x0 + equilbria, 2) + pow(y0 + equilbria, 2) + pow(z0 - z_center, 2) );
		NC::Color color_distance = toColor( std::min(distance_p1, distance_p2), 16);

		//We want to rotate around the center of the lorenz. so we offset zaxis so that the center of the lorenz is at point (0,0,0)
		x = x0;
		y = y0;
		z = z0 - z_center;

		//Rotate around X and Y axis.
		double xRxy = x * deg_multiplier_cos_y + z * deg_multiplier_sin_y;
		double yRxy = x * deg_multiplier_sin_x * deg_multiplier_sin_y + y * deg_multiplier_cos_x - z * deg_multiplier_cos_y * deg_multiplier_sin_x;
		double zRxy = -1 * y * deg_multiplier_cos_x * deg_multiplier_sin_y + y * deg_multiplier_sin_x + z * deg_multiplier_cos_x * deg_multiplier_cos_y;

		x = xRxy * scaling_multiplier;
		y = yRxy * scaling_multiplier;
		z = zRxy;

		//Throw out any points outside the window
		if ( y > (height * -1 ) && y < height && x > (win_width/2 * -1 ) && x < win_width/2 )
		{
			//skip the first 100 since values under 100 stick out too much from the reset of the points
			if (i > 100)
			{
				w << color_distance;
				w << NC::XY((int32_t)(x + win_width/2.0), (int32_t) (y + height)) << Config.visualizer_chars[0];
				w << NC::Color::End;
			}
		}
	}

	rotation_count_left += rotation_interval_left;
	rotation_count_right += rotation_interval_right;
}

/**********************************************************************/

#ifdef HAVE_FFTW3_H
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
		bar_height *= log2(2 + x) * 100.0/win_width;
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

void Visualizer::DrawFrequencySpectrumStereo(int16_t *buf_left, int16_t *buf_right, ssize_t samples, size_t height)
{
	DrawFrequencySpectrum(buf_left, samples, 0, height);
	DrawFrequencySpectrum(buf_right, samples, height, w.getHeight() - height);
}
#endif // HAVE_FFTW3_H

/**********************************************************************/

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

#endif // ENABLE_VISUALIZER

/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
