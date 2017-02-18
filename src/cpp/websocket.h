#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "globals.h"
#include <string>
#include <libwebsockets.h>

#define WEBSOCKET_MAX_PAYLOAD 4096

struct websocket_user_data {
	char stream_name[STREAM_NAME_LEN + 1];
	bool initialized;
	bool privileged_changed;
	bool source_changed;
	bool demod_changed;
	bool offset_changed;
	char buf[LWS_PRE + WEBSOCKET_MAX_PAYLOAD];
};

int websocket_cb(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len);
int init_websocket();

#endif
