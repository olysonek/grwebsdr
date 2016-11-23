#include "utils.h"
#include <gnuradio/filter/firdes.h>

using namespace gr::filter;

std::vector<gr_complex> taps_f2c(std::vector<float> vec)
{
        std::vector<gr_complex> ret;

	ret.reserve(vec.size());
	for (float i : vec)
		ret.push_back(gr_complex(i, 0.0));

	return ret;
}

/* C++ translation of the design_filter function from rational_resampler.py
 * from GNU Radio.
 */
std::vector<float> filter_f(int interpolation, int decimation, double fbw)
{
	float beta = 7.0;
	float halfband = 0.5; 
	float rate = ((float) interpolation) / decimation;
	float trans_width;
	float mid;

	if (rate >= 1.0) {
		trans_width = halfband - fbw;
		mid = halfband - trans_width / 2.0f;
	} else {
		trans_width = rate * (halfband - fbw);
		mid = rate * halfband - trans_width / 2.0f;
	}
	return firdes::low_pass(interpolation, interpolation, mid, trans_width,
			firdes::WIN_KAISER, beta);
}
