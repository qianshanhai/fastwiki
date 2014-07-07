/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_SOCKET_H
#define __WIKI_SOCKET_H

#include <pthread.h>

typedef void (*ws_wait_func_t)();
typedef int (*ws_do_url_t)(void *_class, void *type, int sock, const char *url, int idx);

class WikiSocket {
	public:
		WikiSocket();
		~WikiSocket();

		void ws_init(ws_wait_func_t wait_func, ws_do_url_t do_url, void *do_url_class = NULL);
		int ws_start_http_server(int pthread_total, bool flag = true);
		int ws_get_bind_port();
		int ws_one_thread(int idx);
		int ws_http_output_head(int sock, int code, const char *type, int body_size);
		int ws_http_output_body(int sock, const char *data, int size);
		const char *ws_get_host();

		int ws_get_server_status();

	private:

		pthread_t m_id[128];
		int m_start_flag;
		int m_pthread_total;
		char m_host[128];

		void *m_do_url_class;

		int ws_parse_url(char *buf, char *url, int u_len);
};

#endif
