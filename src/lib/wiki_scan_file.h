/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_SCAN_FILE_H
#define __WIKI_SCAN_FILE_H

#include <sys/stat.h>
#include "wiki_common.h"

#define MAX_FW_DIR_TOTAL 256

#define WK_DAT_PREFIX "fastwiki.dat."
#define WK_IDX_PREFIX "fastwiki.idx."
#define WK_MATH_PREFIX "fastwiki.math."
#define WK_IMAGE_PREFIX "fastwiki.image."
#define WK_FIDX_PREFIX "fastwiki.fidx."

struct file_st {
	fw_files_t data_file;
	int data_total;

	char index_file[128];
	char math_file[128];

	fw_files_t image_file;
	int image_total;

	fw_files_t fidx_file;
	int fidx_file_total;

	char lang[32];

	int flag; /* =1 valid, =0 invalid */
	struct stat stat;
};

typedef struct {
	char path[128];
	struct stat stat;
} fw_dir_t;

class WikiScanFile {
	public:
		WikiScanFile();
		~WikiScanFile();

	private:
		fw_dir_t m_dir[MAX_FW_DIR_TOTAL];
		int m_dir_total;

	public:
		int wsf_fetch_fw_dir(fw_dir_t *dir, int max_total, char *s_dir[], int s_total);
		int wsf_scan_file(struct file_st *file, int max_total, fw_dir_t *dir, int dir_total);

	private:
		int wsf_scan_sdcard(const char *dir);
		int wsf_scan_dir(const char *dir, struct file_st *file, int *total, int max_total);
		int wsf_scan_lang(const char *dir, const char *fname, struct file_st *file, int *total, int max_total);
		struct file_st *wsf_get_file_st(struct file_st *file, int *total, int max_total, const char *lang);

		int wsf_check_dir(fw_dir_t *d, int total, const char *dir, const struct stat *st);
		int wsf_add_dir(const char *dir);
		int wsf_check_fw_name(const char *name, char *lang);
};	

#endif
