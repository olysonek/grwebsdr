#include "utils.h"
#include <boost/shared_ptr.hpp>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/hier_block2.h>

class wfm_demod : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<wfm_demod> sptr;

	static sptr make(double rate, unsigned int interp, unsigned int decim);

private:
	gr::analog::quadrature_demod_cf::sptr demod;
	gr::filter::fir_filter_fff::sptr low_pass;
	gr::filter::rational_resampler_base_fff::sptr resampler;

	wfm_demod(double rate, unsigned int interp, unsigned int decim) {
		demod = gr::analog::quadrature_demod_cf::make(
				rate / (2 * M_PI * 75000));
		low_pass = gr::filter::fir_filter_fff::make(1,
				gr::filter::firdes::low_pass(1.0, rate,
				rate / 2 - rate / 1000, rate / 1000));
		resampler = gr::filter::rational_resampler_base_fff::make(
				interp, decim,
				filter_f(interp, decim, 0.4f));
		//connect(this, 0, demod, 0);
		//connect(demod, 0, low_pass, 0);
	}
};
