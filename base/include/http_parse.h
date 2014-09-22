/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __FASTWIKI_HTTP_PARSE_H
#define __FASTWIKI_HTTP_PARSE_H

struct http_param {
	char name[32];
	char value[192];
};

#define _MAX_HTTP_COOKIE_TOTAL 16
#define _MAX_HTTP_PARAM_TOTAL 16

class HttpParse {
	private:
		char m_host[32];
		char m_url[64];
		char m_zero[4];

		struct http_param m_cookie[_MAX_HTTP_COOKIE_TOTAL];
		int m_cookie_total;

		struct http_param m_param[_MAX_HTTP_PARAM_TOTAL];
		int m_param_total;

	public:
		HttpParse();
		~HttpParse();

	public:
		int hp_init(int sock);
		int hp_init_all(char *buf, int len);

		char *hp_param(const char *name);
		char *hp_cookie(const char *name);

		const char *hp_host();
		const char *hp_url();

		void hp_stat();

	private:
		int hp_init_host(char *p);
		int hp_init_cookie(char *p);
		int hp_init_param(char *p);

		char *hp_www_decode(const char *buf, char *tmp, int len);
		char *hp_fetch(struct http_param *buf, int total, const char *name);

		void hp_print_param(struct http_param *buf, int total);
};

#endif
