#include "receiver.h"
#include "stuff.h"
#include "utils.h"
#include "websocket.h"
#include "http.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <microhttpd.h>
#include <atomic>
#include <mutex>
#include <cstring>
#include <unordered_map>
#include <pthread.h>
#include <cstdlib>

#define PORT 8080

using namespace gr;
using namespace std;

atomic_int con_num;
mutex topbl_mutex;

osmosdr::source::sptr osmosdr_src;
unordered_map<string, receiver::sptr> receiver_map;
string username;
string password;

top_block_sptr topbl;

void usage(const char *progname)
{
	printf("Usage: %s [options]\n\n", progname);
	printf("Options: -h                     Print help\n");
	printf("         -c certificate_file\n");
	printf("         -k private_key_file\n");
}

string get_username()
{
	string ret;

	cout << "Enter new admin user name: ";
	cout.flush();
	getline(cin, ret);
	return ret;
}

string get_password()
{
	string ret;
	cout << "Enter new admin password: ";
	cout.flush();
	system("/usr/bin/stty -echo");
	getline(cin, ret);
	system("/usr/bin/stty echo");
	return ret;
}

const struct lws_protocols protocols[] = {
	{ "http-only", &http_cb, sizeof(struct http_user_data),
		HTTP_MAX_PAYLOAD, 0, nullptr},
	{ "websocket", &websocket_cb, sizeof(struct websocket_user_data),
		WEBSOCKET_MAX_PAYLOAD, 0, nullptr},
	{ nullptr, nullptr, 0, 0, 0, nullptr}
};

struct lws_context *ws_context;

int run(const char *key_path, const char *cert_path)
{
	struct lws_context_creation_info info;

	memset(&info, 0, sizeof(info));
	info.port = PORT;
	info.iface = nullptr;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.ssl_cert_filepath = cert_path;
	info.ssl_private_key_filepath = key_path;
	info.options |= LWS_SERVER_OPTION_REDIRECT_HTTP_TO_HTTPS;

	ws_context = lws_create_context(&info);
	if (!ws_context) {
		fprintf(stderr, "Failed to create Websocket context.\n");
		return -1;
	}

	while (1) {
		lws_service(ws_context, 100);
	}

	lws_context_destroy(ws_context);
	return 0;
}

int main(int argc, char **argv)
{
	double src_rate = 2400000.0;
	const char *cert_path = nullptr, *key_path = nullptr;
	int c;

	while ((c = getopt(argc, argv, "hc:k:")) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			cert_path = optarg;
			break;
		case 'k':
			key_path = optarg;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	username = get_username();
	password = get_password();

	topbl = make_top_block("top_block");
	osmosdr_src = osmosdr::source::make();

	osmosdr_src->set_sample_rate(src_rate);
	osmosdr_src->set_center_freq(102300000.0);
	osmosdr_src->set_freq_corr(0.0);
	osmosdr_src->set_dc_offset_mode(0);
	osmosdr_src->set_iq_balance_mode(0);
	osmosdr_src->set_gain_mode(true);
	osmosdr_src->set_bandwidth(0.0);

	run(key_path, cert_path);

	getchar();
	topbl->stop();
	topbl->wait();

	return 0;
}
