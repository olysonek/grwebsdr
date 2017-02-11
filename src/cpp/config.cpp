#include "config.h"
#include "stuff.h"
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
	int freq_converter_offset = 0;
	int initial_hw_freq = 103000000;
	int sample_rate = 2400000;
	osmosdr::source::sptr source;
	source_info_t info;

	json_object_object_foreach(obj, key, tmp) {
		if (!strcmp(key, "osmosdr_arg")) {
			if (json_object_get_type(tmp) != json_type_string)
				goto bad_format;
			osmosdr_arg = json_object_get_string(tmp);
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
		} else {
			printf("Unknown source parameter in config file: %s\n",
					key);
			return false;
		}
	}
	source = osmosdr::source::make(osmosdr_arg);
	source->set_sample_rate(sample_rate);
	source->set_center_freq(initial_hw_freq);
	osmosdr_sources.emplace(osmosdr_arg, source);
	info.freq_converter_offset = freq_converter_offset;
	sources_info.emplace(osmosdr_arg, info);
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
