#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <string>
#include <libwebsockets.h>

#define WEBSOCKET_MAX_PAYLOAD 4096

struct websocket_user_data {
	std::string stream_name;
	bool initialized;
	bool privileged_changed;
	bool source_changed;
	bool demod_changed;
	char buf[LWS_PRE + WEBSOCKET_MAX_PAYLOAD];
};

int websocket_cb(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len);
int init_websocket();

#endif
