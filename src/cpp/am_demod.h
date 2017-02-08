#ifndef AM_DEMOD_H
#define AM_DEMOD_H

#include <boost/shared_ptr.hpp>
#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/analog/agc_cc.h>
#include <gnuradio/blocks/complex_to_mag.h>

class am_demod : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<am_demod> sptr;
	static sptr make(int in_rate, int out_rate, int cutoff, int trans);
	~am_demod();
private:
	am_demod(int in_rate, int out_rate, int cutoff, int trans);

	gr::analog::agc_cc::sptr agc;
	gr::blocks::complex_to_mag::sptr mag;
	gr::filter::rational_resampler_base_fff::sptr resampler;
};

#endif
