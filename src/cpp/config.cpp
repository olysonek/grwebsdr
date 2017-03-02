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

#include "config.h"
#include "globals.h"
#include <json-c/json_object.h>
#include <json-c/json_util.h>
#include <json-c/linkhash.h>
#include <string>
#include <string.h>
#include <stdio.h>

using namespace std;

bool add_source(struct json_object *obj)
{
	string osmosdr_arg{""};
	string label{""};
	string description{""};
	int freq_converter_offset = 0;
	int initial_hw_freq = 103000000;
	int sample_rate = 2400000;
	double freq_corr = 0.0;
	bool auto_gain = true;
	double gain = 1.0;
	bool got_gain = false;
	osmosdr::source::sptr source;
	source_info_t info;

	json_object_object_foreach(obj, key, tmp) {
		if (!strcmp(key, "osmosdr_arg")) {
			if (json_object_get_type(tmp) != json_type_string)
				goto bad_format;
			osmosdr_arg = json_object_get_string(tmp);
		} else if (!strcmp(key, "label")) {
			if (json_object_get_type(tmp) != json_type_string)
				goto bad_format;
			label = json_object_get_string(tmp);
		} else if (!strcmp(key, "description")) {
			if (json_object_get_type(tmp) != json_type_string)
				goto bad_format;
			description = json_object_get_string(tmp);
		} else if (!strcmp(key, "freq_correction")) {
			if (json_object_get_type(tmp) != json_type_double)
				goto bad_format;
			freq_corr = json_object_get_double(tmp);
		} else if (!strcmp(key, "freq_converter_offset")) {
			if (json_object_get_type(tmp) != json_type_int)
				goto bad_format;
			freq_converter_offset = json_object_get_int(tmp);
		} else if (!strcmp(key, "initial_hw_freq")) {
			if (json_object_get_type(tmp) != json_type_int)
				goto bad_format;
			initial_hw_freq = json_object_get_int(tmp);
		} else if (!strcmp(key, "sample_rate")) {
			if (json_object_get_type(tmp) != json_type_int)
				goto bad_format;
			sample_rate = json_object_get_int(tmp);
		} else if (!strcmp(key, "auto_gain")) {
			if (json_object_get_type(tmp) != json_type_boolean)
				goto bad_format;
			auto_gain = json_object_get_boolean(tmp);
			if (got_gain && auto_gain) {
				puts("Error: Setting automatic gain and"
						"gain value at the same time.");
				return false;
			}
			got_gain = true;
		} else if (!strcmp(key, "gain")) {
			if (json_object_get_type(tmp) != json_type_double)
				goto bad_format;
			gain = json_object_get_double(tmp);
			if (got_gain && auto_gain) {
				puts("Error: Setting automatic gain and"
						"gain value at the same time.");
				return false;
			}
			auto_gain = false;
			got_gain = true;
		} else {
			printf("Unknown source parameter in config file: %s\n",
					key);
			return false;
		}
	}
	if (label == "")
		label = osmosdr_arg;
	source = osmosdr::source::make(osmosdr_arg);
	source->set_freq_corr(freq_corr);
	source->set_sample_rate(sample_rate);
	source->set_center_freq(initial_hw_freq);
	if (auto_gain) {
		source->set_gain_mode(true);
	} else {
		source->set_gain_mode(false);
		source->set_gain(gain);
	}
	osmosdr_sources.push_back(source);
	info.label = label;
	info.description = description;
	info.freq_converter_offset = freq_converter_offset;
	sources_info.push_back(info);
	return true;
bad_format:
	puts("Bad format of config file.");
	return false;
}

bool process_config(const char *path)
{
	struct json_object *obj, *sources, *source;
	int i, len;
	bool ret = true;

	printf("Processing config file %s.\n", path);
	obj = json_object_from_file(path);
	if (obj == nullptr)
		return false;
	if (!json_object_object_get_ex(obj, "sources", &sources)
			|| json_object_get_type(sources) != json_type_array) {
		ret = false;
		goto out;
	}
	len = json_object_array_length(sources);
	for (i = 0; i < len; ++i) {
		source = json_object_array_get_idx(sources, i);
		if (!add_source(source)) {
			ret = false;
			goto out;
		}
	}
out:
	json_object_put(obj);
	if (ret)
		puts("Done processing config file.");
	return ret;
}
