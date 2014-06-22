/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __IMAGE_PARSE_H
#define __IMAGE_PARSE_H

struct image_parse {
	char href[512];
	int px;
};

class ImageParse {
	private:
		struct image_parse m_buf[30];
		int m_total;

		char *m_tmp;

	public:
		ImageParse();
		~ImageParse();

		int ip_init();
		int ip_parse(const char *file);
		int ip_parse(const char *data, int size);
		int ip_push(const char *url);
		int ip_get_url(char *buf);
};

#endif
