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

#ifndef RECEIVER_H
#define RECEIVER_H

#include "ogg_sink.h"
#include <boost/shared_ptr.hpp>
#include <gnuradio/top_block.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccc.h>
#include <gnuradio/hier_block2.h>
#include <osmosdr/source.h>
#include <cstdio>
#include <string>
#include <vector>

class receiver : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<receiver> sptr;
	static std::vector<std::string> supported_demods;

	static sptr make(gr::top_block_sptr top_bl,
			int fds[2]);
	bool set_freq_offset(int offset);
	int get_freq_offset();
	int *get_fd();
	~receiver();
	bool get_privileged();
	void set_privileged(bool val);
	bool change_demod(std::string d);
	std::string get_current_demod();
	size_t get_source_ix();
	osmosdr::source::sptr get_source();
	void set_source(size_t ix);
	bool is_ready();
	bool is_running();
	bool start();
	void stop();

private:
	receiver(gr::top_block_sptr top_bl, int fds[2]);
	size_t source_ix;
	osmosdr::source::sptr source;
	gr::top_block_sptr top_bl;
	gr::filter::freq_xlating_fir_filter_ccc::sptr xlate;
	gr::basic_block_sptr demod;
	ogg_sink::sptr sink;
	int fds[2];
	bool privileged;
	int audio_rate;
	bool running;
	std::string cur_demod;

	void connect_blocks();
	int trim_freq_offset(int offset, int src_rate);
};

#endif
