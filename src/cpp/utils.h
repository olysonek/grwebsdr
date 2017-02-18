#ifndef UTILS_H
#define UTILS_H

#include "globals.h"
#include <vector>
#include <gnuradio/gr_complex.h>

std::vector<gr_complex> taps_f2c(std::vector<float> vec);
int set_nonblock(int fd);
int count_receivers_running();

#endif
