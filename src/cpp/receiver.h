#include <boost/shared_ptr.hpp>
#include <gnuradio/top_block.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccc.h>
#include <gnuradio/blocks/wavfile_sink.h>
#include <osmosdr/source.h>

class receiver {
public:
	typedef boost::shared_ptr<receiver> sptr;
	static sptr make(osmosdr::source::sptr src, gr::top_block_sptr top_bl,
			const char *fifo_name);
	void set_center_freq(double freq);

private:
	receiver(osmosdr::source::sptr src, gr::top_block_sptr top_bl,
			const char *fifo_name);
	osmosdr::source::sptr src;
	gr::top_block_sptr top_bl;
	gr::filter::freq_xlating_fir_filter_ccc::sptr xlate;
	gr::analog::quadrature_demod_cf::sptr demod;
	gr::filter::fir_filter_fff::sptr low_pass;
	gr::filter::rational_resampler_base_fff::sptr resampler;
	gr::blocks::wavfile_sink::sptr sink;
};
