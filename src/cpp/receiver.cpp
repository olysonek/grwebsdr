#include "receiver.h"
#include "wfm_demod.h"
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
	, src(src), top_bl(top_bl), privileged(false), audio_rate(24000)
{
	this->fds[0] = fds[0];
	this->fds[1] = fds[1];

	sink = ogg_sink::make(fds[1], 1, audio_rate);
	setup_wfm();
}

receiver::~receiver()
{
	disconnect_all();
	close(fds[0]);
	close(fds[1]);
}

void receiver::setup_wfm()
{
	if (dynamic_cast<wfm_demod *>(demod.get()) != nullptr)
		return;
	double src_rate = src->get_sample_rate();
	int dec1 = 8; // Pre-demodulation decimation
	double dec1_rate = src_rate / dec1; // Sample rate after first decimation

	disconnect_all();
	xlate = freq_xlating_fir_filter_ccc::make(dec1,
			taps_f2c(firdes::low_pass(1.0, src_rate, 75000, 25000)),
			0.0, src_rate);

	demod = wfm_demod::make(dec1_rate, audio_rate);
	connect_blocks();
}

void receiver::setup_am()
{
	if (dynamic_cast<am_demod *>(demod.get()) != nullptr)
		return;
	double src_rate = src->get_sample_rate();
	int dec1 = 8; // Pre-demodulation decimation
	double dec1_rate = src_rate / dec1; // Sample rate after first decimation

	disconnect_all();
	xlate = freq_xlating_fir_filter_ccc::make(dec1,
			taps_f2c(firdes::low_pass(1.0, src_rate, 10000, 500,
			gr::filter::firdes::WIN_KAISER, 1.0)),
			0.0, src_rate);

	demod = am_demod::make(dec1_rate, audio_rate);
	connect_blocks();
}

void receiver::connect_blocks()
{
	connect(self(), 0, xlate, 0);
	connect(xlate, 0, demod, 0);
	connect(demod, 0, sink, 0);
}

void receiver::change_demod(receiver::demod_t d)
{
	switch (d) {
	case receiver::WFM_DEMOD:
		setup_wfm();
		break;
	case receiver::AM_DEMOD:
		setup_am();
		break;
	default:
		return;
	}
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
