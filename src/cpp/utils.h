#ifndef UTILS_H
#define UTILS_H

#include "stuff.h"
#include <vector>
#include <string>
#include <gnuradio/gr_complex.h>

std::vector<float> filter_f(int interpolation, int decimation, double fbw);
std::vector<gr_complex> filter_c(int interpolation, int decimation, double fbw);
std::vector<gr_complex> taps_f2c(std::vector<float> vec);
int set_nonblock(int fd);
int count_receivers_running();

#endif
