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
#include "ogg_sink.h"
#include <cstdio>
#include <gnuradio/io_signature.h>
#include <stdexcept>

using namespace std;

ogg_sink::sptr ogg_sink::make(int outfd, int n_channels,
		unsigned int sample_rate)
{
	return boost::shared_ptr<ogg_sink>(new ogg_sink(outfd, n_channels,
				sample_rate));
}

ogg_sink::ogg_sink(int outfd, int n_channels, unsigned int sample_rate)
	: gr::sync_block("ogg_sink",
		gr::io_signature::make(1, 1, sizeof(float)),
		gr::io_signature::make(0, 0, 0))
	, fd(outfd), og({})
{
	vorbis_info_init(&vi);
	if (vorbis_encode_init_vbr(&vi, n_channels, sample_rate, 0.5f) != 0)
		throw runtime_error("vorbis_encode_init_vbr failed");
	vorbis_analysis_init(&vs, &vi);
	vorbis_comment_init(&comm);
	if (vorbis_analysis_headerout(&vs, &comm, &op, &op_comm, &op_code) != 0)
		throw runtime_error("vorbis_analysis_headerout failed");
	ogg_stream_init(&os, 0);

	ogg_stream_packetin(&os, &op);
	ogg_stream_packetin(&os, &op_comm);
	ogg_stream_packetin(&os, &op_code);

	ogg_stream_flush(&os, &og);
	print_page();

	vorbis_block_init(&vs, &vb);
}

int ogg_sink::work(int noutput_items, gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
	const float *in = (const float *) input_items[0];
	float **buf;
	int res;

	(void) output_items;

	buf = vorbis_analysis_buffer(&vs, noutput_items);
	memcpy(buf[0], in, noutput_items * sizeof(*in));
	if (vorbis_analysis_wrote(&vs, noutput_items))
		throw runtime_error("vorbis_analysis_wrote failed");
	while (1) {
		res = vorbis_analysis_blockout(&vs, &vb);
		if (res < 0)
			throw runtime_error("vorbis_analysis_blockout failed");
		if (res == 0)
			break;
		if (vorbis_analysis(&vb, &op))
			throw runtime_error("vorbis_analysis failed");
		ogg_stream_packetin(&os, &op);
		ogg_stream_pageout(&os, &og);
		print_page();
	}
	return noutput_items;
}

void ogg_sink::print_page(void)
{
        long len;
        for (len = 0; len < og.header_len;) {
                long tmp = write(fd, og.header + len, og.header_len - len);
                if (tmp <= 0)
			throw runtime_error(string("write failed")
					+ string(strerror(errno)));
                len += tmp;
        }
        for (len = 0; len < og.body_len;) {
                long tmp = write(fd, og.body + len, og.body_len - len);
                if (tmp <= 0)
			throw runtime_error("write failed");
                len += tmp;
        }
        memset(&og, 0, sizeof(og));
}
