#include "utils.h"
#include <gnuradio/filter/firdes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

using namespace gr::filter;

/* C++ translation of the design_filter function from rational_resampler.py
 * from GNU Radio.
 */
std::vector<float> filter_f(int interpolation, int decimation, double fbw)
{
	float beta = 7.0;
	float halfband = 0.5;
	float rate = ((float) interpolation) / decimation;
	float trans_width;
	float mid;

	if (rate >= 1.0) {
		trans_width = halfband - fbw;
		mid = halfband - trans_width / 2.0f;
	} else {
		trans_width = rate * (halfband - fbw);
		mid = rate * halfband - trans_width / 2.0f;
	}
	return firdes::low_pass(interpolation, interpolation, mid, trans_width,
			firdes::WIN_KAISER, beta);
}

std::vector<gr_complex> filter_c(int interpolation, int decimation, double fbw)
{
	std::vector<float> f = filter_f(interpolation, decimation, fbw);
	std::vector<gr_complex> ret;

	for (auto i : f)
		ret.push_back(gr_complex(i, 0.0));
	return ret;
}

std::vector<gr_complex> taps_f2c(std::vector<float> vec)
{
	std::vector<gr_complex> ret;

	ret.reserve(vec.size());
	for (float i : vec)
		ret.push_back(gr_complex(i, 0.0));
	return ret;
}

char *load_file(const char *path)
{
	int fd;
	char *buf = nullptr;
	size_t len = 0, buflen = 0;
	ssize_t res;
	const size_t inc = 1024;
	char *tmp;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return nullptr;
	}
	while (1) {
		buflen += inc;
		tmp = (char *) realloc(buf, buflen);
		if (!tmp) {
			free(buf);
			close(fd);
			return nullptr;
		}
		buf = tmp;
		while (len < buflen - 1
				&& (res = read(fd, buf + len,
				buflen - len - 1)) > 0) {
			len += res;
		}
		if (res < 0) {
			perror("read");
			free(buf);
			close(fd);
			return nullptr;
		} else if (res == 0) {
			break;
		}
	}
	close(fd);
	buf[len] = 0;
	return buf;
}
