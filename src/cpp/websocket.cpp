#include "stuff.h"
#include <libwebsockets.h>
#include <string.h>
#include <string>

#define MAX_PAYLOAD 4096

using namespace std;

struct data {
	unsigned char buf[LWS_PRE + MAX_PAYLOAD];
	unsigned int len;
};

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	(void) wsi;
	(void) user;

	switch (reason) {
	case LWS_CALLBACK_SERVER_WRITEABLE:
		break;
	case LWS_CALLBACK_RECEIVE: {
		string stream_name;
		string msg((char *) in, len);
		istringstream s(msg);
		int offset;
		s >> stream_name;
		s >> offset;
		if (stream_name.length() != STREAM_NAME_LEN)
			break;
		topbl_mutex.lock();
		if (receiver_map.find(stream_name) == receiver_map.end())
			break;
		receiver_map[stream_name]->set_center_freq(offset);
		topbl_mutex.unlock();
		break;
	}
	default:
		break;
	}
	return 0;
}

static const struct lws_protocols protocols[] = {
	{ "", &ws_callback, sizeof(struct data), MAX_PAYLOAD, 0, nullptr},
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
