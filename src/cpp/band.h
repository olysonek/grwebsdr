#include <gnuradio/filter/freq_xlating_fir_filter_ccc.h>

typedef gr::filter::freq_xlating_fir_filter_ccc band;

band::sptr create_band(int decim, double center_freq, double rate);
