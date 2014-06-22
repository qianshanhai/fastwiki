/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "image_parse.h"
#include "q_util.h"

ImageParse::ImageParse()
{
}

ImageParse::~ImageParse()
{
}

int ImageParse::ip_init()
{
	m_total = 0;
	m_tmp = (char *)malloc(1024*1024);

	return 0;
}

int ImageParse::ip_parse(const char *file)
{
#ifdef __linux__
	mapfile_t mt;

	q_mmap(file, &mt);
	ip_parse((char *)mt.start, mt.size);

	q_munmap(&mt);
#endif

	return 0;
}

int ImageParse::ip_push(const char *url)
{
	char *t, *x;
	struct image_parse *p = &m_buf[m_total++];

	strcpy(p->href, url);
	p->px = -1;

	if ((t = strrchr(p->href, '/'))) {
		t++;
		x = t;
		for (x = t; *x; x++) {
			if (!(*x >= '0' && *x <= '9'))
				break;
		}
		if (x > t && strncasecmp(x, "px", 2) == 0) {
			p->px = atoi(t);
		}
	}

	return 0;
}

int ImageParse::ip_get_url(char *buf)
{
	int idx = 0;

	if (m_total <= 0)
		return 0;

	for (int i = 1; i < m_total; i++) {
		if (m_buf[idx].px == -1 || (m_buf[i].px > 0 && m_buf[i].px < m_buf[idx].px)) {
			idx = i;
		}
	}

	strcpy(buf, m_buf[idx].href);

	return 1;
}

#define my_strncasecmp(x, y) strncasecmp(x, y, sizeof(y) - 1)

int ImageParse::ip_parse(const char *data, int size)
{
	const char *p = data;
	char *tmp = m_tmp;
	int pos = 0;

	m_total = 0;

	for (int i = 0; i < size; i++) {
		if (my_strncasecmp(p + i, "fullImageLink") == 0) {
			for (; i < size; i++) {
				tmp[pos++] = p[i];
				if (my_strncasecmp(p + i, "fullMedia") == 0)
					goto out;
			}
		}
	}
out:
	tmp[pos] = 0;

	if (pos == 0) {
		return -1;
	}

	char *x, *y, buf[2048];

	for (x = tmp; x - tmp < pos; x++) {
		if (my_strncasecmp(x, "href=\"") == 0) {
			x += 6;
			if ((y = strchr(x, '"'))) {
				*y = 0;
				strncpy(buf, x, y - x);
				buf[y - x] = 0;
				ip_push(buf);
			}
		}
	}

	return 0;
}

