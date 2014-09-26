/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <stdarg.h>

#include "q_util.h"
#include "wiki_scan_file.h"

#include "q_log.h"

WikiScanFile::WikiScanFile()
{
}

WikiScanFile::~WikiScanFile()
{
}

int WikiScanFile::wsf_scan_file(struct file_st *file, int max_total, fw_dir_t *dir, int dir_total)
{
	int file_total = 0;

	for (int i = 0; i < dir_total; i++) {
		if (wsf_scan_dir(dir[i].path, file, &file_total, max_total) == -1)
			break;
	}

	return file_total;
}

int WikiScanFile::wsf_scan_dir(const char *dir, struct file_st *file, int *total, int max_total)
{
	DIR *dirp;
	struct dirent *d;
	char buf[256];

	if ((dirp = opendir(dir)) == NULL)
		return 0;

	while ((d = readdir(dirp))) {
		if (d->d_name[0] == '.')
			continue;

		snprintf(buf, sizeof(buf), "%s/%s", dir, d->d_name);
		if (dashf(buf)) {
			wsf_scan_lang(dir, d->d_name, file, total, max_total);
		}
	}

	closedir(dirp);

	return 0;
}

struct file_st *WikiScanFile::wsf_get_file_st(struct file_st *file, int *total,
		int max_total, const char *lang)
{
	struct file_st *p;

	for (int i = 0; i < *total; i++) {
		p = &file[i];
		if (strcmp(p->lang, lang) == 0)
			return p;
	}

	if (*total >= max_total)
		return NULL;

	p = &file[(*total)++];
	memset(p, 0, sizeof(struct file_st));
	strncpy(p->lang, lang, sizeof(p->lang) - 1);

	return p;
}

int WikiScanFile::wsf_check_fw_name(const char *name, char *lang)
{
	split_t sp;

	if (split('.', name, sp) < 3 || strcmp(sp[0], "fastwiki") != 0)
		return 0;

	if (strcmp(sp[1], "idx") == 0 || strcmp(sp[1], "dat") == 0
			|| strcmp(sp[1], "fidx") == 0 || strcmp(sp[1], "math") == 0
			|| strcmp(sp[1], "image") == 0) {
		strcpy(lang, sp[2]);
		return 1;
	}

	return 0;
}

#define my_strncmp(s, STR) strncmp(s, STR, sizeof(STR) - 1)

int WikiScanFile::wsf_scan_lang(const char *dir, const char *fname, struct file_st *file,
		int *total, int max_total)
{
	char lang[32];
	struct file_st *p = NULL;

	if (wsf_check_fw_name(fname, lang))  
		p = wsf_get_file_st(file, total, max_total, lang);

	if (p == NULL)
		return -1;

	if (my_strncmp(fname, WK_DAT_PREFIX) == 0) {
		sprintf(p->data_file[p->data_total++], "%s/%s", dir, fname);
	} else if (my_strncmp(fname, WK_IDX_PREFIX) == 0) {
		sprintf(p->index_file, "%s/%s", dir, fname);
	} else if (my_strncmp(fname, WK_MATH_PREFIX) == 0) {
		sprintf(p->math_file, "%s/%s", dir, fname);
	} else if (my_strncmp(fname, WK_IMAGE_PREFIX) == 0) {
		sprintf(p->image_file[p->image_total++], "%s/%s", dir, fname);
	} else if (my_strncmp(fname, WK_FIDX_PREFIX) == 0) {
		sprintf(p->fidx_file[p->fidx_file_total++], "%s/%s", dir, fname);
	}

	return 0;
}

int WikiScanFile::wsf_fetch_fw_dir(fw_dir_t *dir, int max_total, char *s_dir[], int s_total)
{
	int total = 0;

	m_dir_total = 0;
	memset(m_dir, 0, sizeof(m_dir));

	for (int i = 0; i < s_total; i++) {
		wsf_scan_sdcard(s_dir[i]);
	}

	for (int i = 0; i < m_dir_total; i++) {
		if (total >= max_total)
			break;
		memcpy(&dir[total++], &m_dir[i], sizeof(fw_dir_t));
	}

	return total;
}

int WikiScanFile::wsf_scan_sdcard(const char *dir)
{
	DIR *dirp;
	struct dirent *d;
	char file[256], lang[32];

	if ((dirp = opendir(dir)) == NULL)
		return -1;

	while ((d = readdir(dirp))) {
		if (d->d_name[0] == '.')
			continue;

		sprintf(file, "%s/%s", dir, d->d_name);
		if (dashd(file)) {
			wsf_scan_sdcard(file);
		} else if (dashf(file)) {
			if (wsf_check_fw_name(d->d_name, lang))
				wsf_add_dir(dir);
		}
	}

	closedir(dirp);

	return 0;
}

int WikiScanFile::wsf_check_dir(fw_dir_t *d, int total, const char *dir, const struct stat *st)
{
	for (int i = 0; i < total; i++) {
		fw_dir_t *p = &d[i];
		if (memcmp(&p->stat.st_dev, &st->st_dev, sizeof(st->st_dev)) == 0
				&& memcmp(&p->stat.st_ino, &st->st_ino, sizeof(st->st_ino)) == 0)
			return 0;
	}

	return 1;
}

int WikiScanFile::wsf_add_dir(const char *dir)
{
	struct stat st;

	if (m_dir_total >= MAX_FW_DIR_TOTAL)
		return 0;

	if (stat(dir, &st) == -1)
		return 0;

	if (wsf_check_dir(m_dir, m_dir_total, dir, &st) == 0)
		return 0;

	fw_dir_t *p = &m_dir[m_dir_total++];

	strncpy(p->path, dir, sizeof(p->path) - 1);
	memcpy(&p->stat, &st, sizeof(p->stat));

	return 1;
}
