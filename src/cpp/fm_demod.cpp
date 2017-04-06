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
#include "fm_demod.h"
#include "utils.h"
#include <vector>

using namespace gr;
using namespace gr::analog;
using namespace gr::filter;
using namespace std;

fm_demod::sptr fm_demod::make(int in_rate, int max_deviation)
{
	return boost::shared_ptr<fm_demod>(new fm_demod(in_rate, max_deviation));
}

fm_demod::fm_demod(int in_rate, int max_deviation)
	: hier_block2("fm_demod", io_signature::make(1, 1, sizeof(gr_complex)),
			io_signature::make(1, 1, sizeof(float)))
{
	// The FM block historically contained more sub-blocks, but they were
	// moved to the receiver. I'm keeping the FM block in case of future
	// extension.
	demod = quadrature_demod_cf::make(in_rate / (2 * M_PI * max_deviation));

	connect(self(), 0, demod, 0);
	connect(demod, 0, self(), 0);
}

fm_demod::~fm_demod()
{
	disconnect_all();
}
