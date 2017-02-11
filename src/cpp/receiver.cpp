#include "receiver.h"
#include "fm_demod.h"
#include "am_demod.h"
#include "ssb_demod.h"
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

int optimal_decimation(int in_rate, int out_rate)
{
	// FIXME Optimize
	int d;

	for (d = 1; in_rate / d > out_rate; d *= 2)
		;
	d /= 2;
	for (; in_rate / d > out_rate; ++d)
		;
	--d;
	// For some reason, the AM demod doesn't work with sample rates
	// like 25kHz, trying multiples of 4kHz
	for (; in_rate % d != 0 || (in_rate / d) % 4000 != 0; --d)
		;
	if (d < 1)
		d = 1;
	return d;
}

bool receiver::change_demod(string d)
{
	int src_rate;
	int dec;
	int dec_rate;
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
	disconnect_all();
	// FIXME Please, pretty please, refactor me!
	if (d == "WFM") {
		dec = optimal_decimation(src_rate, 2 * (75000 + 25000));
		dec_rate = src_rate / dec;
		taps = taps_f2c(firdes::low_pass(1.0, src_rate, 75000, 25000));
		demod = fm_demod::make(dec_rate, audio_rate, 75000, audio_rate / 2, 4000);
	} else if (d == "FM") {
		dec = optimal_decimation(src_rate, 2 * (4000 + 2000));
		dec_rate = src_rate / dec;
		taps = taps_f2c(firdes::low_pass(1.0, src_rate, 4000, 2000));
		demod = fm_demod::make(dec_rate, audio_rate, 4000, 4000, 2000);
	} else if (d == "AM") {
		dec = optimal_decimation(src_rate, 2 * (4000 + 2000));
		dec_rate = src_rate / dec;
		taps = taps_f2c(firdes::low_pass(1.0, src_rate, 4000, 2000));
		demod = am_demod::make(dec_rate, audio_rate, 4000, 2000);
	} else if (d == "USB") {
		dec = optimal_decimation(src_rate, 2 * (4000 + 2000));
		dec_rate = src_rate / dec;
		taps = firdes::complex_band_pass(1.0, src_rate, 1020, 4000, 1000);
		demod = ssb_demod::make(dec_rate, audio_rate, 4000, 2000);
	} else if (d == "LSB") {
		dec = optimal_decimation(src_rate, 2 * (4000 + 2000));
		dec_rate = src_rate / dec;
		taps = firdes::complex_band_pass(1.0, src_rate, -4000, -1020, 1000);
		demod = ssb_demod::make(dec_rate, audio_rate, 4000, 2000);
	} else if (d == "CW") {
		dec = optimal_decimation(src_rate, 2 * (500 + 500));
		dec_rate = src_rate / dec;
		taps = firdes::complex_band_pass(1.0, src_rate, 1, 500, 500,
				firdes::WIN_KAISER, 1.0);
		demod = am_demod::make(dec_rate, audio_rate, 500, 500);
	}
	cur_demod = d;
	xlate = freq_xlating_fir_filter_ccc::make(dec, taps, offset, src_rate);
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
