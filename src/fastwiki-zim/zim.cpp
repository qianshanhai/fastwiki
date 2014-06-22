/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include "q_util.h"
#include "zim.h"

#include <lzma.h>

#define ZIM_TMP_TITLE_FILE "zim.tmp.title.txt"

#ifdef WIN32
#define lseek _lseeki64
#endif

int lzma_uncompress(const char *in, int in_len, char *out, int out_len)
{
	int ret;

	lzma_action action = LZMA_FINISH;
	lzma_stream strm = LZMA_STREAM_INIT;

#ifdef LZMADEC
	ret = lzma_alone_decoder(&strm, UINT64_MAX);
#else
	ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);
#endif

	strm.avail_in = in_len;
	strm.next_in = (uint8_t *)in;

	strm.avail_out = out_len;
	strm.next_out = (uint8_t *)out;

	ret = lzma_code(&strm, action);
	lzma_end(&strm);

	return ret == LZMA_STREAM_END ? out_len - strm.avail_out : -1;
}

Zim::Zim()
{
}

Zim::~Zim()
{
}

int Zim::zim_init(const char *file)
{
	m_fd = open(file, O_RDONLY | O_BINARY);

	if (m_fd == -1) {
		printf("%s: %s\n", file, strerror(errno));
		return -1;
	}

	read(m_fd, &m_head, sizeof(m_head));

	zim_mime_type();
	zim_cluster();
	zim_init_title();

	return 0;
}

int Zim::zim_cluster()
{
	m_cluster = (unsigned long long *)calloc(m_head.cluster_count + 1, sizeof(unsigned long long));

	lseek(m_fd, m_head.cluster_pos, SEEK_SET);
	read(m_fd, m_cluster, m_head.cluster_count * sizeof(unsigned long long));

	m_cluster[m_head.cluster_count] = file_size(m_fd) + 1;

	unsigned int tmp, max_size = 0;

	for (unsigned int i = 0; i < m_head.cluster_count; i++) {
		tmp = (unsigned int)(m_cluster[i + 1] - m_cluster[i]);
		if (tmp > max_size)
			max_size = tmp;
	}
	m_max_tmp_size = 2 * 1024 * 1024;
	m_max_data_size = m_max_tmp_size * 10;

	m_tmp = (char *)malloc(m_max_tmp_size);
	m_data = (char *)malloc(m_max_data_size);

	m_last_cluster_idx = (unsigned int)-1;
	m_blob_pos = (unsigned int *)m_data;

	return 0;
}

int Zim::zim_mime_type()
{
	char buf[4096];

	memset(buf, 0, sizeof(buf));
	memset(m_mime_type, 0, sizeof(m_mime_type));

	lseek(m_fd, sizeof(m_head), SEEK_SET);
	read(m_fd, buf, sizeof(buf));

	int start = 0;
	int m_mime_type_total = 0;

	for (int i = 0; i < (int)sizeof(buf) - 1; i++) {
		if (buf[i] == 0) {
			strcpy(m_mime_type[m_mime_type_total++], buf + start);
			start = i + 1;
			if (buf[i + 1] == 0) {
				break;
			}
		}
	}

#ifdef DEBUG
	for (int i = 0; i < m_mime_type_total; i++) {
		printf("%s\n", m_mime_type[i]);
	}
#endif

	return 0;
}

int _cmp_zim_idx(const void *a, const void *b)
{
	struct zim_tmp_title *p1 = (struct zim_tmp_title *)a;
	struct zim_tmp_title *p2 = (struct zim_tmp_title *)b;

	if (p1->cluster_idx == p2->cluster_idx)
		return p1->blob_idx - p2->blob_idx;

	return p1->cluster_idx - p2->cluster_idx;
}

int Zim::zim_init_title()
{
	char *title, buf[2048];
	struct article_header *h;
	FILE *tmp_fp = fopen(ZIM_TMP_TITLE_FILE, "wb+");
	int len, pos = 0;

#ifdef DEBUG
	printf("article_count: %d, %llu\n", m_head.article_count, m_head.url_pos);
#endif

	m_url_pos = (unsigned long long *)calloc(m_head.article_count + 1, sizeof(unsigned long long));

	lseek(m_fd, m_head.url_pos, SEEK_SET);
	read(m_fd, m_url_pos, m_head.article_count * sizeof(unsigned long long));


	struct zim_tmp_title *title_pos;
	title_pos = (struct zim_tmp_title *)calloc(m_head.article_count + 1, sizeof(struct zim_tmp_title));

	m_max_blob_idx = 0;

	for (unsigned int i = 0; i < m_head.article_count; i++) {
		lseek(m_fd, m_url_pos[i], SEEK_SET);
		read(m_fd, buf, sizeof(buf));

		h = (struct article_header *)buf;

		if (h->mime_type == 0xfffe || h->mime_type == 0xfffd) {
			continue;
		}

		struct zim_tmp_title *p = &title_pos[i];
		p->mime_type = h->mime_type;
		p->parameter_len = h->parameter_len;
		p->name_space = h->name_space;
		p->cluster_idx = h->cluster_idx;

		char *x = h->url;
		if (h->mime_type == 0xffff) {
			p->blob_idx = (unsigned int)-1;
			x -= 4;
		} else {
			p->blob_idx = h->blob_idx;
			if (h->blob_idx > m_max_blob_idx)
				m_max_blob_idx = h->blob_idx;
		}

		title = x + strlen(x) + 1;
		if (title[0] == 0) {
			title = x;
		}

		p->title_pos = pos;

		len = strlen(title) + 1;
		pos += len;

		fwrite(title, len, 1, tmp_fp);
	}

	fclose(tmp_fp);

#ifdef DEBUG
	printf("m_max_blob_idx = %d\n", m_max_blob_idx);
	fflush(stdout);
#endif

	m_blob_cache = (unsigned int *)calloc(m_max_blob_idx + 1024, sizeof(unsigned int));

	for (unsigned int i = 0; i < m_head.article_count; i++) {
		struct zim_tmp_title *p = &title_pos[i];
		if (zim_is_redirect(p)) {
			p->cluster_idx = title_pos[p->cluster_idx].title_pos;
		}
	}

	qsort(title_pos, m_head.article_count, sizeof(struct zim_tmp_title), _cmp_zim_idx);

	free(m_url_pos);

	size_t size = file_size(ZIM_TMP_TITLE_FILE);

	char *all_title = (char *)malloc(size + 1);

	int fd = open(ZIM_TMP_TITLE_FILE, O_RDONLY | O_BINARY);
	read(fd, all_title, size);
	close(fd);

	tmp_fp = fopen(ZIM_TMP_TITLE_FILE, "wb+");
	pos = 0;

	char *to, *from;

	for (unsigned int i = 0; i < m_head.article_count; i++) {
		struct zim_tmp_title *p = &title_pos[i];
		if (zim_is_redirect(p)) {
			to = all_title + p->cluster_idx;
			from = all_title + p->title_pos;
			p->title_pos = strlen(from) + 1;
			p->cluster_idx = strlen(to) + 1;

			fwrite(p, sizeof(struct zim_tmp_title), 1, tmp_fp);
			fwrite(from, p->title_pos, 1, tmp_fp); 
			fwrite(to, p->cluster_idx, 1, tmp_fp);
		} else {
			from = all_title + p->title_pos;
			p->title_pos = strlen(from) + 1;

			fwrite(p, sizeof(struct zim_tmp_title), 1, tmp_fp);
			fwrite(from, p->title_pos, 1, tmp_fp); 
		}
	}

	fclose(tmp_fp);

	m_title_fp = fopen(ZIM_TMP_TITLE_FILE, "rb");

	free(all_title);
	free(title_pos);

	return 0;
}

int Zim::zim_title_reset()
{
	fseek(m_title_fp, 0, SEEK_SET);

	return 0;
}

int Zim::zim_title_read(struct zim_tmp_title *st, char *title, char *redirect)
{
	title[0] = redirect[0] = 0;

	if (fread(st, sizeof(struct zim_tmp_title), 1, m_title_fp) == 0)
		return 0;

	fread(title, st->title_pos, 1, m_title_fp);

	if (zim_is_redirect(st)) {
		fread(redirect, st->cluster_idx, 1, m_title_fp);
	}

	return 1;
}

int Zim::zim_data_reset()
{
	m_last_cluster_idx = (unsigned int)-1;

	return zim_title_reset();
}

#include <stdarg.h>
static int read_total = 0;
static FILE *m_log_fp = NULL;

int mylog(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (m_log_fp == NULL) {
		m_log_fp = fopen("fastwiki-zim-log.txt", "w+");
	}
	vfprintf(m_log_fp, fmt, ap);
	fflush(m_log_fp);

	va_end(ap);

	return 0;
}

int Zim::zim_data_read(struct zim_tmp_title *st, char *title, char *redirect, char **data, int *ret_size, int flag)
{
	unsigned int size;
	int t_size, done;
	unsigned char blob_flag = 0;

	done = zim_title_read(st, title, redirect);

	read_total++;

	//mylog("total = %d\n", read_total);

	if (done == 0)
		return 0;

	if (zim_is_redirect(st))
		return 1;

	if (flag) {
		if (st->name_space == 'A')
			return 2;
	}

	if (st->cluster_idx != m_last_cluster_idx) {
		if (st->cluster_idx >= m_head.cluster_count) {
			mylog("cluster_idx = %d > %d\n", st->cluster_idx, m_head.cluster_count);
			return -1;
		}
		size = m_cluster[st->cluster_idx + 1] - m_cluster[st->cluster_idx] - 1;

		if (lseek(m_fd, m_cluster[st->cluster_idx], SEEK_SET) == (off_t)-1) {
			mylog("lseek error: total=%d, cluster_idx=%u, offset=%lld\n", read_total, st->cluster_idx, m_cluster[st->cluster_idx]);
			return -1;
		}

		if (read(m_fd, &blob_flag, 1) <= 0) 
			return -1;

		if (blob_flag == 4) {
			if (size >= m_max_tmp_size) {
				free(m_tmp);
				free(m_data);
				m_max_tmp_size = size + 1024;
				m_max_data_size = m_max_tmp_size * 20;
				m_tmp = (char *)malloc(m_max_tmp_size);
				m_data = (char *)malloc(m_max_data_size);
				if (m_tmp == NULL || m_data == NULL) {
					mylog("malloc error: old_size = %u, size = %u + %u\n", size, m_max_tmp_size, m_max_data_size);
					return 0;
				}
			}
			read(m_fd, m_tmp, size);
			t_size = lzma_uncompress(m_tmp, (int)size, m_data, m_max_data_size);
			m_data[t_size] = 0;
			if (t_size <= 0) {
				mylog("lzma_uncompress error\n");
				return -1;
			}
			m_mem_flag = 1;
		} else {
			if (size > m_max_data_size) {
				m_mem_flag = 0;
#ifdef DEBUG
				printf("test: %d, blob_idx=%d, %d, %d\n", size, st->blob_idx, m_blob_cache[0], m_blob_cache[1]);
				fflush(stdout);
#endif
				read(m_fd, m_blob_cache, (m_max_blob_idx + 2) * sizeof(unsigned int));
			} else {
				read(m_fd, m_data, size);
				m_data[size] = 0;
				m_mem_flag = 1;
			}
		}
		m_last_cluster_idx = st->cluster_idx;
	}

	if (m_mem_flag == 1) {
		*data = m_data + m_blob_pos[st->blob_idx];
		*ret_size = m_blob_pos[st->blob_idx + 1] - m_blob_pos[st->blob_idx];
	} else {
		*data = m_data;
		*ret_size = m_blob_cache[st->blob_idx + 1] - m_blob_cache[st->blob_idx];
		if (*ret_size >= (int)m_max_data_size)
			*ret_size = m_max_data_size - 1;

#ifdef DEBUG
		printf("ret_size = %d\n", *ret_size);
#endif

		lseek(m_fd, m_cluster[st->cluster_idx] + 1 + m_blob_pos[st->blob_idx], SEEK_SET);
		read(m_fd, m_data, *ret_size);
	}

	return 1;
}

int Zim::zim_done()
{
	fclose(m_title_fp);
	unlink(ZIM_TMP_TITLE_FILE);

	free(m_data);
	free(m_tmp);

	close(m_fd);

	return 0;
}
