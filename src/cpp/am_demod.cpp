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

#include <config.h>
#include "am_demod.h"

using namespace gr;
using namespace gr::analog;
using namespace gr::blocks;

am_demod::sptr am_demod::make()
{
	return boost::shared_ptr<am_demod>(new am_demod());
}

am_demod::am_demod()
	: hier_block2("am_demod", io_signature::make(1, 1, sizeof(gr_complex)),
			io_signature::make(1, 1, sizeof(float)))
{
	agc = agc_cc::make(0.1f);
	mag = complex_to_mag::make();

	connect(self(), 0, agc, 0);
	connect(agc, 0, mag, 0);
	connect(mag, 0, self(), 0);
}

am_demod::~am_demod()
{
	disconnect_all();
}
