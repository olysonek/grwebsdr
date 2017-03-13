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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <config.h>
#include "receiver.h"
#include <unordered_map>
#include <string>
#include <libwebsockets.h>
#include <vector>

#define STREAM_NAME_LEN 8

typedef struct {
	std::string label;
	std::string description;
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
