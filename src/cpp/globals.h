#ifndef STUFF_H
#define STUFF_H

#include "receiver.h"
#include <unordered_map>
#include <string>
#include <libwebsockets.h>
#include <vector>

#define STREAM_NAME_LEN 8

typedef struct {
	std::string label;
	int freq_converter_offset;
} source_info_t;

extern std::unordered_map<std::string, receiver::sptr> receiver_map;
extern std::vector<osmosdr::source::sptr> osmosdr_sources;
extern std::vector<source_info_t> sources_info;
extern gr::top_block_sptr topbl;
extern struct lws_context *ws_context;
extern const struct lws_protocols protocols[];
extern struct lws_pollfd *pollfds;
extern int *fd_lookup;
extern int count_pollfds;
extern int max_fds;
extern struct lws **fd2wsi;

#endif
