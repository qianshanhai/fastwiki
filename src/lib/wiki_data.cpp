/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "q_log.h"

#include "wiki_index.h"
#include "wiki_data.h"
#include "gzip_compress.h"

WikiData::WikiData()
{
	m_out_size = 0;
	m_page_count = 0;
	m_page_size = sizeof(data_head_t);

	for (int i = 0; i < MAX_FD_TOTAL; i++)
		m_fd_idx[i] = -1;

	m_data_file_total = 0;

	m_buf = NULL;
	m_tmp = NULL;
}

WikiData::~WikiData()
{
	for (int i = 0; i < MAX_FD_TOTAL; i++) {
		if (m_fd_idx[i] >= 0)
			close(m_fd_idx[i]);
	}

	if (m_tmp)
		free(m_tmp);

	if (m_buf)
		free(m_buf);
}

int WikiData::wd_init(const fw_files_t file, int total)
{
	int i, fd, all_page_count = 0;

	m_fd_total = 0;

	for (i = 0; i < MAX_FD_TOTAL; i++) {
		m_fd_idx[i] = -1;
	}

	for (i = 0; i < total; i++) {
		if ((fd = open(file[i], O_RDONLY | O_BINARY)) == -1) {
			LOG("open file %s to read error: %s\n", file[i], strerror(errno));
			break;
		}

		read(fd, &m_head, sizeof(m_head));
		all_page_count += m_head.page_count;
		m_fd_idx[m_head.file_idx] = fd;
		m_fd_total++;

		if (m_head.max_old_len > m_out_size)
			m_out_size = m_head.max_old_len;

		m_z_flag = m_head.z_flag;
	}

	m_head.page_count = all_page_count;
#ifdef DEBUG
	printf("all_page=%d, date=%s\n", all_page_count, m_head.date);
#endif

	for (i = 0; i < m_fd_total; i++) {
		if (m_fd_idx[i] == -1) {
			printf("err total=%d\n", total);
			return -1;
		}
	}

	m_out_size += 4096;

	if (m_out_size < 2*1024*1024)
		m_out_size = 2*1024*1024;

	m_tmp = (char *)malloc(m_out_size);
	m_buf = (char *)malloc(m_out_size);

	if (m_buf == NULL || m_tmp == NULL)
		return -1;

	return 0;
}

int WikiData::wd_check_fd(int data_file_idx)
{
	if (data_file_idx >= MAX_FD_TOTAL || data_file_idx < 0)
		return -1;
	
	if (m_fd_idx[data_file_idx] == -1)
		return -1;

	return 0;
}

int WikiData::wd_get_head(data_head_t *h)
{
	memset(h, 0, sizeof(data_head_t));

	if (m_fd_idx[0] != -1)
		memcpy(h, &m_head, sizeof(data_head_t));
		
	return 0;
}

int WikiData::wd_sys_read(int file_idx, unsigned int pos, int len, char *ret, int ret_max)
{
	int fd, z_len;
	data_record_head_t head;
	old_data_record_head_t old_head;

	if ((fd = m_fd_idx[file_idx]) == -1)
		return -1;

	if (lseek(fd, pos, SEEK_SET) == (off_t)-1)
		return -1;

	/* TODO can read once */
	if (m_head.version[0] == 0) {
		read(fd, &old_head, sizeof(old_head));
		if (old_head.out_len <= 0 || old_head.out_len >= m_out_size)
			return -1;
		if ((z_len = read(fd, m_tmp, old_head.out_len)) != old_head.out_len)
			return -1;;
	} else {
		read(fd, &head, sizeof(head));
		if (head.out_len <= 0 || head.out_len >= m_out_size)
			return -1;
		if ((z_len = read(fd, m_tmp, head.out_len)) != head.out_len)
			return -1;
	}

	if (ret_max == 0) {
		memcpy(ret, m_tmp, z_len);
		return z_len;
	}

	int n;

	if (m_z_flag == FM_FLAG_LZ4) {
		n = lz4_decompress(ret, ret_max, m_tmp, z_len);
	} else if (m_z_flag == FM_FLAG_GZIP) {
		n = gunzip(ret, ret_max,  m_tmp, z_len);
	} else if (m_z_flag == FM_FLAG_TEXT) {
		memcpy(ret, m_tmp, z_len); 
		n = z_len;
	}

	if (n == -1)
		return -1;

	ret[n] = 0;

	return n;
}

#if 0
int WikiData::wd_read(int file_idx, unsigned int pos, int len, char **ret)
{
	int size, tmp_len;

	memcpy(m_buf, m_html_start, m_html_start_len);
	size = m_html_start_len;

	if ((tmp_len = wd_sys_read(file_idx, pos, len, m_buf + size, m_out_size - size)) == -1)
		return -1;

	size += tmp_len;

	/*
	memcpy(m_buf + size, HTML_END, sizeof(HTML_END) - 1);
	size += sizeof(HTML_END) - 1;
	*/

	m_buf[size] = 0;
	*ret = m_buf;

	return size;
}

#endif

#include "wiki_common.h"

void WikiData::wd_init_data_head(data_head_t *p)
{
	memset(p, 0, sizeof(data_head_t));

	p->flag = 0xfd;
	strcpy(p->name, "fastwiki");
	strcpy(p->version, _VERSION);

	p->z_flag = m_z_flag;
	p->file_idx = m_data_file_total;
	p->complete_flag = wiki_is_complete();

	strncpy(p->date, m_date, sizeof(p->date) - 1);
	strncpy(p->lang, m_lang, sizeof(p->lang) - 1);
}

int WikiData::wd_output_current_file()
{
	m_head.page_count = m_page_count;
	m_head.file_size = m_page_size + sizeof(m_head);

	fflush(m_data_fp);

	fseek(m_data_fp, 0L, SEEK_SET);
	fwrite(&m_head, sizeof(m_head), 1, m_data_fp);

	fflush(m_data_fp);
	fclose(m_data_fp);

	m_data_fp = NULL;

	return 0;
}

int WikiData::wd_file_list(char *fname_list)
{
	int n = 0;

	for (int i = 0; i < m_fname_total; i++) {
		n += sprintf(fname_list + n, "\t%s\n", m_fname_list[i]);
	}

	return n;
}

int WikiData::wd_new_file()
{
	char buf[1024];

	if (m_data_file_total > 0) {
		wd_output_current_file();
		m_page_count = 0;
		m_page_size = sizeof(data_head_t);
	}

	wd_init_data_head(&m_head);

	sprintf(buf, "%s.%d", m_fname, m_data_file_total);
	strcpy(m_fname_list[m_fname_total++], buf);

	if ((m_data_fp = fopen(buf, "wb+")) == NULL) {
		printf("file %s: %s\n", buf, strerror(errno));
		return -1;
	}

	setvbuf(m_data_fp, m_tmp, _IOFBF, 10*1024*1024);
	
	fwrite(&m_head, sizeof(m_head), 1, m_data_fp);
	m_data_file_total++;

	return 0;
}

int WikiData::wd_init(const char *outfile, int z_flag, const char *date, const char *lang)
{
	m_data_file_total = 0;

	m_z_flag = z_flag;

	strncpy(m_fname, outfile, sizeof(m_fname) - 1);
	strncpy(m_date, date, sizeof(m_date) - 1);
	strncpy(m_lang, lang, sizeof(m_lang) - 1);

	m_out_size = OUT_SIZE;

	m_fname_list = (char **)calloc(MAX_FD_TOTAL, sizeof(char *));

	char *T = (char *)malloc(64 * MAX_FD_TOTAL);
	for (int i = 0; i < MAX_FD_TOTAL; i++) {
		m_fname_list[i] = T + 64 * i;
	}
	m_fname_total = 0;

	m_tmp = (char *)malloc(10*1024*1024+1024);

	return wd_new_file();
}

int WikiData::wd_add_one_page(int flag, const char *page, int page_z_size, int page_old_size,
		int *ret_file_idx, unsigned int *ret_pos, int *ret_size)
{
	const char *buf = page;
	data_record_head_t h;

	h.old_len = page_old_size;
	h.out_len = page_z_size;
	//h.flag = 0;

	if (page_old_size > m_head.max_old_len)
		m_head.max_old_len = page_old_size;

	fwrite(&h, sizeof(h), 1, m_data_fp);
	fwrite(buf, h.out_len, 1, m_data_fp);

	*ret_size = h.out_len + sizeof(data_record_head_t);
	*ret_pos = m_page_size;
	*ret_file_idx = m_data_file_total - 1;

	m_page_count++;
	m_page_size += *ret_size;

	if (m_page_size >= wiki_split_size()) {
		if (wd_new_file() == -1)
			return -1;
		return 1;
	}

	return 0;
}

int WikiData::wd_output()
{
	m_head.end_file_flag = 1;

	return wd_output_current_file();
}
