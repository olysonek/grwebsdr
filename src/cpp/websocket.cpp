#include "websocket.h"
#include "stuff.h"
#include "utils.h"
#include "receiver.h"
#include <atomic>
#include <string.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <json-c/json_tokener.h>
#include <cstdio>

using namespace std;

struct json_tokener *tok;

string new_stream_name()
{
	static atomic_int ws_id(0);
	int id = ws_id.fetch_add(1);
	stringstream s;
	s << setbase(36) << setfill('0') << setw(4) << id;
	return s.str();
}

void set_privileged(string stream, bool val)
{
	if (receiver_map.find(stream) == receiver_map.end()) {
		return;
	}
	receiver_map[stream]->set_privileged(val);
}

void process_authentication(struct websocket_user_data *data, struct json_object *obj)
{
	struct json_object *tmp, *tmp2;
	string user, pass;

	if (json_object_object_get_ex(obj, "login", &tmp)) {
		data->privileged_changed = true;
		if (!json_object_object_get_ex(tmp, "user", &tmp2))
			return;
		if (json_object_get_type(tmp2) != json_type_string)
			return;
		user = json_object_get_string(tmp2);

		if (!json_object_object_get_ex(tmp, "pass", &tmp2))
			return;
		if (json_object_get_type(tmp2) != json_type_string)
			return;
		pass = json_object_get_string(tmp2);
		if (authenticate(user, pass))
			set_privileged(data->stream_name, true);
	} else if (json_object_object_get_ex(obj, "logout", &tmp)) {
		set_privileged(data->stream_name, false);
		data->privileged_changed = true;
	}
}

void change_freq_offset(struct websocket_user_data *data, struct json_object *obj)
{
	int offset;
	struct json_object *offset_obj;

	if (!json_object_object_get_ex(obj, "freq_offset", &offset_obj)
			|| json_object_get_type(offset_obj) != json_type_int)
		return;
	offset = json_object_get_int(offset_obj);
	if (receiver_map.find(data->stream_name) == receiver_map.end()) {
		return;
	}
	receiver_map[data->stream_name]->set_center_freq(offset);
}

void change_hw_freq(struct websocket_user_data *data, struct json_object *obj)
{
	bool priv;
	struct json_object *freq_obj;
	int freq;

	if (!json_object_object_get_ex(obj, "hw_freq", &freq_obj)
			|| json_object_get_type(freq_obj) != json_type_int)
		return;
	freq = json_object_get_int(freq_obj);

	if (receiver_map.find(data->stream_name) == receiver_map.end()) {
		return;
	}
	priv = receiver_map[data->stream_name]->get_privileged();
	if (!priv)
		return;
	osmosdr_src->set_center_freq(freq);
	lws_callback_on_writable_all_protocol(ws_context, protocols);
}

void change_demod(struct websocket_user_data *data, struct json_object *obj)
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
	else if (!strcmp(demod, "FM"))
		d = receiver::FM_DEMOD;
	else if (!strcmp(demod, "AM"))
		d = receiver::AM_DEMOD;
	else if (!strcmp(demod, "USB"))
		d = receiver::USB_DEMOD;
	else if (!strcmp(demod, "LSB"))
		d = receiver::LSB_DEMOD;
	else if (!strcmp(demod, "CW"))
		d = receiver::CW_DEMOD;
	else
		return;

	if (receiver_map.find(data->stream_name) == receiver_map.end()) {
		return;
	}
	topbl->lock();
	receiver_map[data->stream_name]->change_demod(d);
	topbl->unlock();
}

void init_ws_con(struct lws *wsi, struct websocket_user_data *data)
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

void send_privileged(struct lws *wsi, struct websocket_user_data *data)
{
	bool val;
	char *buf = data->buf + LWS_PRE;
	struct json_object *obj, *val_obj;

	if (receiver_map.find(data->stream_name) == receiver_map.end()) {
		return;
	}
	val = receiver_map[data->stream_name]->get_privileged();

	obj = json_object_new_object();
	val_obj = json_object_new_boolean(val);
	json_object_object_add(obj, "privileged", val_obj);

	strcpy(buf, json_object_get_string(obj));
	json_object_put(obj);
	lws_write(wsi, (unsigned char *) buf, strlen(buf), LWS_WRITE_TEXT);
}

void send_hw_freq(struct lws *wsi, struct websocket_user_data *data)
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

int init_websocket()
{
	tok = json_tokener_new();
	if (!tok) {
		fprintf(stderr, "json_tokener_new failed\n");
		return -1;
	}
	return 0;
}

int websocket_cb(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	struct websocket_user_data *data = (struct websocket_user_data *) user;
	(void) wsi;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		if (init_websocket())
			return -1;
		break;

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
		data->stream_name = new_stream_name() + string(".ogg");
		data->initialized = false;
		lws_callback_on_writable(wsi);
		break;
	}
	default:
		break;
	}
	return 0;
}
