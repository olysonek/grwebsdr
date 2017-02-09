#ifndef HTTP_H
#define HTTP_H

#include <libwebsockets.h>
#include <string>

#define HTTP_MAX_PAYLOAD (1 << 14)
#define MAX_URL_LEN 16

struct http_user_data {
	int fd;
	char url[MAX_URL_LEN + 1];
	char buf[LWS_PRE + HTTP_MAX_PAYLOAD];
};

int http_cb(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len);
int add_pollfd(int fd, short events);

#endif
