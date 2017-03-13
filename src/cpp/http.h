/*
 * GrWebSDR: a web SDR receiver
 *
 * Copyright (C) 2017 Ondřej Lysoněk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING).  If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef HTTP_H
#define HTTP_H

#include <config.h>
#include <libwebsockets.h>
#include <string>

#define HTTP_MAX_PAYLOAD (1 << 14)
#define MAX_URL_LEN 32

struct http_user_data {
	int fd;
	char url[MAX_URL_LEN + 1];
	char buf[LWS_PRE + HTTP_MAX_PAYLOAD];
};

int http_cb(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len);
int add_pollfd(int fd, short events);

#endif
