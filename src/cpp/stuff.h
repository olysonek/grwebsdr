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

extern std::mutex topbl_mutex;
extern std::unordered_map<std::string, receiver::sptr> receiver_map;
extern osmosdr::source::sptr osmosdr_src;
extern gr::top_block_sptr topbl;
extern std::string username;
extern std::string password;
extern struct lws_context *ws_context;
extern const struct lws_protocols protocols[];

void *ws_loop(void *);

#endif
