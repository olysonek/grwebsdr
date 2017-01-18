#include "am_demod.h"
#include <boost/math/common_factor_rt.hpp>
#include <gnuradio/filter/firdes.h>

using namespace gr;
using namespace gr::analog;
using namespace gr::blocks;
using namespace gr::filter;

am_demod::sptr am_demod::make(int in_rate, int out_rate)
{
	return boost::shared_ptr<am_demod>(new am_demod(in_rate, out_rate));
}

am_demod::am_demod(int in_rate, int out_rate)
	: hier_block2("am_demod", io_signature::make(1, 1, sizeof(gr_complex)),
			io_signature::make(1, 1, sizeof(float)))
{
	int div = boost::math::gcd(in_rate, out_rate);
	int interpolation = out_rate / div;
	int decimation = in_rate / div;

	agc = agc_cc::make(0.1f);
	mag = complex_to_mag::make();
	resampler = rational_resampler_base_fff::make(interpolation, decimation,
			firdes::low_pass(1.0, in_rate, 4000, 500));

	connect(self(), 0, agc, 0);
	connect(agc, 0, mag, 0);
	connect(mag, 0, resampler, 0);
	connect(resampler, 0, self(), 0);
}

am_demod::~am_demod()
{
	disconnect_all();
}
