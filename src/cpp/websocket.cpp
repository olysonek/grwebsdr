#include "stuff.h"
#include <atomic>
#include <libwebsockets.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <json-c/json_tokener.h>
#include <cstdio>

#define MAX_PAYLOAD 4096

using namespace std;

atomic_int ws_id(0);
struct json_tokener *tok;

struct user_data {
	string stream_name;
	bool initialized;
	char buf[LWS_PRE + MAX_PAYLOAD];
};

void change_freq_offset(struct user_data *data, struct json_object *obj)
{
	int offset;
	struct json_object *offset_obj;

	if (!json_object_object_get_ex(obj, "freq_offset", &offset_obj)
			|| json_object_get_type(offset_obj) != json_type_int)
		return;
	offset = json_object_get_int(offset_obj);
	topbl_mutex.lock();
	if (receiver_map.find(data->stream_name) == receiver_map.end()) {
		topbl_mutex.unlock();
		return;
	}
	receiver_map[data->stream_name]->set_center_freq(offset);
	topbl_mutex.unlock();
}

void init_ws_con(struct lws *wsi, struct user_data *data)
{
	struct json_object *obj;
	struct json_object *tmp;
	char *buf = data->buf + LWS_PRE;

	obj = json_object_new_object();
	tmp = json_object_new_string(data->stream_name.c_str());
	json_object_object_add(obj, "stream_name", tmp);

	strcpy(buf, "{\"stream_name\":\"");
	strcat(buf, data->stream_name.c_str());
	strcat(buf, "\"}");
	strcpy(buf, json_object_get_string(obj));
	json_object_put(obj);
	lws_write(wsi, (unsigned char *) buf, strlen(buf), LWS_WRITE_TEXT);
}

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	struct user_data *data = (struct user_data *) user;
	(void) wsi;

	switch (reason) {
	case LWS_CALLBACK_SERVER_WRITEABLE: {
		if (!data->initialized) {
			init_ws_con(wsi, data);
			data->initialized = true;
		}
		break;
	}
	case LWS_CALLBACK_RECEIVE: {
		struct json_object *obj;

		obj = json_tokener_parse_ex(tok, (char *) in, len);
		if (!obj) {
			puts("json parsing failed");
			break;
		}
		json_tokener_reset(tok);
		change_freq_offset(data, obj);
		json_object_put(obj);
		break;
	}
	case LWS_CALLBACK_ESTABLISHED: {
		int id = ws_id.fetch_add(1);
		stringstream s;
		s << setbase(36) << setfill('0') << setw(4) << id;
		data->stream_name = s.str() + string(".ogg");
		data->initialized = false;
		lws_callback_on_writable(wsi);
		break;
	}
	default:
		break;
	}
	return 0;
}

static const struct lws_protocols protocols[] = {
	{ "", &ws_callback, sizeof(struct user_data), MAX_PAYLOAD, 0, nullptr},
	{ nullptr, nullptr, 0, 0, 0, nullptr}
};

void *ws_loop(void *arg)
{
	struct lws_context *context;
	struct lws_context_creation_info info;

	(void) arg;

	tok = json_tokener_new();
	if (!tok) {
		fprintf(stderr, "json_tokener_new failed\n");
		return nullptr;
	}

	memset(&info, 0, sizeof(info));
	info.port = 8081;
	info.iface = nullptr;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	context = lws_create_context(&info);
	if (!context) {
		fprintf(stderr, "Failed to create Websocket context.\n");
		return nullptr;
	}

	while (1) {
		lws_service(context, 100);
	}

	lws_context_destroy(context);
	return nullptr;
}
