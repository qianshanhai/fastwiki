/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "wiki_zh.h"
#include "wiki_zh_auto.h"
#include "prime.h"
#include "q_util.h"

WikiZh::WikiZh()
{
	m_init_flag = 0;

	memset(m_idx_hans, 0, sizeof(m_idx_hans));
	m_idx_hans_total = 0;

	for (int i = 0; i < 256; i++) {
		m_zh2hans[i] = NULL;
	}
}

WikiZh::~WikiZh()
{
}

int WikiZh::wz_add(SHash **hash, const struct zh_convert *d, int total, int *idx, int *idx_total)
{
	char k[256];
	struct zh_value value;
	SHash *p;

	p = hash[d->n]; 

	if (p == NULL) {
		p = hash[d->n] = new SHash();
		p->sh_set_hash_magic(get_max_prime(total * 100));
		p->sh_init(total + 10, d->n, sizeof(value));
		idx[*idx_total] = d->n;
		(*idx_total)++;
	}

	strcpy(k, d->from);
	strcpy(value.buf, d->to);
	value.len = strlen(d->to);

	int t = ((unsigned char)k[0] << 16) + ((unsigned char)k[1] << 8) + (unsigned char)k[2];
	m_have[t >> 3] |= 1 << (t & 7);

	p->sh_replace(k, &value); 

	return 0;
}

int WikiZh::wz_init_table(SHash **hash, struct zh_convert *zh, int *idx, int *idx_total)
{
	int total[256];
	struct zh_convert *p;

	memset(total, 0, sizeof(total));

	for (int i = 0; zh[i].from; i++) {
		p = &zh[i];
		total[p->n]++;
	}

	for (int i = 0; zh[i].from; i++) {
		p = &zh[i];
		wz_add(hash, p, total[p->n], idx, idx_total);
	}

	return 0;
}

#include "wiki_zh_auto.h"

int WikiZh::wz_init()
{
	const char *data = __ZH_CONVERT_2HANS;
	const char *tmp = data;

	m_have = (unsigned char *)malloc((1 << 21) + 1024);
	memset(m_have, 0, (1 << 21) + 1024);

	data = strchr(data, '\n');
	data++;

	struct zh_convert *zh2hans = (struct zh_convert *)calloc(atoi(tmp) + 10, sizeof(struct zh_convert));
	struct zh_convert *x;

	int total = 0;

	char key[256];
	int pos = 0;
	char *value;

	for (const char *p = data; *p; p++) {
		if (*p == '\t') {
			key[pos++] = 0;
			value = key + pos;
		} else if (*p == '\n') {
			key[pos] = 0;

			x = &zh2hans[total++];
			x->n = strlen(key);
			x->from = (char *)malloc(x->n + 1);
			x->to = (char *)malloc(strlen(value) + 1);
			strcpy(x->from, key);
			strcpy(x->to, value);

			pos = 0;
		} else
			key[pos++] = *p;
	}
		
	zh2hans[total].from = zh2hans[total].to = NULL;

	wz_init_table(m_zh2hans, zh2hans, m_idx_hans, &m_idx_hans_total);

	for (int i = 0; i < total; i++) {
		x = &zh2hans[i];
		if (x->from)
			free(x->from);
		if (x->to)
			free(x->to);
	}

	free(zh2hans);
	m_init_flag = 1;

	return 0;
}

int WikiZh::wz_sys_convert(SHash **hash, const char *buf, int len, int *idx, int idx_total, char *data)
{
	SHash *p;
	int t, pos = 0;
	struct zh_value *f;

	for (int i = 0; i < len; i++) {
		if (buf[i] & 0x80) {
			t = ((unsigned char)buf[i] << 16)
					+ ((unsigned char)buf[i + 1] << 8) + (unsigned char)buf[i + 2];
			if (m_have[t >> 3] & (1 << (t & 7))) {
				for (int j = idx_total - 1; j >= 0; j--) {
					p = hash[idx[j]];
					if (p == NULL)
						continue;
					if (i + idx[j] < len) {
						if (p->sh_find(buf + i, (void **)&f) == _SHASH_FOUND) {
							memcpy(data + pos, f->buf, f->len);
							pos += f->len;
							i += idx[j] - 1;
							goto next;
						}
					}
				}
			}
			data[pos++] = buf[i];
			data[pos++] = buf[i + 1];
			data[pos++] = buf[i + 2];
			i += 2;
		} else {
			data[pos++] = buf[i];
		}
next:
		;
	}

	data[pos] = 0;

	return pos;
}

int WikiZh::wz_convert_2hans(const char *buf, int len, char *ret)
{
	if (m_init_flag == 0)
		return 0;

	return wz_sys_convert(m_zh2hans, buf, len, m_idx_hans, m_idx_hans_total, ret);
}
