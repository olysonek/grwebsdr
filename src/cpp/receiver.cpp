#include "receiver.h"
#include "fm_demod.h"
#include "am_demod.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>
#include <algorithm>

using namespace std;
using namespace gr;
using namespace gr::analog;
using namespace gr::filter;

vector<string> receiver::supported_demods = { "WFM", "FM", "AM", "LSB", "USB", "CW" };

receiver::sptr receiver::make(double src_rate, gr::top_block_sptr top_bl,
			int fds[2])
{
	return boost::shared_ptr<receiver>(new receiver(src_rate, top_bl, fds));
}

receiver::receiver(double src_rate, gr::top_block_sptr top_bl, int fds[2])
	: hier_block2("receiver", io_signature::make(1, 1, sizeof (gr_complex)),
			io_signature::make(0, 0, 0)),
	src_rate(src_rate), top_bl(top_bl), privileged(false),
	audio_rate(24000), running(false)
{
	this->fds[0] = fds[0];
	this->fds[1] = fds[1];

	sink = ogg_sink::make(fds[1], 1, audio_rate);
	change_demod("WFM");
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

bool receiver::change_demod(string d)
{
	int dec1 = 8; // Pre-demodulation decimation
	double dec1_rate = src_rate / dec1; // Sample rate after first decimation
	int offset = xlate == nullptr ? 0 : xlate->center_freq();
	vector<gr_complex> taps;

	if (find(supported_demods.begin(), supported_demods.end(), d)
			== supported_demods.end()) {
		return false;
	}
	disconnect_all();
	if (d == "WFM") {
		taps = taps_f2c(firdes::low_pass(1.0, src_rate, 75000, 25000));
		demod = fm_demod::make(dec1_rate, audio_rate, 75000);
	} else if (d == "FM") {
		taps = taps_f2c(firdes::low_pass(1.0, src_rate, 4000, 2000));
		demod = fm_demod::make(dec1_rate, audio_rate, 4000);
	} else if (d == "AM") {
		taps = taps_f2c(firdes::low_pass(1.0, src_rate, 5000, 1000));
		demod = am_demod::make(dec1_rate, audio_rate);
	} else if (d == "USB") {
		taps = firdes::complex_band_pass(1.0, src_rate, 1, 5000, 1000);
		demod = am_demod::make(dec1_rate, audio_rate);
	} else if (d == "LSB") {
		taps = firdes::complex_band_pass(1.0, src_rate, -5000, -1, 1000);
		demod = am_demod::make(dec1_rate, audio_rate);
	} else if (d == "CW") {
		taps = firdes::complex_band_pass(1.0, src_rate, 1, 500, 500,
				firdes::WIN_KAISER, 1.0);
		demod = am_demod::make(dec1_rate, audio_rate);
	}
	cur_demod = d;
	xlate = freq_xlating_fir_filter_ccc::make(dec1, taps, offset, src_rate);
	connect_blocks();
	return true;
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

string receiver::get_source_name()
{
	return source_name;
}

osmosdr::source::sptr receiver::get_source()
{
	return source;
}

void receiver::set_source(string source_name)
{
	osmosdr::source::sptr tmp;

	tmp = osmosdr_sources.at(source_name);
	if (running) {
		top_bl->disconnect(source, 0, self(), 0);
		top_bl->connect(tmp, 0, self(), 0);
	}
	source = tmp;
	this->source_name = source_name;
}

bool receiver::start()
{
	if (!is_ready() || is_running())
		return false;
	top_bl->connect(source, 0, self(), 0);
	running = true;
	return true;
}

bool receiver::is_ready()
{
	return source != nullptr;
}

bool receiver::is_running()
{
	return running;
}

void receiver::stop()
{
	if (is_running()) {
		top_bl->disconnect(source, 0, self(), 0);
		running = false;
	}
}
