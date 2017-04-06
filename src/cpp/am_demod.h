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

#ifndef AM_DEMOD_H
#define AM_DEMOD_H

#include <config.h>
#include <boost/shared_ptr.hpp>
#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/agc_cc.h>
#include <gnuradio/blocks/complex_to_mag.h>

class am_demod : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<am_demod> sptr;
	static sptr make();
	~am_demod();
private:
	am_demod();

	gr::analog::agc_cc::sptr agc;
	gr::blocks::complex_to_mag::sptr mag;
};

#endif
