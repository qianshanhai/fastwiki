/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_CURL_H
#define __WIKI_CURL_H

#define MAX_CURL_PTHREAD 128

struct curl_data {
	char *data;
	int len;
	int max_len;
	int flag;
};

class WikiCurl {
	private:
		char m_proxy[1024];
		struct curl_data m_buf[MAX_CURL_PTHREAD];

	public:
		WikiCurl();
		~WikiCurl();

		int curl_init();
		int curl_set_proxy(const char *proxy);
		int curl_get_data(int idx, const char *url, char **data, int *ret_len = NULL);
};

#endif
