#include "http.h"
#include "stuff.h"
#include "utils.h"

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

int handle_new_stream(struct lws *wsi, const char *stream,
		struct http_user_data *data)
{
	unsigned char *buffer = (unsigned char *) data->buf;
	unsigned char *buf_pos = (unsigned char *) data->buf + LWS_PRE;
	unsigned char *buf_end = (unsigned char *) data->buf + sizeof(data->buf);
	const char *header;
	int pipe_fds[2];
	int n;
	receiver::sptr rec;

	topbl_mutex.lock();
	if (receiver_map.find(stream) != receiver_map.end()) {
		topbl_mutex.unlock();
		//XXX
		return 0;
	}
	if (pipe(pipe_fds)) {
		perror("pipe");
		topbl_mutex.unlock();
		return -1;
	}
	if (set_nonblock(pipe_fds[0])) {
		topbl_mutex.unlock();
		return -1;
	}
	data->fd = pipe_fds[0];
	rec = receiver::make(osmosdr_src, topbl, pipe_fds);
	receiver_map.emplace(stream, rec);
	topbl->lock();
	topbl->connect(osmosdr_src, 0, rec, 0);
	topbl->unlock();
	if (receiver_map.size() == 1)
		topbl->start();
	topbl_mutex.unlock();
	if (lws_add_http_header_status(wsi, 200, &buf_pos, buf_end))
		return 1;
	header = "audio/ogg";
	if (lws_add_http_header_by_token(wsi,
				WSI_TOKEN_HTTP_CONTENT_TYPE,
				(unsigned char *) header,
				strlen(header),
				&buf_pos, buf_end))
		return 1;
	header = "0";
	if (lws_add_http_header_by_token(wsi,
				WSI_TOKEN_HTTP_EXPIRES,
				(unsigned char *) header,
				strlen(header),
				&buf_pos, buf_end))
		return 1;
	header = "no-cache";
	if (lws_add_http_header_by_token(wsi,
				WSI_TOKEN_HTTP_PRAGMA,
				(unsigned char *) header,
				strlen(header),
				&buf_pos, buf_end))
		return 1;
	header = "no-cache, no-store, must-revalidate";
	if (lws_add_http_header_by_token(wsi,
				WSI_TOKEN_HTTP_CACHE_CONTROL,
				(unsigned char *) header,
				strlen(header),
				&buf_pos, buf_end))
		return 1;

	if (lws_finalize_http_header(wsi, &buf_pos, buf_end))
		return 1;
	*buf_pos = '\0';
	n = lws_write(wsi, buffer + LWS_PRE,
			buf_pos - (buffer + LWS_PRE),
			LWS_WRITE_HTTP_HEADERS);
	if (n < 0) {
		// TODO receiver cleanup
		return -1;
	}

	lws_callback_on_writable(wsi);
	return 0;
}

int init_http_session(struct lws *wsi, void *user, void *in, size_t len)
{
	struct http_user_data *data = (struct http_user_data *) user;
	const char *stream;

	(void) user;
	(void) len;

	puts("Received LWS_CALLBACK_HTTP");
	printf("URL requested: %s\n", (char *) in);

	data->fd = -1;
	data->url = (char *) in;
	stream = stream_name((char *) in);
	if (stream) {
		return handle_new_stream(wsi, stream, data);
	} else if (data->url == "/" || data->url == "/index.html") {
		int n;
		n = lws_serve_http_file(wsi, "../web/index.html", "text/html",
				nullptr, 0);
		if (n < 0 || ((n > 0) && lws_http_transaction_completed(wsi)))
			return -1;
		return 0;
	} else {
		lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, nullptr);
		return -1;
	}
}

int send_audio(struct lws *wsi, struct http_user_data *data)
{
	size_t max = sizeof(data->buf) - LWS_PRE;
	ssize_t res;
	unsigned char *buffer = (unsigned char *) data->buf;

	res = read(data->fd, data->buf + LWS_PRE, max);
	if (res <= 0) {
		if (errno == EAGAIN) {
			goto call_again;
		} else {
			perror("read");
			return -1;
		}
	}
	res = lws_write(wsi, buffer + LWS_PRE, res, LWS_WRITE_HTTP);
	if (res < 0) {
		puts("lws_write failed");
		return -1;
	}
	lws_set_timeout(wsi, PENDING_TIMEOUT_HTTP_CONTENT, 5);

call_again:
	lws_callback_on_writable(wsi);
	return 0;
}

int http_cb(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	struct http_user_data *data = (struct http_user_data *) user;

	switch (reason) {
	case LWS_CALLBACK_HTTP:
		return init_http_session(wsi, user, in, len);
	case LWS_CALLBACK_HTTP_FILE_COMPLETION:
		goto try_to_reuse;
	case LWS_CALLBACK_HTTP_WRITEABLE:
		if (data->fd < 0)
			goto try_to_reuse;
		return send_audio(wsi, data);
	default:
		break;
	}
	return 0;

try_to_reuse:
	if (lws_http_transaction_completed(wsi))
		return -1;
	return 0;
}
