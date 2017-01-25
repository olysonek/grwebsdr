#ifndef WFM_DEMOD_H
#define WFM_DEMOD_H

#include <boost/shared_ptr.hpp>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/firdes.h>

class fm_demod : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<fm_demod> sptr;
	static sptr make(int in_rate, int out_rate);
	~fm_demod();
private:
	fm_demod(int in_rate, int out_rate);
	gr::analog::quadrature_demod_cf::sptr demod;
	gr::filter::fir_filter_fff::sptr low_pass;
	gr::filter::rational_resampler_base_fff::sptr resampler;
};

#endif
