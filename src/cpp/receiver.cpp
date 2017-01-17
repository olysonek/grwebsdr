#include "receiver.h"
#include "wfm_demod.h"
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
	, src(src), top_bl(top_bl), privileged(false)
{
	double src_rate = src->get_sample_rate();
	int dec1 = 8; // Pre-demodulation decimation
	double dec1_rate = src_rate / dec1; // Sample rate after first decimation
	int audio_rate = 48000;

	this->fds[0] = fds[0];
	this->fds[1] = fds[1];

	xlate = freq_xlating_fir_filter_ccc::make(dec1,
			taps_f2c(firdes::low_pass(1.0, src_rate, 75000, 25000)),
			0.0, src_rate);

	demod = wfm_demod::make(dec1_rate, audio_rate);

	sink = ogg_sink::make(fds[1], 1, audio_rate);

	connect(self(), 0, xlate, 0);
	connect(xlate, 0, demod, 0);
	connect(demod, 0, sink, 0);
}

receiver::~receiver()
{
	disconnect_all();
	close(fds[0]);
	close(fds[1]);
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
