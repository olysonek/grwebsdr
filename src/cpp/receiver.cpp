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

receiver::sptr receiver::make(gr::top_block_sptr top_bl,
			int fds[2])
{
	return boost::shared_ptr<receiver>(new receiver(top_bl, fds));
}

receiver::receiver(gr::top_block_sptr top_bl, int fds[2])
	: hier_block2("receiver", io_signature::make(1, 1, sizeof (gr_complex)),
			io_signature::make(0, 0, 0)),
	top_bl(top_bl), privileged(false),
	audio_rate(24000), running(false)
{
	this->fds[0] = fds[0];
	this->fds[1] = fds[1];

	sink = ogg_sink::make(fds[1], 1, audio_rate);
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

int receiver::trim_freq_offset(int offset, int src_rate)
{
	if (offset > src_rate / 2)
		return src_rate / 2;
	else if (offset < -src_rate / 2)
		return -src_rate / 2;
	else
		return offset;
}

bool receiver::change_demod(string d)
{
	double src_rate;
	int dec1 = 8; // Pre-demodulation decimation
	double dec1_rate;
	int offset;
	vector<gr_complex> taps;

	if (source == nullptr)
		return false;
	if (find(supported_demods.begin(), supported_demods.end(), d)
			== supported_demods.end()) {
		return false;
	}

	src_rate = source->get_sample_rate();
	offset = xlate == nullptr ? 0
			: trim_freq_offset(xlate->center_freq(), src_rate);
	dec1_rate = src_rate / dec1; // Sample rate after first decimation;
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

string receiver::get_current_demod()
{
	return cur_demod;
}

bool receiver::set_freq_offset(int offset)
{
	if (xlate == nullptr)
		return false;
	offset = trim_freq_offset(offset, source->get_sample_rate());
	xlate->set_center_freq(offset);
	return true;
}

int receiver::get_freq_offset()
{
	if (xlate == nullptr)
		return 0;
	return xlate->center_freq();
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
	osmosdr::source::sptr old_source = source;

	source = osmosdr_sources.at(source_name);
	if (running)
		top_bl->disconnect(old_source, 0, self(), 0);
	if (old_source != nullptr && cur_demod != ""
			&& source->get_sample_rate()
			!= old_source->get_sample_rate()) {
		change_demod(cur_demod);
	}
	if (running)
		top_bl->connect(source, 0, self(), 0);
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
	return source != nullptr && cur_demod != "";
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
