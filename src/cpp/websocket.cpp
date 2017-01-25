#include "stuff.h"
#include "receiver.h"
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

struct user_data {
	string stream_name;
	bool initialized;
	bool privileged_changed;
	char buf[LWS_PRE + MAX_PAYLOAD];
};

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len);

atomic_int ws_id(0);
struct json_tokener *tok;

static const struct lws_protocols protocols[] = {
	{ "", &ws_callback, sizeof(struct user_data), MAX_PAYLOAD, 0, nullptr},
	{ nullptr, nullptr, 0, 0, 0, nullptr}
};
struct lws_context *context;

void set_privileged(string stream, bool val)
{
	topbl_mutex.lock();
	if (receiver_map.find(stream) == receiver_map.end()) {
		topbl_mutex.unlock();
		return;
	}
	receiver_map[stream]->set_privileged(val);
	topbl_mutex.unlock();
}

void process_authentication(struct user_data *data, struct json_object *obj)
{
	struct json_object *tmp;

	if (json_object_object_get_ex(obj, "login", &tmp)) {
		set_privileged(data->stream_name, true);
		data->privileged_changed = true;
	} else if (json_object_object_get_ex(obj, "logout", &tmp)) {
		set_privileged(data->stream_name, false);
		data->privileged_changed = true;
	}
}

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

void change_hw_freq(struct user_data *data, struct json_object *obj)
{
	bool priv;
	struct json_object *freq_obj;
	int freq;

	if (!json_object_object_get_ex(obj, "hw_freq", &freq_obj)
			|| json_object_get_type(freq_obj) != json_type_int)
		return;
	freq = json_object_get_int(freq_obj);

	topbl_mutex.lock();
	if (receiver_map.find(data->stream_name) == receiver_map.end()) {
		topbl_mutex.unlock();
		return;
	}
	priv = receiver_map[data->stream_name]->get_privileged();
	topbl_mutex.unlock();
	if (!priv)
		return;
	osmosdr_src->set_center_freq(freq);
	lws_callback_on_writable_all_protocol(context, protocols);
}

void change_demod(struct user_data *data, struct json_object *obj)
{
	struct json_object *demod_obj;
	const char *demod;
	receiver::demod_t d;

	if (!json_object_object_get_ex(obj, "demod", &demod_obj)
			|| json_object_get_type(demod_obj) != json_type_string)
		return;
	demod = json_object_get_string(demod_obj);
	if (!strcmp(demod, "WFM"))
		d = receiver::WFM_DEMOD;
	else if (!strcmp(demod, "AM"))
		d = receiver::AM_DEMOD;
	else if (!strcmp(demod, "USB"))
		d = receiver::USB_DEMOD;
	else
		return;

	topbl_mutex.lock();
	if (receiver_map.find(data->stream_name) == receiver_map.end()) {
		topbl_mutex.unlock();
		return;
	}
	topbl->lock();
	receiver_map[data->stream_name]->change_demod(d);
	topbl->unlock();
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

	tmp = json_object_new_int(osmosdr_src->get_center_freq());
	json_object_object_add(obj, "hw_freq", tmp);

	tmp = json_object_new_int(osmosdr_src->get_sample_rate());
	json_object_object_add(obj, "bandwidth", tmp);

	tmp = json_object_new_string("WFM");
	json_object_object_add(obj, "demod", tmp);

	strcpy(buf, json_object_get_string(obj));
	json_object_put(obj);
	lws_write(wsi, (unsigned char *) buf, strlen(buf), LWS_WRITE_TEXT);
}

void send_privileged(struct lws *wsi, struct user_data *data)
{
	bool val;
	char *buf = data->buf + LWS_PRE;
	struct json_object *obj, *val_obj;

	topbl_mutex.lock();
	if (receiver_map.find(data->stream_name) == receiver_map.end()) {
		topbl_mutex.unlock();
		return;
	}
	val = receiver_map[data->stream_name]->get_privileged();
	topbl_mutex.unlock();

	obj = json_object_new_object();
	val_obj = json_object_new_boolean(val);
	json_object_object_add(obj, "privileged", val_obj);

	strcpy(buf, json_object_get_string(obj));
	json_object_put(obj);
	lws_write(wsi, (unsigned char *) buf, strlen(buf), LWS_WRITE_TEXT);
}

void send_hw_freq(struct lws *wsi, struct user_data *data)
{
	struct json_object *obj, *val_obj;
	char *buf = data->buf + LWS_PRE;

	obj = json_object_new_object();
	val_obj = json_object_new_int(osmosdr_src->get_center_freq());
	json_object_object_add(obj, "hw_freq", val_obj);

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
		if (data->privileged_changed) {
			send_privileged(wsi, data);
			data->privileged_changed = false;
		}
		send_hw_freq(wsi, data);
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
		change_hw_freq(data, obj);
		change_demod(data, obj);
		process_authentication(data, obj);
		json_object_put(obj);
		lws_callback_on_writable(wsi);
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

void *ws_loop(void *arg)
{
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
