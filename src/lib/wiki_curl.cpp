/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>

#include "wiki_curl.h"

WikiCurl::WikiCurl()
{
}

WikiCurl::~WikiCurl()
{
}

int WikiCurl::curl_init()
{
	curl_global_init(CURL_GLOBAL_ALL);

	memset(m_buf, 0, sizeof(m_buf));
	memset(m_proxy, 0, sizeof(m_proxy));

	return 0;
}

int WikiCurl::curl_set_proxy(const char *proxy)
{
	strncpy(m_proxy, proxy, sizeof(m_proxy) - 1);

	return 0;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{ 
	int tmp = size * nmemb;
	struct curl_data *p = (struct curl_data *)stream;

	if (tmp > p->max_len - p->len) {
		p->flag = 1;
		return 0;
	}

	memcpy(p->data + p->len, ptr, tmp);
	p->len += tmp;

	return tmp;
}

#define MAX_CURL_DATA_LEN (2*1024*1024)

int WikiCurl::curl_get_data(int idx, const char *url, char **data, int *ret_len)
{
	struct curl_data *p;

	if (m_buf[idx].data == NULL) {
		m_buf[idx].data = (char *)malloc(MAX_CURL_DATA_LEN);
		m_buf[idx].max_len = MAX_CURL_DATA_LEN;
	}

	CURL *c = curl_easy_init();
	p = &m_buf[idx];

	p->len = 0;
	p->flag = 0;

	curl_easy_setopt(c, CURLOPT_URL, url);

	if (strlen(m_proxy) > 5)
		curl_easy_setopt(c, CURLOPT_PROXY, m_proxy);

	curl_easy_setopt(c, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0)");

	curl_easy_setopt(c, CURLOPT_WRITEDATA, p);
	curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_data);

	int res = curl_easy_perform(c);
	curl_easy_cleanup(c);

	if (res == CURLE_OK && p->flag == 0) {
		*data = p->data;
		p->data[p->len] = 0;
		if (ret_len)
			*ret_len = p->len;
		return p->len;
	}

	return 0;
}
