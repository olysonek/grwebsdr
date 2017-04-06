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

#ifndef FM_DEMOD_H
#define FM_DEMOD_H

#include <config.h>
#include <boost/shared_ptr.hpp>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/firdes.h>

class fm_demod : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<fm_demod> sptr;
	static sptr make(int in_rate, int max_deviation);
	~fm_demod();
private:
	fm_demod(int in_rate, int max_deviation);
	gr::analog::quadrature_demod_cf::sptr demod;
};

#endif
