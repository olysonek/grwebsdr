#include <boost/shared_ptr.hpp>
#include <gnuradio/audio/sink.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base_ccc.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/top_block.h>
#include <iostream>
#include <cmath>
#include <osmosdr/source.h>
#include <cstdio>

using namespace boost;
using namespace gr;
using namespace gr::analog;
using namespace gr::filter;

int main()
{
	osmosdr::source::sptr src = osmosdr::source::make();

	rational_resampler_base_ccc::sptr resampler1 =
			rational_resampler_base_ccc::make(1, 4,
			std::vector<gr_complex>());

	fir_filter_ccf::sptr low_pass1 = fir_filter_ccf::make(1,
			firdes::low_pass(1.0, 2000000.0, 100000.0, 1000000.0));

	quadrature_demod_cf::sptr demod = quadrature_demod_cf::make(
			500000/(2*M_PI*75000));

	fir_filter_fff::sptr low_pass2 = fir_filter_fff::make(10.0,
			firdes::low_pass(1.0, 500000.0, 50000.0/2-50000.0/32, 50000.0/32));

	rational_resampler_base_fff::sptr resampler2 =
			rational_resampler_base_fff::make(48, 50,
			std::vector<float>());

	audio::sink::sptr sink = audio::sink::make(48000);

	src->set_sample_rate(2000000.0);
	src->set_center_freq(103400000.0);
	src->set_freq_corr(0.0);
	src->set_gain_mode(false);
	src->set_gain(10.0);
	src->set_if_gain(20.0);
	src->set_bb_gain(20.0);
	src->set_bandwidth(0.0);

	top_block_sptr bl = make_top_block("bla");
	bl->connect(src, 0, resampler1, 0);
	bl->connect(resampler1, 0, low_pass1, 0);
	bl->connect(low_pass1, 0, demod, 0);
	bl->connect(demod, 0, low_pass2, 0);
	bl->connect(low_pass2, 0, resampler2, 0);
	bl->connect(resampler2, 0, sink, 0);

	bl->start();
	getchar();
	bl->stop();
	bl->wait();

	return 0;
}
