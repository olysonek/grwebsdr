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

class classa {
public:
	~classa() {
		puts("destructor called\n");
	}
};

class receiver : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<receiver> sptr;
	static sptr make(osmosdr::source::sptr src, gr::top_block_sptr top_bl,
			int fds[2]);
	void set_center_freq(double freq);
	int *get_fd();
	~receiver();
	bool get_privileged();
	void set_privileged(bool val);

private:
	classa objecta;
	receiver(osmosdr::source::sptr src, gr::top_block_sptr top_bl, int fds[2]);
	osmosdr::source::sptr src;
	gr::top_block_sptr top_bl;
	gr::filter::freq_xlating_fir_filter_ccc::sptr xlate;
	gr::basic_block_sptr demod;
	ogg_sink::sptr sink;
	int fds[2];
	bool privileged;
	int audio_rate;
	enum { NO_DEMOD, WFM_DEMOD, AM_DEMOD } demod_type;

	void setup_wfm();
	void setup_am();
	void connect_blocks();
};

#endif
