#include "ssb_demod.h"
#include <boost/math/common_factor_rt.hpp>
#include <gnuradio/filter/firdes.h>

using namespace gr;
using namespace gr::analog;
using namespace gr::blocks;
using namespace gr::filter;

ssb_demod::sptr ssb_demod::make(int in_rate, int out_rate, int cutoff, int trans)
{
	return boost::shared_ptr<ssb_demod>(new ssb_demod(in_rate, out_rate, cutoff, trans));
}

ssb_demod::ssb_demod(int in_rate, int out_rate, int cutoff, int trans)
	: hier_block2("ssb_demod", io_signature::make(1, 1, sizeof(gr_complex)),
			io_signature::make(1, 1, sizeof(float)))
{
	int div = boost::math::gcd(in_rate, out_rate);
	int interpolation = out_rate / div;
	int decimation = in_rate / div;

	agc = agc_cc::make(0.1f, 0.1f);
	carrier = sig_source_c::make(in_rate, GR_SIN_WAVE, 0, 1);
	add = add_cc::make();
	// TODO experiment with adding the complex conjugate of agc's output
	mag = complex_to_mag::make();
	resampler = rational_resampler_base_fff::make(interpolation, decimation,
			firdes::low_pass(1.0, in_rate, cutoff, trans));

	connect(self(), 0, agc, 0);
	connect(agc, 0, add, 0);
	connect(carrier, 0, add, 1);
	connect(add, 0, mag, 0);
	connect(mag, 0, resampler, 0);
	connect(resampler, 0, self(), 0);
}

ssb_demod::~ssb_demod()
{
	disconnect_all();
}
