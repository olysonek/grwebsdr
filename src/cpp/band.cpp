#include "band.h"
#include "utils.h"
#include <gnuradio/filter/firdes.h>

using namespace gr::filter;

band::sptr create_band(int decim, double center_freq,
		double rate)
{
	return freq_xlating_fir_filter_ccc::make(decim,
			taps_f2c(firdes::low_pass(1.0, rate, 75000, 25000)),
			center_freq, rate);
}
