#include "wfm_demod.h"
#include <gnuradio/io_signature.h>

wfm_demod::sptr wfm_demod::make(double rate, unsigned int interp,
		unsigned int decim)
{
	return gnuradio::get_initial_sptr(new wfm_demod(rate, interp, decim));
}
