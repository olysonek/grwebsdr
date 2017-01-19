#ifndef STUFF_H
#define STUFF_H

#include "receiver.h"
#include <mutex>
#include <unordered_map>
#include <string>

#define STREAM_NAME_LEN 8

extern std::mutex topbl_mutex;
extern std::unordered_map<std::string, receiver::sptr> receiver_map;
extern osmosdr::source::sptr osmosdr_src;
extern gr::top_block_sptr topbl;

void *ws_loop(void *);

#endif
