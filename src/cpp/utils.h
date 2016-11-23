#include <gnuradio/gr_complex.h>
#include <vector>

std::vector<gr_complex> taps_f2c(std::vector<float> vec);

std::vector<float> filter_f(int interpolation, int decimation, double fbw);
