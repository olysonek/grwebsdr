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

#include <config.h>
#include "auth.h"
#include "receiver.h"
#include "globals.h"
#include "utils.h"
#include "websocket.h"
#include "http.h"
#include "config_load.h"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <cstdlib>
#include <osmosdr/device.h>
#include <vector>
#include <stdexcept>
#include <termios.h>

using namespace gr;
using namespace std;

vector<osmosdr::source::sptr> osmosdr_sources;
vector<source_info_t> sources_info;
unordered_map<string, receiver::sptr> receiver_map;

top_block_sptr topbl;

struct lws_context *ws_context;
struct lws_pollfd *pollfds;
int *fd_lookup;
int count_pollfds;
int max_fds;
struct lws **fd2wsi;

void usage(const char *progname)
{
	printf("Usage: %s [options]\n\n", progname);
	printf("Options: -h                     Print help\n");
	printf("         -c certificate_file\n");
	printf("         -k private_key_file\n");
	printf("         -s                     Scan for sources\n");
	printf("         -f config_file\n");
	printf("         -p port number\n");
	printf("         -r resource path (default is ../web)\n");
	printf("         -d path to user database\n");
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
	struct termios term;

	cout << "Enter new admin password: ";
	cout.flush();
	if (tcgetattr(STDIN_FILENO, &term) < 0) {
		perror("tcgetattr");
		exit(1);
	}
	term.c_lflag &= ~ECHO;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
		perror("tcsetattr");
		exit(1);
	}
	getline(cin, ret);
	term.c_lflag |= ECHO;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
		perror("tcsetattr");
		exit(1);
	}

	return ret;
}

bool should_use_source(string name)
{
	string answer;

	cout << "Use the following source? " << name << endl;
	while (answer != "y" && answer != "n") {
		cout << "[y/n] ";
		cout.flush();
		getline(cin, answer);
	}
	return answer == "y";
}

int read_int(string prompt)
{
	int ret = 0;
	string line;

	while (1) {
		try {
			cout << prompt;
			cout.flush();
			getline(cin, line);
			ret = stoi(line);
			break;
		} catch (invalid_argument& e) {
			cout << "Bad format. Enter an integer." << endl;
		}
	}
	return ret;
}

int ask_freq_converter_offset()
{
	return read_int("Enter up/down converter offset for the device: ");
}

int ask_hw_freq()
{
	return read_int("Enter initial HW frequency: ");
}

int ask_sample_rate()
{
	return read_int("Enter sample rate: ");
}

const struct lws_protocols protocols[] = {
	{ "http-only", &http_cb, sizeof(struct http_user_data),
		HTTP_MAX_PAYLOAD, 0, nullptr},
	{ "websocket", &websocket_cb, sizeof(struct websocket_user_data),
		WEBSOCKET_MAX_PAYLOAD, 0, nullptr},
	{ nullptr, nullptr, 0, 0, 0, nullptr}
};

int run(const char *key_path, const char *cert_path, int port,
		const char *resource_path)
{
	struct lws_context_creation_info info;
	int n;
	bool quitting = false;
	struct lws_http_mount mount, stream_mount;

	memset(&mount, 0, sizeof(mount));
	memset(&stream_mount, 0, sizeof(mount));
	mount.mount_next = &stream_mount;
	mount.mountpoint = "/";
	mount.mountpoint_len = strlen("/");
	mount.origin = resource_path;
	mount.def = "index.html";
	mount.origin_protocol = LWSMPRO_FILE;
	stream_mount.mountpoint = "/streams";
	stream_mount.mountpoint_len = strlen("/streams");
	stream_mount.origin = "http-only";
	stream_mount.origin_protocol = LWSMPRO_CALLBACK;

	max_fds = getdtablesize();
	pollfds = (struct lws_pollfd *) malloc(sizeof(*pollfds) * max_fds);
	fd_lookup = (int *) malloc(sizeof(*fd_lookup) * max_fds);
	fd2wsi = (struct lws **) calloc(max_fds, sizeof(*fd2wsi));
	if (!pollfds || !fd_lookup || !fd2wsi) {
		fprintf(stderr, "malloc failed\n");
		return -1;
	}
	add_pollfd(STDIN_FILENO, POLLIN);

	memset(&info, 0, sizeof(info));
	info.port = port;
	info.iface = nullptr;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.max_http_header_pool = 100;
	info.ssl_cert_filepath = cert_path;
	info.ssl_private_key_filepath = key_path;
	info.options |= LWS_SERVER_OPTION_REDIRECT_HTTP_TO_HTTPS;
	info.mounts = &mount;

	ws_context = lws_create_context(&info);
	if (!ws_context) {
		fprintf(stderr, "Failed to create Websocket context.\n");
		return -1;
	}
	puts("Starting the server. Press enter to quit.");

	while (!quitting) {
		n = poll(pollfds, count_pollfds, 50);
		if (n <= 0)
			continue;
		for (n = 0; n < count_pollfds; ++n) {
			if (!pollfds[n].revents)
				continue;
			if (pollfds[n].fd == STDIN_FILENO) {
				quitting = true;
				break;
			}
			lws_service_fd(ws_context, &pollfds[n]);
			// If lws didn't service the fd, it might be
			// a receiver fd
			if (pollfds[n].revents && fd2wsi[pollfds[n].fd]) {
				lws_callback_on_writable(fd2wsi[pollfds[n].fd]);
			}
		}
	}

	puts("Stopping the server.");
	lws_context_destroy(ws_context);
	return 0;
}

void scan_sources()
{
	osmosdr::devices_t devices;

	cout << "Looking for sources..." << endl;
	devices = osmosdr::device::find();
	for (osmosdr::device_t device : devices) {
		cout << device.to_string() << endl;
	}
}

void add_sources_interactive()
{
	osmosdr::devices_t devices;

	cout << "Looking for tuners..." << endl;
	devices = osmosdr::device::find(osmosdr::device_t("nofake"));
	for (osmosdr::device_t device : devices) {
		string str = device.to_string();
		if (should_use_source(str)) {
			int offset, freq, sample_rate;
			osmosdr::source::sptr source;
			source_info_t info;

			offset = ask_freq_converter_offset();
			freq = ask_hw_freq();
			sample_rate = ask_sample_rate();

			source = osmosdr::source::make(str);
			source->set_freq_corr(0.0);
			source->set_gain_mode(true);
			source->set_sample_rate(sample_rate);
			source->set_center_freq(freq);
			osmosdr_sources.push_back(source);
			info.freq_converter_offset = offset;
			info.label = str;
			sources_info.push_back(info);
		}
	}
}

int main(int argc, char **argv)
{
	const char *cert_path = nullptr, *key_path = nullptr;
	const char *config_path = nullptr;
	const char *resource_path = "../web";
	const char *user_db = nullptr;
	int port = 8080;
	int c;

	while ((c = getopt(argc, argv, "hc:k:sf:p:r:d:")) != -1) {
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
		case 's':
			scan_sources();
			return 0;
		case 'f':
			config_path = optarg;
			break;
		case 'p':
			try {
				port = stoi(optarg);
			} catch (invalid_argument& e) {
				usage(argv[0]);
				return 1;
			}
			break;
		case 'r':
			resource_path = optarg;
			break;
		case 'd':
			user_db = optarg;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (config_path == nullptr) {
		add_sources_interactive();
	} else {
		if (!process_config(config_path))
			return 1;
	}

	if (osmosdr_sources.size() == 0) {
		cout << "No tuner selected. Quitting." << endl;
		return 0;
	}

	if (user_db == nullptr) {
		set_admin_username(get_username());
		set_admin_password(get_password());
	} else {
		if (!set_user_db(user_db))
			return 1;
	}

	topbl = make_top_block("top_block");

	for (osmosdr::source::sptr src : osmosdr_sources) {
		src->set_dc_offset_mode(0);
		src->set_iq_balance_mode(0);
		src->set_bandwidth(0.0);
	}

	if (run(key_path, cert_path, port, resource_path) != 0)
		return 1;

	getchar();
	topbl->stop();
	topbl->wait();

	auth_finalize();

	return 0;
}
