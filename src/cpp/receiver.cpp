#include "receiver.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>

using namespace std;
using namespace gr;
using namespace gr::analog;
using namespace gr::filter;
using namespace gr::blocks;

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

std::vector<gr_complex> filter_c(int interpolation, int decimation, double fbw)
{
	std::vector<float> f = filter_f(interpolation, decimation, fbw);
	std::vector<gr_complex> ret;

	for (auto i : f)
		ret.push_back(gr_complex(i, 0.0));
	return ret;
}

std::vector<gr_complex> taps_f2c(std::vector<float> vec)
{
	std::vector<gr_complex> ret;

	ret.reserve(vec.size());
	for (float i : vec)
		ret.push_back(gr_complex(i, 0.0));
	return ret;
}

receiver::sptr receiver::make(osmosdr::source::sptr src, gr::top_block_sptr top_bl,
			int fd)
{
	return boost::shared_ptr<receiver>(new receiver(src, top_bl, fd));
}

receiver::receiver(osmosdr::source::sptr src, gr::top_block_sptr top_bl, int fd)
	: src(src), top_bl(top_bl)
{
	double src_rate = src->get_sample_rate();
	int dec1 = 8; // Pre-demodulation decimation
	double dec1_rate = src_rate / dec1; // Sample rate after first decimation
	int dec2 = dec1_rate / 1000; // Decimate down to 1kHz

	xlate = freq_xlating_fir_filter_ccc::make(dec1,
			taps_f2c(firdes::low_pass(1.0, src_rate, 75000, 25000)),
			0.0, src_rate);

	demod = quadrature_demod_cf::make(dec1_rate / (2 * M_PI * 75000));

	low_pass = fir_filter_fff::make(1, firdes::low_pass(1.0, dec1_rate,
				dec1_rate / 2 - dec1_rate / 1000,
				dec1_rate / 1000));

	resampler = rational_resampler_base_fff::make(48, dec2,
			filter_f(48, dec2, 0.4f));

	sink = ogg_sink::make(fd, 1, 48000);

	top_bl->connect(src, 0, xlate, 0);
	top_bl->connect(xlate, 0, demod, 0);
	top_bl->connect(demod, 0, low_pass, 0);
	top_bl->connect(low_pass, 0, resampler, 0);
	top_bl->connect(resampler, 0, sink, 0);
}

void receiver::disconnect()
{
	top_bl->disconnect(xlate);
	top_bl->disconnect(demod);
	top_bl->disconnect(low_pass);
	top_bl->disconnect(resampler);
	//top_bl->disconnect(sink);
}

void receiver::set_center_freq(double freq)
{
	xlate->set_center_freq(freq);
}
