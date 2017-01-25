#include "receiver.h"
#include "fm_demod.h"
#include "am_demod.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>

using namespace std;
using namespace gr;
using namespace gr::analog;
using namespace gr::filter;

receiver::sptr receiver::make(osmosdr::source::sptr src, gr::top_block_sptr top_bl,
			int fds[2])
{
	return boost::shared_ptr<receiver>(new receiver(src, top_bl, fds));
}

receiver::receiver(osmosdr::source::sptr src, gr::top_block_sptr top_bl,
		int fds[2])
	: hier_block2("receiver", io_signature::make(1, 1, sizeof (gr_complex)),
			io_signature::make(0, 0, 0))
	, src(src), top_bl(top_bl), privileged(false), audio_rate(24000),
	demod_type(NO_DEMOD)
{
	this->fds[0] = fds[0];
	this->fds[1] = fds[1];

	sink = ogg_sink::make(fds[1], 1, audio_rate);
	change_demod(WFM_DEMOD);
}

receiver::~receiver()
{
	disconnect_all();
	close(fds[0]);
	close(fds[1]);
}

void receiver::connect_blocks()
{
	connect(self(), 0, xlate, 0);
	connect(xlate, 0, demod, 0);
	connect(demod, 0, sink, 0);
}

void receiver::change_demod(receiver::demod_t d)
{
	double src_rate = src->get_sample_rate();
	int dec1 = 8; // Pre-demodulation decimation
	double dec1_rate = src_rate / dec1; // Sample rate after first decimation
	int offset = xlate == nullptr ? 0 : xlate->center_freq();
	vector<gr_complex> taps;

	disconnect_all();
	switch (d) {
	case receiver::WFM_DEMOD:
		taps = taps_f2c(firdes::low_pass(1.0, src_rate, 75000, 25000));
		demod = fm_demod::make(dec1_rate, audio_rate);
		demod_type = WFM_DEMOD;
		break;
	case receiver::AM_DEMOD:
		taps = taps_f2c(firdes::low_pass(1.0, src_rate, 5000, 1000));
		demod = am_demod::make(dec1_rate, audio_rate);
		demod_type = AM_DEMOD;
		break;
	case receiver::USB_DEMOD:
		taps = firdes::complex_band_pass(1.0, src_rate, 1, 5000, 1000);
		demod = am_demod::make(dec1_rate, audio_rate);
		demod_type = USB_DEMOD;
		break;
	case receiver::LSB_DEMOD:
		taps = firdes::complex_band_pass(1.0, src_rate, -5000, -1, 1000);
		demod = am_demod::make(dec1_rate, audio_rate);
		demod_type = LSB_DEMOD;
		break;
	case receiver::CW_DEMOD:
		taps = firdes::complex_band_pass(1.0, src_rate, 1, 500, 500,
				firdes::WIN_KAISER, 1.0);
		demod = am_demod::make(dec1_rate, audio_rate);
		demod_type = CW_DEMOD;
		break;
	default:
		return;
	}
	xlate = freq_xlating_fir_filter_ccc::make(dec1, taps, offset, src_rate);
	connect_blocks();
}

void receiver::set_center_freq(double freq)
{
	xlate->set_center_freq(freq);
}

int *receiver::get_fd()
{
	return fds;
}

bool receiver::get_privileged()
{
	return privileged;
}

void receiver::set_privileged(bool val)
{
	privileged = val;
}
