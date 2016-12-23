#include "receiver.h"
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

#define PORT 8080

using namespace gr;
using namespace std;

atomic_int con_num;
mutex topbl_mutex;
int nlisteners;

receiver::sptr rec;
osmosdr::source::sptr src;

int fds[2];

ssize_t callback(void *cls, uint64_t pos, char *buf, size_t max)
{
	ssize_t ret;

	(void) cls;
	(void) pos;
	ret = read(fds[0], buf, max);
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

top_block_sptr bl;

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

	(void) cls;
	(void) url;
	(void) version;
	(void) upload_data;
	(void) upload_data_size;

	if (*con_cls != &answer) {
		*con_cls = (void *) &answer;
		return MHD_YES;
	}
	*con_cls = NULL;
	if (strcmp(method, MHD_HTTP_METHOD_GET) != 0)
		return MHD_NO;
	if (strcmp(url, "/stream.ogg") == 0) {
		topbl_mutex.lock();
		printf("access, nlist: %d\n", nlisteners);
		++nlisteners;
		bl->lock();
		if (rec != nullptr)
			rec->disconnect();
		rec = nullptr;
		if (pipe(fds)) {
			perror("pipe");
			bl->unlock();
			topbl_mutex.unlock();
			return MHD_NO;
		}
		rec = receiver::make(src, bl, fds[1]);
		bl->unlock();
		if (nlisteners == 1)
			bl->start();
		topbl_mutex.unlock();
		response = MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 1024,
				&callback, NULL, NULL);
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
	puts("request_completed called");
	(void) cls;
	(void) connection;
	(void) con_cls;

	if (toe != MHD_REQUEST_TERMINATED_CLIENT_ABORT)
		return;
	topbl_mutex.lock();
	printf("compl, nlist: %d\n", nlisteners);
	--nlisteners;
	if (nlisteners == 0) {
		bl->stop();
		bl->wait();
		bl->lock();
		rec->disconnect();
		rec = nullptr;
		bl->unlock();
		close(fds[0]);
		close(fds[1]);
	}
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

int main()
{
	double src_rate = 2000000.0;
	struct MHD_Daemon *daemon;

	daemon = MHD_start_daemon(MHD_USE_DEBUG | MHD_USE_POLL_INTERNALLY
			| MHD_USE_THREAD_PER_CONNECTION,
			PORT, NULL, NULL, &answer, NULL,
			MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
			MHD_OPTION_NOTIFY_CONNECTION, &connection_cb, NULL,
			MHD_OPTION_END);
	if (daemon == NULL) {
		fprintf(stderr, "Failed to create MHD daemon.\n");
		return 1;
	}

	bl = make_top_block("bla");
	src = osmosdr::source::make();

	src->set_sample_rate(src_rate);
	src->set_center_freq(102300000.0);
	src->set_freq_corr(0.0);
	src->set_dc_offset_mode(0);
	src->set_iq_balance_mode(0);
	src->set_gain_mode(false);
	src->set_gain(10.0);
	src->set_if_gain(20.0);
	src->set_bb_gain(20.0);
	src->set_bandwidth(0.0);

	getchar();
	bl->stop();
	bl->wait();

	MHD_stop_daemon(daemon);

	return 0;
}
