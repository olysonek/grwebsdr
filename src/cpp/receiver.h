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
