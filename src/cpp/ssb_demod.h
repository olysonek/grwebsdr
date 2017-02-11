#ifndef SSB_DEMOD_H
#define SSB_DEMOD_H

#include <boost/shared_ptr.hpp>
#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/analog/agc_cc.h>
#include <gnuradio/blocks/complex_to_mag.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/add_cc.h>

class ssb_demod : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<ssb_demod> sptr;
	static sptr make(int in_rate, int out_rate, int cutoff, int trans);
	~ssb_demod();
private:
	ssb_demod(int in_rate, int out_rate, int cutoff, int trans);

	gr::analog::agc_cc::sptr agc;
	gr::analog::sig_source_c::sptr carrier;
	gr::blocks::add_cc::sptr add;
	gr::blocks::complex_to_mag::sptr mag;
	gr::filter::rational_resampler_base_fff::sptr resampler;
};

#endif
