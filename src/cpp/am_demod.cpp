/*
 * GrWebSDR: a web SDR receiver
 *
 * Copyright (C) 2017 Ondřej Lysoněk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING).  If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "am_demod.h"
#include <boost/math/common_factor_rt.hpp>
#include <gnuradio/filter/firdes.h>

using namespace gr;
using namespace gr::analog;
using namespace gr::blocks;
using namespace gr::filter;

am_demod::sptr am_demod::make(int in_rate, int out_rate, int cutoff, int trans)
{
	return boost::shared_ptr<am_demod>(new am_demod(in_rate, out_rate, cutoff, trans));
}

am_demod::am_demod(int in_rate, int out_rate, int cutoff, int trans)
	: hier_block2("am_demod", io_signature::make(1, 1, sizeof(gr_complex)),
			io_signature::make(1, 1, sizeof(float)))
{
	int div = boost::math::gcd(in_rate, out_rate);
	int interpolation = out_rate / div;
	int decimation = in_rate / div;

	agc = agc_cc::make(0.1f);
	mag = complex_to_mag::make();
	resampler = rational_resampler_base_fff::make(interpolation, decimation,
			firdes::low_pass(1.0, in_rate, cutoff, trans));

	connect(self(), 0, agc, 0);
	connect(agc, 0, mag, 0);
	connect(mag, 0, resampler, 0);
	connect(resampler, 0, self(), 0);
}

am_demod::~am_demod()
{
	disconnect_all();
}
