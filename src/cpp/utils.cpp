#include "utils.h"
#include <gnuradio/filter/firdes.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace gr::filter;

std::vector<gr_complex> taps_f2c(std::vector<float> vec)
{
	std::vector<gr_complex> ret;

	ret.reserve(vec.size());
	for (float i : vec)
		ret.push_back(gr_complex(i, 0.0));
	return ret;
}

int set_nonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
		perror("fcntl");
		return -1;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
		perror("fcntl");
		return -1;
	}
	return 0;
}

int count_receivers_running()
{
	int ret = 0;

	for (auto pair : receiver_map) {
		if (pair.second->is_running())
			++ret;
	}
	return ret;
}
