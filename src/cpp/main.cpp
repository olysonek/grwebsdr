#include "receiver.h"
#include "stuff.h"
#include "utils.h"
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

#define PORT 8080

using namespace gr;
using namespace std;

atomic_int con_num;
mutex topbl_mutex;

osmosdr::source::sptr osmosdr_src;
unordered_map<string, receiver::sptr> receiver_map;

int fds[2];

ssize_t callback(void *cls, uint64_t pos, char *buf, size_t max)
{
	ssize_t ret;
	int fd = *(int *) cls;

	(void) pos;

	ret = read(fd, buf, max);
	if (ret < 0) {
		if (errno == EAGAIN) {
			return 0;
		} else {
			perror("read");
			return MHD_CONTENT_READER_END_WITH_ERROR;
		}
	} else {
		return ret;
	}
}

const char *stream_name(const char *url)
{
	const char *ret;
	size_t len;
	const char *ogg = ".ogg";
	const size_t ogg_len = strlen(ogg);

	ret = strrchr(url, '/');
	if (!ret)
		return nullptr;
	++ret;
	len = strlen(ret);
	if (len != STREAM_NAME_LEN)
		return nullptr;
	if (!strcmp(ogg, ret + len - ogg_len))
		return ret;
	else
		return nullptr;
}

top_block_sptr topbl;

int answer(void *cls, struct MHD_Connection *con, const char *url,
		const char *method, const char *version,
		const char *upload_data, size_t *upload_data_size,
		void **con_cls)
{
	puts("answer called");
	struct MHD_Response *response;
	int ret;
	int fd;
	struct stat st;
	const char *stream;

	(void) cls;
	(void) url;
	(void) version;
	(void) upload_data;
	(void) upload_data_size;

	if (!*con_cls) {
		*con_cls = strdup(url);
		return *con_cls ? MHD_YES : MHD_NO;
	}

	if (strcmp(method, MHD_HTTP_METHOD_GET) != 0)
		return MHD_NO;
	printf("URL requested: %s\n", url);
	stream = stream_name(url);
	if (stream) {
		int pipe_fds[2];
		receiver::sptr rec;

		topbl_mutex.lock();
		printf("access, nlist: %lu\n", receiver_map.size());
		if (receiver_map.find(stream) != receiver_map.end()) {
			topbl_mutex.unlock();
			return MHD_NO;
		}
		if (pipe(pipe_fds)) {
			perror("pipe");
			topbl_mutex.unlock();
			return MHD_NO;
		}
		rec = receiver::make(osmosdr_src, topbl, pipe_fds);
		receiver_map.emplace(stream, rec);
		topbl->lock();
		topbl->connect(osmosdr_src, 0, rec, 0);
		topbl->unlock();
		if (receiver_map.size() == 1)
			topbl->start();
		topbl_mutex.unlock();
		response = MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 1024,
				&callback, rec->get_fd(), nullptr);
		MHD_add_response_header(response, "Content-Type", "audio/ogg");
		MHD_add_response_header(response, MHD_HTTP_HEADER_EXPIRES, "0");
		MHD_add_response_header(response, MHD_HTTP_HEADER_PRAGMA,
				"no-cache");
		MHD_add_response_header(response, MHD_HTTP_HEADER_CACHE_CONTROL,
				"no-cache, no-store, must-revalidate");
	} else if (strcmp(url, "/") == 0 || strcmp(url, "/index.html") == 0) {
		fd = open("../web/index.html", O_RDONLY);
		if (fd < 0) {
			perror("open");
			return MHD_NO;
		}
		if (fstat(fd, &st) < 0) {
			perror("fstat");
			close(fd);
			return MHD_NO;
		}
		response = MHD_create_response_from_fd(st.st_size, fd);
	} else {
		return MHD_NO;
	}
	if (!response)
		return MHD_NO;
	ret = MHD_queue_response(con, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;
}

void request_completed(void *cls, struct MHD_Connection *connection,
		void **con_cls, enum MHD_RequestTerminationCode toe)
{
	const char *stream;
	printf("request_completed called, toe: %d\n", (int) toe);
	(void) cls;
	(void) connection;

	stream = stream_name((char *) *con_cls);
	if (!stream)
		return;
	topbl_mutex.lock();
	printf("compl, nlist: %lu\n", receiver_map.size());
	if (receiver_map.size() == 1
			&& receiver_map.find(stream) != receiver_map.end()) {
		topbl->stop();
		topbl->wait();
	}
	topbl->lock();
	topbl->disconnect(osmosdr_src, 0, receiver_map[stream], 0);
	receiver_map.erase(stream);
	topbl->unlock();
	topbl_mutex.unlock();
	puts("request_completed returning");
}

void connection_cb(void *cls, struct MHD_Connection *connection,
		void **socket_context, enum MHD_ConnectionNotificationCode toe)
{
	(void) cls;
	(void) connection;
	(void) socket_context;

	if (toe == MHD_CONNECTION_NOTIFY_STARTED) {
		int i = con_num.fetch_add(1);
		printf("Connection %d started.\n", i);
		*socket_context = new int(i);
	} else if (toe == MHD_CONNECTION_NOTIFY_CLOSED) {
		int i = **((int **) socket_context);
		printf("Connection %d closed.\n", i);
	}
}

void usage(const char *progname)
{
	printf("Usage: %s [options]\n\n", progname);
	printf("Options: -h                     Print help\n");
	printf("         -c certificate_file\n");
	printf("         -k private_key_file\n");
}

int main(int argc, char **argv)
{
	double src_rate = 2400000.0;
	struct MHD_Daemon *daemon;
	pthread_t ws_thread;
	websocket_data ws_data = { nullptr, nullptr };
	int c;
	int use_ssl = 0;
	char *cert = nullptr, *priv_key = nullptr;
	struct MHD_OptionItem ssl_opts[3];

	while ((c = getopt(argc, argv, "hc:k:")) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			ws_data.cert_path = optarg;
			break;
		case 'k':
			ws_data.key_path = optarg;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (ws_data.cert_path && ws_data.key_path) {
		use_ssl = MHD_USE_SSL;
		cert = load_file(ws_data.cert_path);
		if (!cert) {
			printf("Failed to load the certificate file.\n");
			return 1;
		}
		// TODO: Wipe the private key from memory
		// http://crypto.stackexchange.com/questions/9998/should-i-delete-cryptographic-data-from-memory
		priv_key = load_file(ws_data.key_path);
		if (!priv_key) {
			printf("Failed to load the private key file.\n");
			free(cert);
			return 1;
		}
	} else if (ws_data.cert_path || ws_data.key_path) {
		printf("The -c and -k options must be used together.\n");
		return 1;
	}

	if (use_ssl) {
		ssl_opts[0].option = MHD_OPTION_HTTPS_MEM_KEY;
		ssl_opts[0].ptr_value = priv_key;
		ssl_opts[0].value = 0;
		ssl_opts[1].option = MHD_OPTION_HTTPS_MEM_CERT;
		ssl_opts[1].ptr_value = cert;
		ssl_opts[1].value = 0;
		ssl_opts[2].option = MHD_OPTION_END;
		ssl_opts[2].ptr_value = 0;
		ssl_opts[2].value = 0;
	} else {
		ssl_opts[0].option = MHD_OPTION_END;
		ssl_opts[0].ptr_value = 0;
		ssl_opts[0].value = 0;
	}

	if (pthread_create(&ws_thread, nullptr, &ws_loop, &ws_data)) {
		fprintf(stderr, "Failed to create Websocket thread\n");
		return 1;
	}
	daemon = MHD_start_daemon(MHD_USE_DEBUG | MHD_USE_POLL_INTERNALLY
			| MHD_USE_THREAD_PER_CONNECTION | use_ssl,
			PORT, nullptr, nullptr, &answer, nullptr,
			MHD_OPTION_NOTIFY_COMPLETED, &request_completed, nullptr,
			MHD_OPTION_NOTIFY_CONNECTION, &connection_cb, nullptr,
			MHD_OPTION_ARRAY, ssl_opts,
			MHD_OPTION_END);
	if (daemon == nullptr) {
		fprintf(stderr, "Failed to create MHD daemon.\n");
		return 1;
	}

	topbl = make_top_block("top_block");
	osmosdr_src = osmosdr::source::make();

	osmosdr_src->set_sample_rate(src_rate);
	osmosdr_src->set_center_freq(102300000.0);
	osmosdr_src->set_freq_corr(0.0);
	osmosdr_src->set_dc_offset_mode(0);
	osmosdr_src->set_iq_balance_mode(0);
	osmosdr_src->set_gain_mode(true);
	osmosdr_src->set_bandwidth(0.0);

	getchar();
	topbl->stop();
	topbl->wait();

	MHD_stop_daemon(daemon);
	free(priv_key);
	free(cert);

	return 0;
}
