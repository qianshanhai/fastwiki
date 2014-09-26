/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "q_util.h"
#include "q_socket.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include "wiki_socket.h"

#define BASE_BIND_PORT 8218

static int m_bind_port = -1;
static int m_bind_sock = -1;
static ws_wait_func_t m_wait_func = NULL;
static ws_do_url_t m_do_url = NULL;

WikiSocket::WikiSocket()
{
	if (m_bind_sock != -1)
		close(m_bind_sock);

	m_start_flag = 0;
	m_pthread_total = 0;
}

WikiSocket::~WikiSocket()
{
#ifndef WIN32
	if (m_start_flag) {
		for (int i = 0; i < m_pthread_total; i++) {
			pthread_kill(m_id[i], SIGKILL);
		}
	}
#endif
}

void WikiSocket::ws_init(ws_wait_func_t wait_func, ws_do_url_t do_url, void *do_url_class)
{
	m_wait_func = wait_func;
	m_do_url = do_url;
	m_do_url_class = do_url_class;
}

struct my_thread_arg {
	void *type;
	int idx;
	int bind_sock;
};

static void *ws_thread(void *arg)
{
	struct my_thread_arg *p = (struct my_thread_arg *)arg;
	WikiSocket *ws = (WikiSocket *)p->type;

	ws->ws_one_thread(p->idx);

	return NULL;
}

extern "C" int LOG(const char *fmt, ...);

#include <sys/time.h>

int WikiSocket::ws_one_thread(int idx)
{
	int sock;

	HttpParse *http = new HttpParse();

	for (;;) {
		sock = q_accept(m_bind_sock);
		if (sock == -1) {
			if (errno == EINTR) {
				q_sleep(1);
				continue;
			}
			break;
		}
		http->hp_init(sock);
		m_do_url(m_do_url_class, (void *)this, http, sock, idx);

#ifdef WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	}

	delete http;

	return 0;
}

int WikiSocket::ws_get_bind_port()
{
	return m_bind_port;
}

int WikiSocket::ws_start_http_server(int pthread_total, bool flag)
{
	int i, ret;
	struct my_thread_arg arg;

	m_pthread_total = pthread_total;

	for (i = 0; i < 1000; i++) {
		m_bind_port = BASE_BIND_PORT + i;
		if ((m_bind_sock = q_bind(m_bind_port, flag)) >= 0)
			break;
	}

	if (m_bind_sock == -1)
		return -1;

	if (m_pthread_total <= 0) {
#ifndef WIN32
		for (int i = 0; i < -m_pthread_total; i++) {
			pid_t pid;
			if ((pid = fork()) == 0) {
				ws_one_thread(i);
				exit(0);
			}
		}
#endif
	} else {
		for (int i = 0; i < m_pthread_total; i++) {
			arg.type = (void *)this;
			arg.idx = i;
			ret = pthread_create(&m_id[i], NULL, ws_thread, &arg);
			if (ret != 0){
				return -1;
			}
#ifdef WIN32
			_sleep(1);
#else
			usleep(300000);
#endif
		}
	}

	if (m_wait_func) {
		(*m_wait_func)();
	}

	m_start_flag = 1;

	return 0;
}

int WikiSocket::ws_http_output_head(int sock, int code, const char *type, int body_size)
{
	int n;
	char buf[4096];

	n = sprintf(buf,
			"%s\r\n"
			"Server: fastwiki/" _VERSION "\r\n"
			"Pragma: no-cache\r\n"
			"Cache-Control: no-cache\r\n"
			"Connection: close\r\n"
			"Accept-Ranges: bytes\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: %s\r\n\r\n",
			(code == 200 ? "HTTP/1.1 200 OK" : "HTTP/1.1 404 Not Found"),
			(int)body_size, type);

	send(sock, buf, n, 0);

	return 0;
}

int WikiSocket::ws_http_output_body(int sock, const char *data, int size)
{
	if (size > 0) {
		send(sock, data, size, 0);
	}

	return 0;
}

int WikiSocket::ws_get_server_status()
{
	return m_start_flag;
}
