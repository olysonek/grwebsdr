#include "fm_demod.h"
#include "utils.h"
#include <boost/math/common_factor_rt.hpp>

using namespace gr;
using namespace gr::analog;
using namespace gr::filter;

fm_demod::sptr fm_demod::make(int in_rate, int out_rate, int cutoff, int out_cutoff, int out_trans)
{
	return boost::shared_ptr<fm_demod>(new fm_demod(in_rate, out_rate,
				cutoff, out_cutoff, out_trans));
}

fm_demod::fm_demod(int in_rate, int out_rate, int cutoff, int out_cutoff, int out_trans)
	: hier_block2("fm_demod", io_signature::make(1, 1, sizeof(gr_complex)),
			io_signature::make(1, 1, sizeof(float)))
{
	int div = boost::math::gcd(in_rate, out_rate);
	int interpolation = out_rate / div;
	int decimation = in_rate / div;

	demod = quadrature_demod_cf::make(in_rate / (2 * M_PI * cutoff));
	low_pass = fir_filter_fff::make(1, firdes::low_pass(1.0, in_rate,
				out_cutoff, out_trans));
	resampler = rational_resampler_base_fff::make(interpolation, decimation,
			filter_f(interpolation, decimation, 0.4f));

	connect(self(), 0, demod, 0);
	connect(demod, 0, low_pass, 0);
	connect(low_pass, 0, resampler, 0);
	connect(resampler, 0, self(), 0);
}

fm_demod::~fm_demod()
{
	disconnect_all();
}
