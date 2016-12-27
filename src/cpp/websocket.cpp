#include "stuff.h"
#include <atomic>
#include <libwebsockets.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iomanip>

#define MAX_PAYLOAD 4096

using namespace std;

atomic_int ws_id(0);

struct user_data {
	string stream_name;
	bool stream_name_sent;
	char buf[LWS_PRE + MAX_PAYLOAD];
};

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	struct user_data *data = (struct user_data *) user;
	(void) wsi;

	switch (reason) {
	case LWS_CALLBACK_SERVER_WRITEABLE: {
		if (!data->stream_name_sent) {
			char *buf = data->buf + LWS_PRE;
			strcpy(buf, "{\"stream_name\":\"");
			strcat(buf, data->stream_name.c_str());
			strcat(buf, "\"}");
			lws_write(wsi, (unsigned char *) data->buf + LWS_PRE,
					strlen(data->buf + LWS_PRE),
					LWS_WRITE_TEXT);
			data->stream_name_sent = true;
		}
		break;
	}
	case LWS_CALLBACK_RECEIVE: {
		string stream_name;
		char *start;
		unsigned int len;
		start = (char *) in + strcspn((char *) in, "0123456789");
		len = strspn(start, "0123456789");
		string msg(start, len);
		istringstream s(msg);
		int offset;
		s >> offset;
		topbl_mutex.lock();
		if (receiver_map.find(data->stream_name) == receiver_map.end()) {
			topbl_mutex.unlock();
			break;
		}
		receiver_map[data->stream_name]->set_center_freq(offset);
		topbl_mutex.unlock();
		break;
	}
	case LWS_CALLBACK_ESTABLISHED: {
		int id = ws_id.fetch_add(1);
		stringstream s;
		s << setbase(36) << setfill('0') << setw(4) << id;
		data->stream_name = s.str() + string(".ogg");
		data->stream_name_sent = false;
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
