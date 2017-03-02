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

#ifndef OGG_SINK_H
#define OGG_SINK_H

#include <boost/shared_ptr.hpp>
#include <gnuradio/sync_block.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

class ogg_sink : virtual public gr::sync_block {
public:
	typedef boost::shared_ptr<ogg_sink> sptr;
	static sptr make(int outfd, int n_channels, unsigned int sample_rate);
	~ogg_sink();
	int work(int noutpuut_items, gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items);
private:
	int fd;
	vorbis_info vi;
	vorbis_dsp_state vs;
	vorbis_comment comm;
	vorbis_block vb;
	ogg_packet op, op_comm, op_code;
	ogg_stream_state os;
	ogg_page og;
	ogg_sink(int outfd, int n_channels, unsigned int sample_rate);
	void print_page(void);
};

#endif
