/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_ZH_CONVERT_H
#define __WIKI_ZH_CONVERT_H

#include "s_hash.h"

struct zh_value {
	char buf[128];
	int len;
};

struct zh_convert {
	char *from;
	char *to;
	int n;
};

class WikiZh {
	private:
		SHash *m_zh2hans[256];  /* t 2 s */
		int m_idx_hans[256];
		int m_idx_hans_total;
		unsigned char *m_have;

		int m_init_flag;

	public:
		WikiZh();
		~WikiZh();

		int wz_init();
		int wz_convert_2hans(const char *buf, int len, char *ret);

	private:
		int wz_init_table(SHash **hash, struct zh_convert *zh, int *idx, int *idx_total);
		int wz_add(SHash **hash, const struct zh_convert *d, int total, int *idx, int *idx_total);
		int wz_sys_convert(SHash **hash, const char *buf, int len, int *idx, int idx_total, char *data);
};

#endif
