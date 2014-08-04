/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "q_util.h"
#include "s_hash.h"
#include "prime.h"
#include "crc32sum.h"
#include "wiki_index.h"

WikiIndex::WikiIndex()
{
	m_fp = NULL;

	m_wi_head = NULL;
	m_href_idx = NULL;
	m_sort_idx = NULL;
	m_key_string = NULL;
	m_key_store = NULL;
	m_hash = NULL;

	m_tmp_idx = NULL;

	m_search_func = NULL;
	m_search_flag = 0;

	m_fd = -1;

	m_check_data_func = NULL;
}

WikiIndex::~WikiIndex()
{
	if (m_tmp_idx) {
		free(m_tmp_idx);
		m_tmp_idx = NULL;
	}

	if (m_fd >= 0)
		close(m_fd);

	if (m_wi_head)
		free(m_wi_head);

	if (m_hash)
		delete m_hash;
}

#define UPPER(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + 'a' - 'A') : (c))

int my_strcmp(const char *p1, const char *p2, size_t n)
{
	int i;

	for (i = 0; p1[i] && p2[i]; i++) {
		if (UPPER(p1[i]) != UPPER(p2[i]))
			break;
	}
	return (unsigned char)UPPER(p1[i]) - (unsigned char)UPPER(p2[i]);
}

int my_strncmp(const char *p1, const char *p2, size_t n)
{
	int i;

	for (i = 0; i < (int)n && p1[i] && p2[i]; i++) {
		if (UPPER(p1[i]) != UPPER(p2[i]))
			break;
	}

	return i == (int)n ? 0 : (unsigned char)UPPER(p1[i]) - (unsigned char)UPPER(p2[i]);
}

int WikiIndex::wi_set_data_func(int (*func)(int data_file_idx))
{
	m_check_data_func = func;

	return 0;
}

int WikiIndex::wi_set_key_flag(char *key, unsigned char *flag)
{
	int j;

	for (j = 0; key[j] && j < MAX_KEY_LEN; j++) {
		if (key[j] >= 'A' && key[j] <= 'Z') {
			key[j] += 'a' - 'A';
			if (j < 64) {
				flag[j / 8] |= 1 << (j % 8);
			}
		}
	}

	return j;
}

int WikiIndex::wi_unset_key_flag(char *key, const unsigned char *flag)
{
	int i;

	for (i = 0; key[i] && i < MAX_KEY_LEN; i++) {
		if (key[i] >= 'a' && key[i] <= 'z') {
			if (flag[i / 8] & (1 << (i % 8))) {
				key[i] -= 'a' - 'A';
			}
		}
	}

	return i;
}

int WikiIndex::wi_unset_key_flag(const char *old, char *key, const unsigned char *flag)
{
	int i;

	for (i = 0; old[i] && i < MAX_KEY_LEN; i++) {
		key[i] = old[i];
		if (i < MAX_KEY_LEN) {
			if (flag[i / 8] & (1 << (i % 8))) {
				key[i] -= 'a' - 'A';
			}
		}
	}

	key[i] = 0;

	return i;
}

#define WI_TITLE_TMPFILE "fastwiki.tmp.0"

int WikiIndex::wi_output_init()
{
	m_tmp = (char *)malloc(2*1024*1024 + 1024);

	if ((m_fp = fopen(WI_TITLE_TMPFILE, "wb+")) == NULL)
		return -1;

	setvbuf(m_fp, m_tmp, _IOFBF, 2*1024*1024);

	m_key_len = 0;
	m_key_total = 0;

	return 0;
}

int WikiIndex::wi_one_title(const char *title, int len)
{
	fprintf(m_fp, "%s\n", title);

	m_key_len += len + 1;
	m_key_total++;

	return 0;
}

static char *m_key_tmp_string = NULL;

static int _sort_title(const void *p1, const void *p2)
{
	sort_idx_t *x1, *x2;

	x1 = (sort_idx_t *)p1;
	x2 = (sort_idx_t *)p2;

	return strcmp(m_key_tmp_string + x1->key_pos, m_key_tmp_string + x2->key_pos);
}

struct tmp_key {
	unsigned int crc32;
};

struct tmp_value {
	int idx;
};

int WikiIndex::wi_add_title_done()
{
	char buf[1024];

	m_sort_idx = (sort_idx_t *)calloc(m_key_total + 1, sizeof(sort_idx_t));

	memset(m_sort_idx, 0, m_key_total * sizeof(sort_idx_t));

	m_key_len += 8 - m_key_len % 4;

	m_key_string = (char *)malloc(m_key_len + 1024);
	memset(m_key_string, 0, m_key_len);

	fflush(m_fp);
	fseek(m_fp, 0, SEEK_SET);

	m_idx_total = 0;
	sort_idx_t *p;
	int pos = 0;

	while (fgets(buf, sizeof(buf), m_fp)) {
		chomp(buf);
		p = &m_sort_idx[m_idx_total++];
		p->key_len = (unsigned char)strlen(buf);
		p->key_pos = pos;

		memcpy(m_key_string + pos, buf, p->key_len);
		wi_set_key_flag(m_key_string + pos, p->flag);

		pos += p->key_len;

		m_key_string[pos++] = 0;
	}

	fclose(m_fp);

	unlink(WI_TITLE_TMPFILE);

	m_key_tmp_string = m_key_string;
	qsort(m_sort_idx, m_idx_total, sizeof(sort_idx_t), _sort_title);
	m_key_tmp_string = NULL;

	if ((m_fp = fopen(WI_TITLE_TMPFILE, "wb+")) == NULL)
		return -1;

	setvbuf(m_fp, m_tmp, _IOFBF, 2*1024*1024);

	for (int i = 0; i < m_idx_total; i++) {
		p = &m_sort_idx[i];
		wi_unset_key_flag(m_key_string + p->key_pos, p->flag);
		fprintf(m_fp, "%s\n", m_key_string + p->key_pos);
	}
	fflush(m_fp);

	m_title_hash = new SHash();
	m_title_hash->sh_init(m_idx_total, sizeof(struct tmp_key), sizeof(struct tmp_value));

	m_idx_total = 0;
	pos = 0;

	memset(m_key_string, 0, m_key_len);
	memset(m_sort_idx, 0, m_key_total * sizeof(sort_idx_t));

	fseek(m_fp, 0, SEEK_SET);

	while (fgets(buf, sizeof(buf), m_fp)) {
		chomp(buf);
		p = &m_sort_idx[m_idx_total];
		p->key_len = (unsigned char)strlen(buf);
		p->key_pos = pos;

		p->data_len = 0;
		p->data_file_idx = (unsigned char)-1;

		memcpy(m_key_string + pos, buf, p->key_len);
		wi_set_key_flag(m_key_string + pos, p->flag);

		wi_add_key_to_hash(m_key_string + pos, p->key_len, m_idx_total, p->flag);
		m_idx_total++;

		pos += p->key_len;

		m_key_string[pos++] = 0;
	}

	fclose(m_fp);
	unlink(WI_TITLE_TMPFILE);

	return 0;
}

int WikiIndex::wi_add_key_to_hash(const char *title, int len, int idx, unsigned char flag[8])
{
	struct tmp_key key;
	struct tmp_value value;

	key.crc32 = crc32sum(title, len);
	value.idx = idx;

	m_title_hash->sh_add(&key, &value);

	return 0;
}

int WikiIndex::wi_title_find(const char *title, int len)
{
	int total;
	struct tmp_key key;
	struct tmp_value *f, *v[64];
	sort_idx_t *p;
	
	unsigned char flag[64];
	char buf[1024];

	if (len <= 0 || len >= (int)sizeof(buf))
		return -1;

	memcpy(buf, title, len);
	buf[len] = 0;

	memset(flag, 0, sizeof(flag));
	wi_set_key_flag(buf, flag);

	key.crc32 = crc32sum(buf, len);

	total = 0;
	m_title_hash->sh_begin();

	while (m_title_hash->sh_next(&key, (void **)&f) == _SHASH_FOUND) {
		p = &m_sort_idx[f->idx];
		if (strcmp(m_key_string + p->key_pos, buf) == 0) {
			v[total++] = f;
			if (memcmp(p->flag, flag, 8) == 0)
				return f->idx;
		}
	}

	if (total == 0)
		return -1;

	return v[0]->idx;
}


int WikiIndex::wi_add_one_page(const char *title, int len, const char *redirect, int r_len,
		unsigned char data_file_idx, unsigned short data_len, unsigned int data_pos)
{
	int t_idx = -1, rd_idx = -1;
	sort_idx_t *p;

	if (len == r_len) {
		if (strncasecmp(title, redirect, len) == 0)
			return 0;
	}

	if (r_len > 0) {
		if ((rd_idx = wi_title_find(redirect, r_len)) >= 0) {
			if ((t_idx = wi_title_find(title, len)) >= 0) {
				m_sort_idx[t_idx].data_pos = rd_idx;
				m_sort_idx[t_idx].data_len = (unsigned short)-1;
			}
		}
	} else {
		if ((t_idx = wi_title_find(title, len)) >= 0) {
			p = &m_sort_idx[t_idx];
			p->data_file_idx = data_file_idx;
			p->data_len = data_len;
			p->data_pos = data_pos;
		}
	}

	return 0;
}

int WikiIndex::wi_set_head(wi_head_t *h, int idx_total, int key_len, int key_total,
		int store_len, int hash_len)
{
	h->flag = 0xfd;
	strcpy(h->name, "fastwiki");

	h->version = 0x1;

	h->href_idx_total = idx_total;
	h->href_idx_size = idx_total * sizeof(int);
	h->sort_idx_total = idx_total;
	h->sort_idx_size = idx_total * sizeof(sort_idx_t);
	h->key_string_size = key_len; 

	h->key_store_total = key_total;
	h->key_store_size = store_len;
	h->hash_size = hash_len;
	h->file_size = sizeof(wi_head_t) + h->href_idx_size + h->sort_idx_size
			+ h->key_string_size + h->key_store_size + h->hash_size;

	return 0;
}

int WikiIndex::wi_output(const char *outfile)
{
	int *href_idx = (int *)calloc(m_idx_total, sizeof(int));

	if (href_idx == NULL) {
		printf("No enough memory: m_idx_total=%d\n", m_idx_total);
		return -1;
	}

	memset(href_idx, 0, m_idx_total * sizeof(int));

	for (int i = 0; i < m_idx_total; i++) {
		sort_idx_t *p = &m_sort_idx[i];
		href_idx[i] = i;
		if (p->data_len == (unsigned short)-1) {
			sort_idx_t *t = &m_sort_idx[p->data_pos];
			p->data_file_idx = t->data_file_idx;
			p->data_len = t->data_len;
			p->data_pos = t->data_pos;
		}
	}

	m_wi_head = (wi_head_t *)malloc(sizeof(wi_head_t));;
	memset(m_wi_head, 0, sizeof(wi_head_t));

	/* This value used in wi_create_vhash() */
	m_wi_head->sort_idx_total = m_idx_total;

	char *store, *hash;
	int key_total = 0, store_len = 0, hash_len = 0;

	if (wi_create_vhash(m_sort_idx, m_idx_total, m_key_string, (char **)&store, &store_len,
				&key_total, (char **)&hash, &hash_len) == -1)
		return -1;

	wi_set_head(m_wi_head, m_idx_total, m_key_len, key_total, store_len, hash_len);
	wi_write_index_file(outfile, m_wi_head, href_idx, m_sort_idx, m_key_string, store, hash);

	return 0;
}

int WikiIndex::wi_write_index_file(const char *outfile, const wi_head_t *h, const int *href_idx,
			sort_idx_t *sort_idx, const char *key_string, const char *store, const char *hash)
{

	int fd, flag = O_CREAT | O_TRUNC | O_RDWR | O_APPEND | O_BINARY;

	if ((fd = open(outfile, flag, 0644)) == -1)
		return -1;

	write(fd, h, sizeof(wi_head_t));
	write(fd, href_idx, h->href_idx_size);
	write(fd, sort_idx, h->sort_idx_size);
	write(fd, key_string, h->key_string_size);
	write(fd, store, h->key_store_size);
	write(fd, hash, h->hash_size);

	close(fd);

	return 0;
}

/*
 * note: sizeof(ret) == 4 !!
 */
int WikiIndex::wi_split_key(const char *key, int k_len, char ret[KEY_SPLIT_LEN][MAX_KEY_LEN], int *len)
{
	char tmp[1024];
	int pos = 0;

	*len = 0;

	for (int i = 0; i < k_len; i++) {
		if (key[i] & 0x80) {
			if (pos > 0) {
				if (*len >= KEY_SPLIT_LEN - 1)
					return 0;
				strncpy(ret[*len], tmp, pos);
				(*len)++;
				pos = 0;
			}
			if (*len >= KEY_SPLIT_LEN - 1)
				return 0;
			strncpy(ret[*len], key + i, 3);
			(*len)++;
			i += 2;
		} else if (key[i] == ' ' || key[i] == '.'
				|| key[i] == ',' || key[i] == '('
				|| key[i] == ')' || key[i] == '&'
				|| key[i] == '<' || key[i] == '>'
				|| key[i] == '-' || key[i] == '+') {
			if (pos > 0) {
				if (*len >= KEY_SPLIT_LEN - 1)
					return 0;
				strncpy(ret[*len], tmp, pos);
				(*len)++;
				pos = 0;
			}
		} else {
			if (pos >= (int)sizeof(tmp) - 1)
				break;
			tmp[pos++] = key[i];
		}
	}

	if (pos > 0) {
		if (*len >= KEY_SPLIT_LEN - 1)
			return 0;
		strncpy(ret[*len], tmp, pos);
		(*len)++;
	}

	return 0;
}

int WikiIndex::wi_create_vhash(sort_idx_t *sort_idx, int total, char *key_string,
		char **_store, int *_store_len, int *key_total, char **h, int *h_len)
{
	int len;
	char key[KEY_SPLIT_LEN][MAX_KEY_LEN];
	SHash *T;

	struct key_store k;
	struct {
		int total;
	} v, *f;

	T = new SHash();
	T->sh_set_hash_magic(get_max_prime(50*10000));
	if (T->sh_init(25*10000, sizeof(struct key_store), sizeof(v)) == -1) {
		printf("No enough memory: %d\n", __LINE__);
		return -1;
	}

	for (int i = 0; i < total; i++) {
		sort_idx_t *p = &sort_idx[i];
		memset(key, 0, sizeof(key));
		wi_split_key(key_string + p->key_pos, (int)p->key_len, key, &len);
		for (int j = 0; j < len; j++) {
			memset(&k, 0, sizeof(k));
			strncpy(k.key, key[j], sizeof(k.key) - 1);
			q_tolower(k.key);
			if (T->sh_find(&k, (void **)&f) == _SHASH_FOUND) {
				f->total++;
			} else {
				v.total = 1;
				T->sh_add(&k, &v);
			}
		}
	}

	int size = 0;
	*key_total = 0;
	T->sh_reset();
	while (T->sh_read_next(&k, (void *)&v) == _SHASH_FOUND) {
		size += sizeof(struct key_store_head) + v.total * sizeof(int);
		(*key_total)++;
	}

	int store_pos = 0;
	char *store = (char *)malloc(size + 10*1024*1024);
	if (store == NULL) {
		printf("No enough memory: size=%d\n", size);
		return -1;
	}

	memset(store, 0, size);

	m_hash = new SHash();
	m_hash->sh_set_hash_magic(get_max_prime(*key_total + 20*10000));
	if (m_hash->sh_init(*key_total + 10, sizeof(struct key_store), sizeof(struct hash_v)) == -1) {
		printf("No enough memory: %d\n", __LINE__);
		return -1;
	}

	struct hash_v *find, value;
	struct key_store_head *head;

	for (int i = 0; i < total; i++) {
		sort_idx_t *p = &sort_idx[i];
		memset(key, 0, sizeof(key));
		wi_split_key(key_string + p->key_pos, (int)p->key_len, key, &len);

		if (len > 1 && (key[0][0] & 0x80 ) && (key[1][0] & 0x80)) {
			memset(&k, 0, sizeof(k));
			snprintf(k.key, sizeof(k.key), "%s%s", key[0], key[1]);
			if (m_hash->sh_find(&k, (void **)&find) != _SHASH_FOUND) {
				memset(&value, 0, sizeof(value));
				m_hash->sh_add(&k, &value);
			}
		}

		for (int j = 0; j < len; j++) {
			memset(&k, 0, sizeof(k));
			strncpy(k.key, key[j], sizeof(k.key) - 1);
			q_tolower(k.key);
			if (m_hash->sh_find(&k, (void **)&find) == _SHASH_FOUND) {
				head = (struct key_store_head *)(store + find->pos);
				if (head->value[head->total - 1] < i) {
					head->value[head->total++] = i;
				}
			} else {
				if (T->sh_find(&k, (void **)&f) != _SHASH_FOUND) {
					printf("impossible error i = %d\n", i);
					return -1;
				}
				memset(&value, 0, sizeof(value));
				value.pos = store_pos;
				m_hash->sh_add(&k, &value);

				memcpy(store + store_pos, &k, sizeof(k));
				head = (struct key_store_head *)(store + store_pos);
				head->value[head->total++] = i;

				store_pos += sizeof(struct key_store) + (f->total + 1) * sizeof(int);
			}
		}
	}

	m_search_func = &WikiIndex::wi_sys_search;
	m_search_flag = 1;

	sort_idx_t tmp_idx;
	int tmp_total;

	m_hash->sh_reset();
	while (m_hash->sh_read_next(&k, (void **)&find) == _SHASH_FOUND) {
		if (wi_search(k.key, WK_LIKE_FIND, &tmp_idx, &tmp_total) == 0) {
			find->start_idx = tmp_idx.key_pos;
			find->end_idx = tmp_idx.key_pos + tmp_total - 1;
		}
	}
	
	*_store = store;
	*_store_len = size;
	m_hash->sh_get_addr((void **)h, h_len);

	return 0;
}

void print_sort_idx(sort_idx_t *p)
{
	printf("key_pos = %d, key_len = %d, data_len=%d, data_pos = %d\n",
			p->key_pos, (int)p->key_len, p->data_len, p->data_pos);
}

static void print_head(wi_head_t *p)
{
	printf("href_idx_total=%d, href_idx_size=%d\n", p->href_idx_total, p->href_idx_size);
	printf("sort_idx_total=%d, sort_idx_size=%d\n", p->sort_idx_total, p->sort_idx_size);
	printf("key_string_size=%d\n", p->key_string_size);
	printf("key_store_total=%d, key_store_size=%d\n", p->key_store_total, p->key_store_size);
	printf("hash_size=%d\n", p->hash_size);

}

inline int WikiIndex::wi_get_href_idx(int idx)
{
	int tmp;

	if (m_search_flag == 1)
		return m_href_idx[idx];

	lseek(m_fd, sizeof(wi_head_t) + idx * sizeof(int), SEEK_SET);
	read(m_fd, &tmp, sizeof(tmp));

	return tmp;
}

inline sort_idx_t *WikiIndex::wi_get_sort_idx(int idx)
{
	if (m_search_flag == 1)
		return &m_sort_idx[idx];

	memset(&m_curr_sort_idx, 0, sizeof(sort_idx_t));

	lseek(m_fd, sizeof(wi_head_t) + m_wi_head->href_idx_size + idx * sizeof(sort_idx_t), SEEK_SET);
	read(m_fd, &m_curr_sort_idx, sizeof(sort_idx_t));

	return &m_curr_sort_idx;
}

inline char *WikiIndex::wi_get_key_string(const sort_idx_t *idx)
{
	if (m_search_flag == 1)
		return m_key_string + idx->key_pos;

	lseek(m_fd, sizeof(wi_head_t) + m_wi_head->href_idx_size
					+ m_wi_head->sort_idx_size + idx->key_pos, SEEK_SET);
	read(m_fd, m_curr_key, (int)idx->key_len);
	m_curr_key[idx->key_len] = 0;

	return m_curr_key;
}

sort_idx_t *WikiIndex::wi_random_key(sort_idx_t *idx)
{
	unsigned int random = rand() % m_wi_head->sort_idx_total;

	memcpy(idx, wi_get_sort_idx(random), sizeof(sort_idx_t));

	return idx;
}

int WikiIndex::wi_init(const char *file)
{
	m_search_func = &WikiIndex::wi_user_search;
	m_search_flag = 0;

	m_wi_head = (wi_head_t *)malloc(sizeof(wi_head_t));

	if ((m_fd = open(file, O_RDONLY | O_BINARY)) == -1)
		return -1;

	lseek(m_fd, 0, SEEK_SET);
	read(m_fd, m_wi_head, sizeof(wi_head_t));

#ifdef DEBUG
	wi_stat();
#endif

#if 0
	lseek(m_fd, sizeof(wi_head_t) + m_wi_head->href_idx_size, SEEK_SET);
	char *tmp = (char *)malloc(m_wi_head->sort_idx_size + 1024);
	read(m_fd, tmp, m_wi_head->sort_idx_size);

	sort_idx_t *s = (sort_idx_t *)tmp;
	for (int i = 0; i < m_wi_head->sort_idx_total; i++) {
		printf("idx=%d, pos=%u, len=%d\n", s[i].data_file_idx, s[i].data_pos, (int)s[i].data_len);
	}
#endif

	unsigned int hash_pos = sizeof(wi_head_t) + m_wi_head->href_idx_size 
					+ m_wi_head->sort_idx_size + m_wi_head->key_string_size
					+ m_wi_head->key_store_size;

	m_hash = new SHash();
	m_hash->sh_fd_init_ro(m_fd, hash_pos);

	m_tmp_idx = (int *)calloc(MAX_TMP_IDX_TOTAL, sizeof(int));
	if (m_tmp_idx == NULL) {
		return -1;
	}

	return 0;
}

#define G(idx) wi_get_key_string(wi_get_sort_idx(idx))

int WikiIndex::wi_user_search(int start, int i, const char *key, 
		int key_len, sort_idx_t *ret_idx, int *total)
{
	sort_idx_t *p = wi_get_sort_idx(i);

	if (*total >= MAX_FIND_RECORD || my_strncmp(wi_get_key_string(p), key, key_len) != 0)
		return 1;

	if (m_check_data_func != NULL) {
		if (m_check_data_func((int)p->data_file_idx) == -1)
			return 0;
	}

	memcpy(&ret_idx[*total], p, sizeof(sort_idx_t));
	(*total)++;

	return 0;
}

int WikiIndex::wi_sys_search(int start, int i, const char *key,
		int key_len, sort_idx_t *ret_idx, int *total)
{
	if (my_strncmp(G(i), key, key_len) != 0) {
		*total = i - start;
		ret_idx->key_pos = start;
		return 1;
	} else if (i == m_wi_head->sort_idx_total - 1) {
		*total = i - start + 1;
		ret_idx->key_pos = start;
		return 1;
	}

	return 0;
}

int WikiIndex::wi_search2(const char *key, int key_len, int flag, int idx, sort_idx_t *ret_idx, int *total)
{
	*total = 0;

	if (flag == WK_KEY_FIND) {
		memcpy(ret_idx, wi_get_sort_idx(idx), sizeof(sort_idx_t));
		*total = 1;
		return 0;
	}

	for (; idx >= 0; idx--) {
		if (my_strncmp(G(idx), key, key_len) != 0) {
			idx++;
			break;
		}
	}

	if (idx < 0)
		idx = 0;

	for (int i = idx; i < m_wi_head->sort_idx_total; i++) {
		if ((this->*m_search_func)(idx, i, key, key_len, ret_idx, total) == 1)
			break;
	}

	return 0;
}

int WikiIndex::wi_check_search(const char *key, int *low, int *high)
{
	int len, flag;
	char tmp[KEY_SPLIT_LEN][MAX_KEY_LEN];
	struct key_store buf[2];
	struct hash_v find;

	memset(tmp, 0, sizeof(tmp));
	wi_split_key(key, strlen(key), tmp, &len);
	memset(&buf, 0, sizeof(buf));

	strncpy(buf[0].key, tmp[0], sizeof(buf[0].key) - 1);

	flag = len > 1 && (tmp[0][0] & 0x80) && (tmp[1][0] & 0x80);
	if (flag) {
		snprintf(buf[1].key, sizeof(buf[1].key), "%s%s", tmp[0], tmp[1]);
	}

	for (int i = flag ? 1 : 0; i >= 0; i--) { 
		q_tolower(buf[i].key);
		if (m_hash->sh_fd_find(&buf[i], &find) == _SHASH_FOUND) {
			*low = find.start_idx;
			*high = find.end_idx;
			return 0;
		}
	}

	return -1;
}

/*
 * flag:
 *   WK_KEY_FIND  : find a key
 *   WK_LIKE_FIND : same start key
 */
int WikiIndex::wi_search(const char *key, int flag, sort_idx_t *ret_idx, int *total)
{
	int (*cmp)(const char *p1, const char *p2, size_t n);
	int n, low = 0, high = m_wi_head->sort_idx_total - 1, mid;
	int key_len = strlen(key);

	if (m_search_flag == 1) {
		low = 0;
		high = m_wi_head->sort_idx_total - 1;
	} else {
		if (wi_check_search(key, &low, &high) == -1) {
			low = 0;
			high = m_wi_head->sort_idx_total - 1;
		}
	}
	if (flag == WK_KEY_FIND)
		cmp = my_strcmp;
	else
		cmp = my_strncmp;

	if (cmp(G(low), key, key_len) == 0) {
		wi_search2(key, key_len, flag, low, ret_idx, total);
		return 0;
	}

	if (cmp(G(high), key, key_len) == 0) {
		wi_search2(key, key_len, flag, high, ret_idx, total);
		return 0;
	}

	while (low <= high) {
		mid = (low + high) / 2;
		n = cmp(G(mid), key, key_len);
		if (n == 0) {
			wi_search2(key, key_len, flag, mid, ret_idx, total);
			return 0;
		} else if (n > 0) {
			high = mid - 1;
		} else {
			low = mid + 1;
		}

		if (low > high) {
			break;
		}
	}

	return -1;
}

int WikiIndex::wi_binary_find(const int *to, int total, int v)
{
	int low = 0, high = total - 1, mid;

	if (to[low] == v || to[high] == v)
		return 0;

	while (low <= high) {
		mid = (low + high) / 2;
		if (to[mid] == v)
			return 0;
		else if (to[mid] > v)
			high = mid - 1;
		else 
			low = mid + 1;

		if (low > high)
			return -1;
	}

	return -1;
}

int WikiIndex::wi_cmp_two_idx(int *f, int f_total, const int *to, int to_total)
{
	int r = 0;

	for (int i = 0; i < f_total; i++) {
		if (f[i] >= 0) {
			if (wi_binary_find(to, to_total, f[i]) == -1) {
				f[i] = -1;
			} else {
				r++;
			}
		}
	}
	
	return r;
}

struct tmp {
	int *v;
	int total;
};

static int _sort_v(const void *p1, const void *p2)
{
	struct tmp *v1, *v2;

	v1 = (struct tmp *)p1;
	v2 = (struct tmp *)p2;

	return v1->total - v2->total;
}

int WikiIndex::wi_match(const char *key, sort_idx_t *idx, int *idx_len)
{
	struct key_store k;
	struct hash_v find;
	struct tmp v[128];
	int ret = 0, total = 0;
	char split_key[KEY_SPLIT_LEN][MAX_KEY_LEN];
	int split_len;

	memset(v, 0, sizeof(v));

	memset(split_key, 0, sizeof(split_key));
	wi_split_key(key, strlen(key), split_key, &split_len);

	for (int i = 0; i < split_len; i++) {
		if (split_key[i][0] == 0)
			continue;
		memset(&k, 0, sizeof(k));
		strncpy(k.key, split_key[i], sizeof(k.key) - 1);
		q_tolower(k.key);
		if (m_hash->sh_fd_find(&k, &find) != _SHASH_FOUND) {
			ret = -1;
			goto out;
		}

		struct tmp *p = &v[i];

		lseek(m_fd, sizeof(wi_head_t) + m_wi_head->href_idx_size
					+ m_wi_head->sort_idx_size + m_wi_head->key_string_size
					+ find.pos + sizeof(struct key_store), SEEK_SET);

		read(m_fd, &(p->total), sizeof(p->total));

		if (p->total > MAX_TMP_IDX_TOTAL) {
			ret = -1;
			goto out;
		}

		p->v = (int *)calloc(p->total + 1, sizeof(int));
		read(m_fd, p->v, p->total * sizeof(int));

		total++;
	}

	if (total == 1) {
		*idx_len = 0;
		for (int i = 0; i < v[0].total && *idx_len < MAX_FIND_RECORD; i++) {
			memcpy(&idx[*idx_len], wi_get_sort_idx(v[0].v[i]), sizeof(sort_idx_t));
			(*idx_len)++;
		}
		goto out;
	}

	qsort(v, total, sizeof(struct tmp), _sort_v);

	if (v[0].total >= MAX_TMP_IDX_TOTAL) {
		ret = -1;
		goto out;
	}

	for (int i = 0; i < v[0].total; i++)
		m_tmp_idx[i] = v[0].v[i];

	for (int i = 1; i < total; i++) {
		if (wi_cmp_two_idx(m_tmp_idx, v[0].total, v[i].v, v[i].total) == -1) {
			ret = -1;
			goto out;
		}
	}

	*idx_len = 0;
	sort_idx_t *p;

	for (int i = 0; i < v[0].total; i++) {
		if (*idx_len >= MAX_FIND_RECORD)
			break;
		if (m_tmp_idx[i] >= 0) {
			p = wi_get_sort_idx(m_tmp_idx[i]);
			if (m_check_data_func == NULL ||  m_check_data_func(p->data_file_idx) == 0) {
				memcpy(&idx[*idx_len], p, sizeof(sort_idx_t));
				(*idx_len)++;
			}
		}
	}

out:
	for (int i = 0; i < split_len; i++) {
		if (v[i].v)
			free(v[i].v);
	}

	return ret;
}

int WikiIndex::wi_get_key_href(int href_idx, char *key)
{
	if (href_idx <= 0 || href_idx >= m_wi_head->href_idx_total)
		return -1;

	if (wi_get_href_idx(href_idx) == -1)
		return -1;

	return wi_get_key(wi_get_sort_idx(wi_get_href_idx(href_idx)), key);
}

int WikiIndex::wi_get_key(const sort_idx_t *idx, char *key)
{
	const char *tmp = wi_get_key_string(idx);

	strcpy(key, tmp);

	return wi_unset_key_flag(key, idx->flag);
}

/*
 * flag: 
 *  WK_INDEX_FIND   index
 *  WK_KEY_FIND     find key
 *  WK_MATCH_FIND   match key, split by space
 *  WK_LIKE_FIND    like start with key
 */ 
int WikiIndex::wi_find(const char *key, int flag, int index, sort_idx_t *idx, int *idx_len)
{
	int tmp;

	*idx_len = 0;

	if (flag == WK_INDEX_FIND) {
		if (index >= 0 && index < m_wi_head->href_idx_total) {
			if (m_wi_head->version == 0) {
				if ((tmp = wi_get_href_idx(index)) == -1) {
#ifdef DEBUG
					printf("idx=%d\n", tmp);
#endif
					return -1;
				}
			} else {
				tmp = index;
			}
			memcpy(idx, wi_get_sort_idx(tmp), sizeof(sort_idx_t));
			*idx_len = 1;
			return 0;
		}
		return -1;
	} else if (flag == WK_KEY_FIND || flag == WK_LIKE_FIND)
		return wi_search(key, flag, idx, idx_len);
	else if (flag == WK_MATCH_FIND)
		return wi_match(key, idx, idx_len);

	return -1;
}

int WikiIndex::wi_stat()
{
	wi_head_t *h = m_wi_head;

	printf("href_idx_total = %d, href_idx_size = %d\n"
			"sort_idx_total = %d, sort_idx_size = %d\n"
			"key_string_size = %d\n"
			"key_store_total = %d, key_store_size = %d\n"
			"hash_size = %d\n",
			h->href_idx_total, h->href_idx_size, h->sort_idx_total, h->sort_idx_size,
			h->key_string_size, h->key_store_total, h->key_store_size, h->hash_size);

#if 0	
	int error = 0;

	for (int i = 0; i < m_wi_head->href_idx_total; i++) {
		if (wi_get_href_idx(i) == -1)
			error++;
	}

	printf("all=%d, error=%d\n", m_wi_head->href_idx_total, error);
#endif

	return 0;
}

