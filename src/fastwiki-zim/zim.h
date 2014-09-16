/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __FW_ZIM_H
#define __FW_ZIM_H

#include <stdio.h>

struct zim_header {
	unsigned int magic_number;
	int version;
	unsigned int uuid[4];
	unsigned int article_count;
	unsigned int cluster_count;
	unsigned long long url_pos;
	unsigned long long title_pos;
	unsigned long long cluster_pos;
	unsigned long long mime_pos;
	unsigned int main_page;
	unsigned int layout_page;
	unsigned long long checksum_pos;
};

struct article_header {
	unsigned short mime_type;
	unsigned char parameter_len;
	unsigned char name_space;
	unsigned int revision;
	unsigned int cluster_idx;
	unsigned int blob_idx;
	char url[4];
};

struct zim_tmp_title {
	unsigned short mime_type;
	unsigned char parameter_len;
	unsigned char name_space;
	unsigned int cluster_idx;
	unsigned int blob_idx;     /* -1 for redirect */
	unsigned int title_pos;
};

#define zim_is_redirect(x) ((x)->blob_idx == (unsigned int)-1)

class Zim {
	private:
		int m_fd;
		struct zim_header m_head;
		char m_mime_type[32][64];
		int m_mime_type_total;

		unsigned long long *m_url_pos;
		unsigned long long *m_cluster;

		unsigned int *m_blob_pos;
		unsigned int *m_blob_cache;
		unsigned int m_max_blob_idx;

		char *m_tmp;
		char *m_data;

		unsigned int m_max_tmp_size;
		unsigned int m_max_data_size;

		unsigned int m_last_cluster_idx;
		int m_mem_flag;

		FILE *m_title_fp;

	public:
		Zim();
		~Zim();

	public:
		int zim_init(const char *file);
		int zim_title_reset();
		int zim_title_read(struct zim_tmp_title *st, char *title, char *redirect);

		int zim_data_reset();
		int zim_data_read(struct zim_tmp_title *st, char *title, char *redirect,
				char **data, int *ret_size, int flag);

		int zim_data_sys_read(struct zim_tmp_title *st, char **data, int *ret_size, int flag);

		int zim_done();

	private:
		int zim_mime_type();
		int zim_cluster();
		int zim_init_title();
};

#endif
