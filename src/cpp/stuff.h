#ifndef STUFF_H
#define STUFF_H

#include "receiver.h"
#include <mutex>
#include <unordered_map>
#include <string>
#include <libwebsockets.h>

#define STREAM_NAME_LEN 8

typedef struct {
	const char *key_path;
	const char *cert_path;
} websocket_data;

extern std::unordered_map<std::string, receiver::sptr> receiver_map;
extern osmosdr::source::sptr osmosdr_src;
extern gr::top_block_sptr topbl;
extern std::string username;
extern std::string password;
extern struct lws_context *ws_context;
extern const struct lws_protocols protocols[];
extern struct lws_pollfd *pollfds;
extern int *fd_lookup;
extern int count_pollfds;
extern int max_fds;
extern struct lws **fd2wsi;

void *ws_loop(void *);

#endif
