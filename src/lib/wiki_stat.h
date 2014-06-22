/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_STAT_H
#define __WIKI_STAT_H

#include <dirent.h>

#include "q_util.h"
#include "crc32sum.h"
#include "s_hash.h"

#include "file_io.h"
#include "wiki_common.h"

#define MAX_STAT_BUF_SIZE (2*1024*1024)

struct dup_key {
	char fname[64];
};

struct dup_value {
	int value;
};

struct key {
	unsigned short total;
};

class WikiStat { 
	private:
		SHash *m_check_dup;

		char m_output[256];
		char m_indir[256];
		char m_check_dup_file[256];
		
		int m_debug;

		mapfile_t m_mmap;
		unsigned int m_hash;

		char *m_buf; /* FileIO buf */

	public:
		WikiStat();
		~WikiStat();

		int st_start();
		int st_init(int flag = 1);
		int st_check_env();
		int st_convert_title(const char *title, char *buf);
		int st_convert_title2(char *buf);

		int st_check_title(char *buf);
		int st_add_hash(const char *buf, int len, int total);
		int st_one_record(const char *title, int total);
		int st_onefile(const char *file);
		int st_onedir(const char *dir);
		int st_check_dup(const char *fname);
		int st_print(int total);
		int st_create_file();

		int st_convert_file(const char *file, const char *output);

		int st_find_init(const char *file);
		int st_stat_find(const char *title);
};

#endif
