#ifndef SSB_DEMOD_H
#define SSB_DEMOD_H

#include <boost/shared_ptr.hpp>
#include <gnuradio/hier_block2.h>

class ssb_demod : public gr::hier_block2 {
public:
	typedef boost::shared_ptr<ssb_demod> sptr;
	static sptr make(int in_rate, int out_rate, int cutoff, int trans);
	~ssb_demod();
private:
	ssb_demod(int in_rate, int out_rate, int cutoff, int trans);
};

#endif
