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
