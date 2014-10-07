/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "q_log.h"
#include "q_util.h"
#include "prime.h"

#include "wiki_full_index.h"

#define _wfi_page_index_size() (m_index_total * sizeof(int))
#define _wfi_hash_size()  ({void *addr; int size; m_hash->sh_get_addr(&addr, &size); size; })
#define _wfi_value_size() ((m_word_total) * sizeof(idx_value_t))
#define _wfi_reverse_idx_size() ((m_word_total) * sizeof(int))
#define _wfi_word_length() (m_word_str_len)

#define _wfi_head_size() (_wfi_page_index_size() + _wfi_hash_size() + _wfi_value_size() + _wfi_reverse_idx_size() + _wfi_word_length())


WikiFullIndex::WikiFullIndex()
{
	m_ro_init_flag = 0;
}

WikiFullIndex::~WikiFullIndex()
{
}

int WikiFullIndex::wfi_init(const fw_files_t file, int total)
{
	int fd;

	m_ro_init_flag = 0;

	for (int i = 0; i < MAX_FD_TOTAL; i++)
		m_fd[i] = -1;

	for (int i = 0; i < total; i++) {
		if ((fd = open(file[i], O_RDONLY | O_BINARY)) == -1) {
			LOG("open fidx file %s error: %s\n", file[i], strerror(errno));
			return -1;
		}
		read(fd, &m_head, sizeof(m_head));
		m_fd[m_head.file_idx] = fd;
	}

	if (m_fd[0] == -1) {
		LOG("wfi_init error\n");
		return -1;
	}

	lseek(m_fd[0], 0, SEEK_SET);
	read(m_fd[0], &m_head, sizeof(m_head));

	m_comp_buf_len = m_head.bitmap_size + 1024;

	m_comp_buf = (char *)malloc(m_comp_buf_len);
	m_bitmap = (unsigned char *)malloc(m_comp_buf_len);
	m_tmp_buf = (unsigned char *)malloc(m_comp_buf_len);

	m_decompress_func = get_decompress_func(m_head.z_flag); 

	m_hash = new SHash();
	m_hash->sh_fd_init_ro(m_fd[0], m_head.hash_pos);

	m_ro_init_flag = 1;

	return 0;
}

inline int WikiFullIndex::wfi_reverse_idx(int idx)
{
	int v = 0;

	lseek(m_fd[0], m_head.reverse_index_pos + idx * sizeof(int), SEEK_SET);
	read(m_fd[0], &v, sizeof(v));

	return v;
}

inline int WikiFullIndex::wfi_get_value(int idx, idx_value_t *v)
{
	memset(v, 0, sizeof(*v));

	lseek(m_fd[0], m_head.value_pos + idx * sizeof(idx_value_t), SEEK_SET);
	read(m_fd[0], v, sizeof(*v));

	return 0;
}

int WikiFullIndex::wfi_init_is_done()
{
	return m_ro_init_flag;
}

#define _wfi_count_total_bit(x, _ct) \
	do { \
		if (p[i] & (1 << x)) { \
			(*_ct)++; \
		} \
	} while (0)

#define _wfi_count_total(_ct) \
	do { \
		_wfi_count_total_bit(0, _ct); \
		_wfi_count_total_bit(1, _ct); \
		_wfi_count_total_bit(2, _ct); \
		_wfi_count_total_bit(3, _ct); \
		_wfi_count_total_bit(4, _ct); \
		_wfi_count_total_bit(5, _ct); \
		_wfi_count_total_bit(6, _ct); \
		_wfi_count_total_bit(7, _ct); \
	} while (0)

#define _wfi_check_bit(x) \
	do { \
		if (p[i] & (1 << x)) { \
			page_idx[real_total++] = wfi_convert_index(find_idx * 64 + i * 8 + x); \
			if (real_total >= max_total) { \
				return real_total; \
			} \
		} \
	} while (0)

#define _wfi_check_char() \
	do { \
		_wfi_check_bit(0); \
		_wfi_check_bit(1); \
		_wfi_check_bit(2); \
		_wfi_check_bit(3); \
		_wfi_check_bit(4); \
		_wfi_check_bit(5); \
		_wfi_check_bit(6); \
		_wfi_check_bit(7); \
	} while (0)

/*
 * string format:
 * 1. "a"   : find all include "a" articles
 * 2. "a b" : find all include "a" and include "b" articles.
 *            use space to split many words, the word have "and" relation.
 * 3. "a*"  : use '*' to match thoese word that start this word
 * 4. "a,b" : use ',' or '|' to match 'or', "a,b" means 'a' or 'b'
 * 5. "a$"  : use '$' to match those words that end with a.
 * 6. "a*b" : match the words that start 'a', and include 'b' after 'a'
 * 7. "a*b*c$" :  
 * 8. "!a"  : match not include "a"
 * 9. "a !b": match include "a" but not include "b" 
 */
int WikiFullIndex::wfi_find(const char *string, int *page_idx, int max_total, int *count_total)
{
	split_t sp;
	struct word_key key;
	struct word_value value;
	int once = 0, find_flag, real_total = 0;

	unsigned long long *bitmap = (unsigned long long *)m_bitmap;
	unsigned long long *comp_buf = (unsigned long long *)m_comp_buf;

	if (count_total)
		*count_total = 0;

	if (m_ro_init_flag == 0 || page_idx == NULL)
		return 0;

	split(' ', string, sp);

	for_each_split(sp, T) {
		if (T[0] == 0)
			continue;

		memset(&key, 0, sizeof(key));
		strncpy(key.word, T, sizeof(key.word) - 1);
		q_tolower(key.word);

		if (m_hash->sh_fd_find(&key, &value) != _SHASH_FOUND)
			return 0;

		idx_value_t v;
		wfi_get_value(value.pos, &v);

		lseek(m_fd[v.file_idx], v.pos, SEEK_SET);
		read(m_fd[v.file_idx], m_tmp_buf, v.len);

		if (v.text_flag == 1) {
			int *p = (int *)m_tmp_buf;
			int total = *p++;
			unsigned char *buf = (unsigned char *)m_comp_buf;

			memset(m_comp_buf, 0, m_head.bitmap_size);

			for (int i = 0; i < total; i++) {
				buf[p[i] / 8] |= 1 << (p[i] % 8);
			}
		} else {
			int len = m_decompress_func(m_comp_buf, m_comp_buf_len, (char *)m_tmp_buf, v.len);
			if (len != m_head.bitmap_size) {
				LOG("fastwiki fdix file maybe damage\n");
				return -1;
			}
		}

		if (once == 0) {
			once = 1;
			memcpy(m_bitmap, m_comp_buf, m_head.bitmap_size);
			continue;
		}

		find_flag = 0;

		for (int j = 0; j < m_head.bitmap_size / 8; j++) {
			bitmap[j] &= comp_buf[j];
			if (bitmap[j]) {
				find_flag = 1;
			}
		}

		if (find_flag == 0)
			return 0;
	}

	/* TODO should be for() once */

	unsigned char *p;

	if (count_total != NULL) {
		*count_total = 0;
		for (int find_idx = 0; find_idx < m_head.bitmap_size / 8; find_idx++) {
			if (bitmap[find_idx]) {
				p = (unsigned char *)&bitmap[find_idx];
				for (int i = 0; i < 8; i++) {
					if (p[i]) {
						_wfi_count_total(count_total);
					}
				}
			}
		}
	}

	for (int find_idx = 0; find_idx < m_head.bitmap_size / 8; find_idx++) {
		if (bitmap[find_idx]) {
			p = (unsigned char *)&bitmap[find_idx];
			for (int i = 0; i < 8; i++) {
				if (p[i]) {
					_wfi_check_char();
				}
			}
		}
	}

	return real_total;
}

inline int WikiFullIndex::wfi_convert_index(int idx)
{
	int tmp = 0;

	if (m_head.page_index_flag == 1)
		return idx;

	lseek(m_fd[0], sizeof(struct fidx_head) + idx * sizeof(int), SEEK_SET);
	read(m_fd[0], &tmp, sizeof(int));

	return tmp;
}

int WikiFullIndex::wfi_create_init(const char *z_flag, int index_total, const char *lang,
		const char *tmp_dir, size_t mem_size, SHash *word_hash, int pthread_total)
{
	m_curr_flush_fd_idx = -1;

	m_word_hash = word_hash;

	m_index_total = index_total;
	m_bitmap_size = index_total / 8;

	if (m_bitmap_size % 8)
		m_bitmap_size += 8 - m_bitmap_size % 8;

	m_comp_buf_len = m_bitmap_size + 512*1024;

	m_bitmap = (unsigned char *)malloc(m_bitmap_size);
	m_tmp_buf = (unsigned char *)malloc(m_comp_buf_len);
	m_comp_buf = (char *)malloc(m_comp_buf_len);

	m_page_index_pos = 0;
	m_page_index = (int *)malloc(_wfi_page_index_size());
	memset(m_page_index, 0, _wfi_page_index_size());

	strcpy(m_lang, lang);
	strcpy(m_tmp_dir, tmp_dir);

	m_pthread_total = pthread_total;
	pthread_mutex_init(&m_mutex, NULL);

	wfi_init_tmp_mem(mem_size);

	m_compress_func = get_compress_func(&m_z_flag, z_flag);
	m_decompress_func = get_decompress_func(m_z_flag);

	if (m_z_flag == -1)
		return -1;

	wfi_count_min_compress_size();

	return 0;
}

int WikiFullIndex::wfi_count_min_compress_size()
{
	memset(m_bitmap, 0, m_bitmap_size);

	int n = m_compress_func(m_comp_buf, m_comp_buf_len, (char *)m_bitmap, m_bitmap_size);

	m_min_compress_total = n / sizeof(int) - 1;

	if (m_min_compress_total < 10)
		m_min_compress_total = 10;

#ifdef DEBUG
	LOG("m_min_compress_total: %d\n", m_min_compress_total);
#endif

	m_min_page_idx = (int *)calloc(m_min_compress_total + 1, sizeof(int));

	return 0;
}

#if 0
int WikiFullIndex::wfi_init_var(int tmp_hash_total, int max_word_len, int max_find_key_len)
{
	m_tmp_hash_total = tmp_hash_total;
	m_max_word_len = max_word_len;
	m_max_find_key_len = max_find_key_len;

	return 0;
}
#endif

int WikiFullIndex::wfi_init_tmp_mem(size_t mem_size)
{
	m_tmp_mem = (void *)malloc(mem_size);
	memset(m_tmp_mem, 0, mem_size);

	m_tmp_head = (struct wfi_tmp_head *)m_tmp_mem;
	m_tmp_head->max_total = (mem_size - sizeof(*m_tmp_head)) / (sizeof(struct wfi_tmp_rec) + sizeof(unsigned int)); 

	m_tmp_rec = (struct wfi_tmp_rec *)((char *)m_tmp_mem + sizeof(struct wfi_tmp_head)
				+ m_tmp_head->max_total * sizeof(unsigned int));

	m_tmp_hash = new SHash();
	m_tmp_hash->sh_init(500*10000, sizeof(struct wfi_tmp_key), sizeof(struct wfi_tmp_value));

	m_flush_hash = new SHash();
	m_flush_hash->sh_init(100*10000, sizeof(struct fidx_key), sizeof(fidx_value));

	for (int i = 0; i < m_pthread_total; i++) {
		m_key_hash[i] = new SHash();
		m_key_hash[i]->sh_init(10*10000, sizeof(struct wfi_tmp_key), 0);
	}

	return 0;
}

#define _WFI_VALID_THREE_BYTE(x) ((((unsigned char)(x)) >> 5) == 0x7)
#define _WFI_VALID_TWO_BYTE(x) ((!_WFI_VALID_THREE_BYTE(x)) && ((((unsigned char)(x)) >> 6) == 0x3))
#define _WFI_VALID_KEY_CHAR(x) (((x) >= 'a' && (x) <= 'z') \
		|| ((x) >= 'A' && (x) <= 'Z'))

#define _WFI_VALID_DIGIT(x) ((x) >= '0' && (x) <= '9')

#define _WFI_ADD_ONE_WORD(p, len) \
	do { \
		if (len > 0) { \
			memset(&key, 0, sizeof(key)); \
			strncpy(key.word, p, _q_min(len, _WFI_IDX_TITLE_LEN - 1)); \
			key_hash->sh_replace(&key, NULL); \
		} \
	} while (0)

#define _WFI_ADD_MANY_WORD(p, len, word_len) \
	do { \
		int j; \
		for (j = 0; j < (len)/ (word_len); j++) { \
			_WFI_ADD_ONE_WORD(p + (word_len) * j, (word_len)); \
		} \
		if (len % (word_len)) { \
			_WFI_ADD_ONE_WORD(p + (word_len) * j, len % (word_len)); \
		} \
	} while (0)
	
#define _WFI_CHECK_WORD_1(p) _WFI_VALID_THREE_BYTE(0[p])
#define _WFI_CHECK_WORD_2(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_1(p + _WFI_CHECK_WORD_LEN))
#define _WFI_CHECK_WORD_3(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_2(p + _WFI_CHECK_WORD_LEN))
#define _WFI_CHECK_WORD_4(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_3(p + _WFI_CHECK_WORD_LEN))
#define _WFI_CHECK_WORD_5(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_4(p + _WFI_CHECK_WORD_LEN))
#define _WFI_CHECK_WORD_6(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_5(p + _WFI_CHECK_WORD_LEN))
#define _WFI_CHECK_WORD_7(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_6(p + _WFI_CHECK_WORD_LEN))
#define _WFI_CHECK_WORD_8(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_7(p + _WFI_CHECK_WORD_LEN))
#define _WFI_CHECK_WORD_9(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_8(p + _WFI_CHECK_WORD_LEN))
#define _WFI_CHECK_WORD_10(p) (_WFI_CHECK_WORD_1(p) && _WFI_CHECK_WORD_9(p + _WFI_CHECK_WORD_LEN))

#define _WFI_CHECK_ALL_WORD(p, x) \
	if ((x * _WFI_CHECK_WORD_LEN) <= _WFI_IDX_TITLE_LEN - 1 \
				&&  i + x * _WFI_CHECK_WORD_LEN < page_len && _WFI_CHECK_WORD_##x(p)) { \
		memset(&key, 0, sizeof(key)); \
		strncpy(key.word, p, x * _WFI_CHECK_WORD_LEN); \
		if (m_word_hash->sh_find(&key, (void **)NULL) == _SHASH_FOUND) { \
			_WFI_ADD_ONE_WORD(p, x * _WFI_CHECK_WORD_LEN); \
		} \
	}

#ifdef DEBUG
static int m_page_total = 0;
static int m_debug_total = wiki_debug_total();
#endif

int WikiFullIndex::wfi_add_page(int page_idx, const char *page, int page_len, int pthread_idx)
{
	int pos = 0;
	char curr_key[2048];
	const char *p = page;
	struct wfi_tmp_key key;
	SHash *key_hash = m_key_hash[pthread_idx];

	if (m_page_index_pos >= m_index_total)
		return -1;

	key_hash->sh_clean();

	for (int i = 0; i < page_len; i++) {
		if (p[i] == '<') {
			for (; p[i] != '>' && i < page_len; i++);
		}
		if (_WFI_VALID_KEY_CHAR(p[i]) || _WFI_VALID_TWO_BYTE(p[i])) {
			for (pos = 0; pos < (int)sizeof(curr_key) - 1; i++) {
				if (_WFI_VALID_KEY_CHAR(p[i])) {
					curr_key[pos++] = p[i];
				} else if (_WFI_VALID_TWO_BYTE(p[i])) {
					curr_key[pos++] = p[i++];
					curr_key[pos++] = p[i];
				} else {
					break;
				}
			}
			if (pos <= _WFI_IDX_TITLE_LEN - 1)
				_WFI_ADD_ONE_WORD(curr_key, pos);
			else
				_WFI_ADD_MANY_WORD(curr_key, pos, _WFI_IDX_TITLE_LEN - 1);
		}

		if (_WFI_VALID_DIGIT(p[i])) {
			for (pos = 0; pos < (int)sizeof(curr_key) - 1 && _WFI_VALID_DIGIT(p[i]); i++) {
				curr_key[pos++] = p[i];
			}
			if (pos <= 4)
				_WFI_ADD_ONE_WORD(curr_key, pos);
			else
				_WFI_ADD_MANY_WORD(curr_key, pos, 4);
		} 

		if (_WFI_CHECK_WORD_1(p + i)) {
			_WFI_CHECK_ALL_WORD(p + i, 10);
			_WFI_CHECK_ALL_WORD(p + i, 9);
			_WFI_CHECK_ALL_WORD(p + i, 8);
			_WFI_CHECK_ALL_WORD(p + i, 7);
			_WFI_CHECK_ALL_WORD(p + i, 6);
			_WFI_CHECK_ALL_WORD(p + i, 5);
			_WFI_CHECK_ALL_WORD(p + i, 4);
			_WFI_CHECK_ALL_WORD(p + i, 3);
			_WFI_CHECK_ALL_WORD(p + i, 2);

			_WFI_ADD_ONE_WORD(p + i, _WFI_CHECK_WORD_LEN);
			i += _WFI_CHECK_WORD_LEN - 1;
		}
	}

	pthread_mutex_lock(&m_mutex);

	key_hash->sh_reset();
	while (key_hash->sh_read_next(&key, (void **)NULL) == _SHASH_FOUND) {
		wfi_add_one_word(key.word, m_page_index_pos);
	}

	m_page_index[m_page_index_pos++] = page_idx;

#ifdef DEBUG
	m_page_total++;
	if (m_page_total % 1000 == 0) {
		LOG("hash total:%d\n", m_tmp_hash->sh_hash_total());
	}
	if (m_debug_total > 0 && m_page_total >= m_debug_total) {
		struct wfi_tmp_key key;
		struct wfi_tmp_value value;

		FILE *fp = fopen("word.txt.1", "w+");
		m_tmp_hash->sh_reset();
		while (m_tmp_hash->sh_read_next(&key, (void *)&value) == _SHASH_FOUND) {
			fprintf(fp, "%s\n", key.word);
		}
		fclose(fp);
		exit(0);
	}
#endif

	pthread_mutex_unlock(&m_mutex);

	return 0;
}

int WikiFullIndex::wfi_add_page_done()
{
	if (wfi_flush_all_data() == -1) {
		LOG("wfi_flush_all_data() error\n");
		return -1;
	}
	wfi_flush_full_index();

	return 0;
}

int WikiFullIndex::wfi_new_tmp_rec(unsigned int last_idx, unsigned int page_idx, unsigned int *new_last_idx)
{
	if (m_tmp_head->deleted_total > 0) {
		*new_last_idx = m_tmp_head->deleted[--m_tmp_head->deleted_total];
	} else if (m_tmp_head->used < m_tmp_head->max_total - 1) {
		*new_last_idx = ++m_tmp_head->used;
	} else {
		return -1;
	}

	struct wfi_tmp_rec *r = &m_tmp_rec[*new_last_idx];

	r->total = 1;
	r->page_idx[0] = page_idx;
	r->next = 0;

	if (last_idx > 0)
		m_tmp_rec[last_idx].next = *new_last_idx;

	return 0;
}

int WikiFullIndex::wfi_delete_tmp_rec(unsigned int start_idx)
{
	struct wfi_tmp_rec *r;

	for (;;) {
		m_tmp_head->deleted[m_tmp_head->deleted_total++] = start_idx;

		r = &m_tmp_rec[start_idx];
		if ((start_idx = r->next) == 0)
			break;
	}

	return 0;
}

#define _wfi_check_new_tmp_rec(_last_rec_idx, _last_rec_idx_addr) \
	do { \
		if (wfi_new_tmp_rec(_last_rec_idx, page_idx, _last_rec_idx_addr) == -1) { \
			if (wfi_flush_some_data() == -1) \
				return -1; \
			return wfi_add_one_word(word, page_idx); \
		} \
	} while (0)

int WikiFullIndex::wfi_add_one_word(const char *word, int page_idx)
{
	struct wfi_tmp_key key;
	struct wfi_tmp_value *f, value;

	memset(&key, 0, sizeof(key));
	strncpy(key.word, word, sizeof(key.word) - 1);

	q_tolower(key.word);

	if (m_tmp_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		if (f->total == 0) {
			_wfi_check_new_tmp_rec(0, &f->last_rec_idx);
			f->start_rec_idx = f->last_rec_idx;
		} else {
			struct wfi_tmp_rec *r = &m_tmp_rec[f->last_rec_idx];
			if (r->total < _MAX_TMP_PAGE_IDX_TOTAL) {
				r->page_idx[r->total++] = page_idx;
			} else {
				_wfi_check_new_tmp_rec(f->last_rec_idx, &f->last_rec_idx);
			}
		}
		f->total++;
	} else {
		_wfi_check_new_tmp_rec(0, &value.last_rec_idx);

		value.total = 1;
		value.start_rec_idx = value.last_rec_idx;
		m_tmp_hash->sh_add(&key, &value);
	}

	return 0;
}

int WikiFullIndex::wfi_tmp_get_max_k(struct wfi_tmp_max *m, int k)
{
	int idx;
	struct wfi_tmp_max *p, tmp;

	m_tmp_hash->sh_reset();
	while (m_tmp_hash->sh_read_next(&tmp.key, (void **)&tmp.v) == _SHASH_FOUND) {
		idx = -1;

		for (int i = 0; i < k; i++) {
			p = &m[i];
			if (p->v == NULL) {
				idx = i;
				break;
			}
			if (idx == -1 || p->v->total < m[idx].v->total)
				idx = i;
		}
		memcpy(&m[idx], &tmp, sizeof(struct wfi_tmp_max));
	}

	return 0;
}

#define _WFI_FLUSH_FRIST_TOTAL 10

int WikiFullIndex::wfi_flush_some_data()
{
	struct wfi_tmp_max *p, max[_WFI_FLUSH_FRIST_TOTAL];

	memset(max, 0, sizeof(max));
	wfi_tmp_get_max_k(max, _WFI_FLUSH_FRIST_TOTAL);

	for (int i = 0; i < _WFI_FLUSH_FRIST_TOTAL; i++) {
		p = &max[i];
		if (p->v == NULL || p->v->total == 0)
			continue;

		if (wfi_flush_one_data(&p->key, p->v) == -1) {
			LOG("wfi_flush_one_data() error\n");
			return -1;
		}
	}

	return 0;
}

#define _wfi_update_bitmap(_b, _r, _i) \
		_b[_r->page_idx[_i] / 8] |= 1 << (_r->page_idx[_i] % 8)

int WikiFullIndex::wfi_update_one_bitmap(unsigned int idx, unsigned char *bitmap, int *min_page)
{
	struct wfi_tmp_rec *r;
	int n = 0;
	int *p = &min_page[1];

	for (;;) {
		r = &m_tmp_rec[idx];
		for (int i = 0; i < r->total; i++) {
			_wfi_update_bitmap(bitmap, r, i);
			if (n < m_min_compress_total)
				p[n++] = r->page_idx[i];
		}

		if ((idx = r->next) == 0)
			break;
	}

	min_page[0] = n;

	return n >= m_min_compress_total ? 0 : 1;
}

int WikiFullIndex::wfi_flush_one_data(struct wfi_tmp_key *k, struct wfi_tmp_value *v)
{
	int n, flag = 0;
	struct fidx_key key;
	struct fidx_value value;
	char *out = (char *)m_tmp_buf;

	memcpy(&key, k, sizeof(key));

	memset(m_bitmap, 0, m_bitmap_size);
	if (wfi_update_one_bitmap(v->start_rec_idx, m_bitmap, m_min_page_idx)) {
		out = (char *)m_min_page_idx;
		n = m_min_page_idx[0] * sizeof(int) + sizeof(int);
		flag = 1;
	} else {
		if ((n = m_compress_func((char *)m_tmp_buf, m_comp_buf_len, (char *)m_bitmap, m_bitmap_size)) == -1) {
			LOG("compress error: m_bitmap_size = %d\n", m_bitmap_size);
			return -1;
		}
		flag = 0;
	}

	if (wfi_write_one_bitmap_to_file(out, n, &value, flag) == -1) {
		LOG("wfi_write_one_bitmap_to_file error\n");
		return -1;
	}

	m_flush_hash->sh_add(&key, &value);

	wfi_delete_tmp_rec(v->start_rec_idx);
	memset(v, 0, sizeof(struct wfi_tmp_value));

	return 0;
}

#define _WFI_MAX_FLUSH_FILE_SIZE (1800000000) /* 1.8G */

#define _wfi_set_fidx_tmp_fname(_buf, _idx) \
		sprintf(_buf, "%s/fastwiki.full.index.tmp.%d", m_tmp_dir, _idx)

int WikiFullIndex::wfi_write_one_bitmap_to_file(const void *buf, int len, struct fidx_value *v, int flag)
{
	int fd;
	char file[256];

	if (m_curr_flush_fd_idx == -1 || m_flush_size[m_curr_flush_fd_idx] >= _WFI_MAX_FLUSH_FILE_SIZE) {
		m_curr_flush_fd_idx++;

		_wfi_set_fidx_tmp_fname(file, m_curr_flush_fd_idx);

		if ((fd = open(file, O_RDWR | O_CREAT | O_BINARY | O_TRUNC | O_APPEND, 0644)) == -1) {
			LOG("open file %s to write error: %s\n", file, strerror(errno));
			return -1;
		}

		m_flush_fd[m_curr_flush_fd_idx] = fd;
		m_flush_size[m_curr_flush_fd_idx] = 0;
	}

	memset(v, 0, sizeof(struct fidx_value));

	v->file_idx = m_curr_flush_fd_idx;
	v->pos = m_flush_size[m_curr_flush_fd_idx];
	v->len = len;
	v->text_flag = flag;

	m_flush_size[m_curr_flush_fd_idx] += len;

	write(m_flush_fd[m_curr_flush_fd_idx], buf, len);

	return 0;
}

#define _wfi_one_fidx_size(_fd_idx) ((_fd_idx) == 0 \
		? (1800*1024*1024 - _wfi_head_size()) : (1800*1024*1024))

void *wfi_flush_data_pthread(void *arg)
{
	WikiFullIndex *wfi = (WikiFullIndex *)arg;

	wfi->wfi_flush_data_one_pthread();

	return NULL;
}

int WikiFullIndex::wfi_flush_data_one_pthread()
{
	int n;
	struct fidx_value *f;
	void *tmp_find;
	struct tmp_key_val rec;

	unsigned char *bitmap = (unsigned char *)malloc(m_bitmap_size + 1);
	unsigned char *tmp_buf = (unsigned char *)malloc(m_comp_buf_len);

	int *min_page = (int *)calloc(m_min_compress_total + 1, sizeof(int));

	void *tmp1 = malloc(m_comp_buf_len);
	void *tmp2 = malloc(m_comp_buf_len);

	struct wfi_curr_pos *p = &m_wfi_curr_pos;

	for (;;) {
		pthread_mutex_lock(&m_mutex);
		n = read(m_tmp_fd, &rec, sizeof(rec));
		pthread_mutex_unlock(&m_mutex);

		if (n != sizeof(rec))
			break;

		memset(bitmap, 0, m_bitmap_size);

		min_page[0] = 0;

		if (rec.val.total > 0) {
			wfi_update_one_bitmap(rec.val.start_rec_idx, bitmap, min_page);
		}

		m_flush_hash->sh_begin(&tmp_find);
		while (m_flush_hash->sh_next(&tmp_find, &rec.key, (void **)&f) == _SHASH_FOUND) {
			if (wfi_read_file_to_one_bitmap(bitmap, m_bitmap_size, f, min_page, tmp1, tmp2) == -1) {
				LOG("wfi_read_file_to_one_bitmap error\n");
				return -1;
			}
		}

		int text_flag = 0, len = 0;
		char *out = (char *)tmp_buf;

		if (min_page[0] < m_min_compress_total) {
			len = (min_page[0] + 1) * sizeof(int);
			out = (char *)min_page;
			text_flag = 1;
		} else {
			len = m_compress_func((char *)tmp_buf, m_comp_buf_len, (char *)bitmap, m_bitmap_size);
		}

		pthread_mutex_lock(&m_mutex); /* TODO use another mutex */

		if (p->curr_fd_idx == -1 || p->curr_pos > _wfi_one_fidx_size(p->curr_fd_idx)) {
			p->curr_fd_idx++;
			if (p->fd >= 0)
				close(p->fd);
			p->fd = wfi_new_index_file(p->curr_fd_idx);
			p->curr_pos = sizeof(struct fidx_head);
		}

		struct word_value *w;

		if (m_hash->sh_find(&rec.key, (void **)&w) == _SHASH_FOUND) {
			idx_value_t *t = &m_value[w->pos];
			t->pos = p->curr_pos;
			t->file_idx = p->curr_fd_idx;
			t->len = len;
			t->text_flag = text_flag;

			p->curr_pos += len;
			write(p->fd, out, len);
		}

		pthread_mutex_unlock(&m_mutex);
	}

	return 0;
}

static char *__m_string = NULL;
static idx_value_t *__m_value = NULL;

int _cmp_tmp_word(const void *a, const void *b)
{
	struct tmp_key_val *pa = (struct tmp_key_val *)a;
	struct tmp_key_val *pb = (struct tmp_key_val *)b;

	return strcmp(pa->key.word, pb->key.word);
}

int _cmp_reverse_index(const void *a, const void *b)
{
	int n;
	int x = *((int *)a);
	int y = *((int *)b);

	idx_value_t *xa = &__m_value[x]; 
	idx_value_t *xb = &__m_value[y]; 

	int len = _q_min(xa->word_len, xb->word_len);

	for (int i = 0; i < len; i++) {
		n = (unsigned char)__m_string[xa->word_len - 1 - i] - (unsigned char)__m_string[xb->word_len - 1 - i];
		if (n != 0)
			return n;
	}

	return xa->word_len - xb->word_len;
}

#define _WFI_TMP_HASH_FNAME ".wfi.tmp_hash"

int WikiFullIndex::wfi_write_tmp_hash(int *word_len)
{
	struct tmp_key_val rec;

	*word_len = 0;
	m_word_total = m_tmp_hash->sh_hash_total();

	if ((m_tmp_fd = open(_WFI_TMP_HASH_FNAME, O_RDWR | O_CREAT | O_BINARY | O_TRUNC, 0644)) == -1)
		return -1;

	m_tmp_hash->sh_reset();

	while ( m_tmp_hash->sh_read_next(&rec.key, (void *)&rec.val) == _SHASH_FOUND) {
		*word_len += strlen(rec.key.word) + 1;
		write(m_tmp_fd, &rec, sizeof(rec));
	}

	*word_len += 8 - (*word_len) % 8;

	delete m_tmp_hash;

	return 0;
}

int WikiFullIndex::wfi_sort_tmp_record()
{
	int size = m_word_total * (sizeof(struct tmp_key_val));
	char *tmp = (char *)malloc(size);

	lseek(m_tmp_fd, 0, SEEK_SET);
	read(m_tmp_fd, tmp, size);

	qsort(tmp, m_word_total, sizeof(struct tmp_key_val), _cmp_tmp_word);

	lseek(m_tmp_fd, 0, SEEK_SET);
	write(m_tmp_fd, tmp, size);

	free(tmp);

	return 0;
}

int WikiFullIndex::wfi_create_idx_value(int word_len)
{
	
	struct tmp_key_val rec;

	lseek(m_tmp_fd, 0, SEEK_SET);

	m_value = (idx_value_t *)malloc(_wfi_value_size());
	m_string = (char *)malloc(word_len);
	m_reverse_index = (int *)malloc(_wfi_reverse_idx_size());

	int pos = 0;

	for (int i = 0; i < m_word_total; i++) {
		idx_value_t *p = &m_value[i];
		if (read(m_tmp_fd, &rec, sizeof(rec)) != sizeof(rec))
			break;
		p->word_len = strlen(rec.key.word);
		p->word_pos = pos;

		strncpy(m_string + pos, rec.key.word, p->word_len);
		pos += p->word_len;
		m_string[pos++] = 0;

		m_reverse_index[i] = i;
	}

	__m_string = m_string;
	__m_value = m_value;
	qsort(m_reverse_index, m_word_total, sizeof(int), _cmp_reverse_index);

	m_hash = new SHash();
	m_hash->sh_set_hash_magic(get_best_hash((float)m_word_total * 1.1));
	m_hash->sh_init(m_word_total, sizeof(struct word_key), sizeof(struct word_value));

	struct word_key one;
	struct word_value value;

	for (int i = 0; i < m_word_total; i++) {
		idx_value_t *p = &m_value[i];
		memset(&one, 0, sizeof(one));
		strncpy(one.word, m_string + p->word_pos, p->word_len);

		value.pos = i;
		m_hash->sh_add(&one, &value);
	}

	return 0;
}

int WikiFullIndex::wfi_flush_all_data()
{
	wfi_write_tmp_hash(&m_word_str_len);
	wfi_sort_tmp_record();
	wfi_create_idx_value(m_word_str_len);

	lseek(m_tmp_fd, 0, SEEK_SET);

	memset(&m_wfi_curr_pos, 0, sizeof(m_wfi_curr_pos));
	m_wfi_curr_pos.curr_fd_idx = -1;
	m_wfi_curr_pos.curr_pos = sizeof(struct fidx_head);

	pthread_t id[128];

	for (int i = 0; i < m_pthread_total; i++) {
		int n = pthread_create(&id[i], NULL, wfi_flush_data_pthread, (void *)this);
		if (n != 0)
			return -1;
		q_sleep(1);
	}

	for (int i = 0; i < m_pthread_total; i++) {
		pthread_join(id[i], NULL);
	}

	close(m_tmp_fd);
	close(m_wfi_curr_pos.fd);

#ifndef DEBUG
	unlink(_WFI_TMP_HASH_FNAME);
#endif

	return 0;
}

int WikiFullIndex::wfi_read_file_to_one_bitmap(unsigned char *buf, int size, struct fidx_value *f,
		int *min_page, void *tmp1, void *tmp2)
{
	int n;
	unsigned long long *bitmap = (unsigned long long *)buf;
	unsigned long long *tmp = (unsigned long long *)tmp1;

	pthread_mutex_lock(&m_mutex);

	lseek(m_flush_fd[f->file_idx], f->pos, SEEK_SET);
	n = read(m_flush_fd[f->file_idx], tmp1, f->len);

	pthread_mutex_unlock(&m_mutex);

	if (n != (int)f->len) {
		LOG("read tmp file data error: len=%d, but just read: %d\n", f->len, n);
		return -1;
	}

	if (f->text_flag == 1) {
		int pos = min_page[0];
		int *p = (int *)tmp1;
		int total = *p++;
		for (int i = 0; i < total; i++) {
			if (pos < m_min_compress_total)
				min_page[pos++] = p[i];
			buf[p[i] / 8] |= 1 << (p[i] % 8);
		}
		min_page[0] = pos;
	} else {
		if ((n = m_decompress_func((char *)tmp2, m_comp_buf_len, (char *)tmp1, f->len)) == -1) {
			LOG("decompress error: len=%d\n", f->len);
			return -1;
		}

		for (int i = 0; i < m_bitmap_size / 8; i++) {
			bitmap[i] |= tmp[i];
		}
	}

	return 0;
}

const char *WikiFullIndex::wfi_get_fidx_fname(char *file, const char *lang, int idx)
{
	sprintf(file, "fastwiki.fidx.%s.%d", lang, idx);

	return file;
}

int WikiFullIndex::wfi_new_index_file(int curr_fd_idx)
{
	int fd;
	char file[256];
	struct fidx_head head;

	wfi_get_fidx_fname(file, m_lang, curr_fd_idx);

	if ((fd = open(file, O_RDWR | O_CREAT | O_BINARY | O_TRUNC, 0644)) == -1)
		return -1;

	if (curr_fd_idx > 0) {
		memset(&head, 0, sizeof(head));
		head.file_idx = curr_fd_idx;

		write(fd, &head, sizeof(head));
	}

	return fd;
}

int WikiFullIndex::wfi_check_inline_index()
{
	for (int i = 0; i < m_page_index_pos; i++) {
		if (m_page_index[i] != i)
			return 0;
	}

	return 1;
}

int WikiFullIndex::wfi_recount_value_file_idx()
{
	int add = _wfi_page_index_size() + _wfi_hash_size()
		+ _wfi_value_size() + _wfi_word_length() + _wfi_reverse_idx_size();

	for (int i = 0; i < m_word_total; i++) {
		idx_value_t *p = &m_value[i];
		if (p->file_idx == 0) {
			p->pos += add;
		}
	}
	
	return 0;
}

int WikiFullIndex::wfi_set_head(struct fidx_head *h)
{
	void *addr;
	int size;

	m_hash->sh_get_addr(&addr, &size);

	memset(h, 0, sizeof(*h));

	h->z_flag = m_z_flag;
	h->bitmap_size = m_bitmap_size;
	strncpy(h->lang, m_lang, sizeof(h->lang) - 1);
	h->file_idx = 0;

	h->page_index_flag = wfi_check_inline_index();
	h->page_index_total = m_index_total;

	h->value_total = m_word_total;
	h->hash_pos = sizeof(struct fidx_head) + _wfi_page_index_size();
	h->hash_size = size;

	h->value_pos = h->hash_pos + h->hash_size;
	h->string_pos = h->value_pos + _wfi_value_size();
	h->string_len = _wfi_word_length();

	h->reverse_index_pos = h->string_pos + _wfi_word_length();

	return 0;
}

int WikiFullIndex::wfi_flush_full_index()
{
	int fd, size;
	char tmp_file[] = ".tmp.fw.fidx";
	struct fidx_head head;
	void *addr;

	wfi_recount_value_file_idx();
	wfi_set_head(&head);
	m_hash->sh_get_addr(&addr, &size);

	if ((fd = open(tmp_file, O_RDWR | O_CREAT | O_BINARY | O_TRUNC, 0644)) == -1) {
		LOG("open tmp file %s to write error: %s\n", tmp_file, strerror(errno));
		return -1;
	}

	write(fd, &head, sizeof(head));
	write(fd, m_page_index, _wfi_page_index_size());
	write(fd, addr, head.hash_size);

	write(fd, m_value, _wfi_value_size());
	write(fd, m_string, _wfi_word_length());
	write(fd, m_reverse_index, _wfi_reverse_idx_size());

	int n, fd2;
	char file[256];

	wfi_get_fidx_fname(file, m_lang, 0);

	if ((fd2 = open(file, O_RDONLY | O_BINARY)) == -1) {
		LOG("read file %s error: %s\n", file, strerror(errno));
		return -1;
	}

	while ((n = read(fd2, m_tmp_buf, m_bitmap_size)) > 0)
		write(fd, m_tmp_buf, n);

	close(fd2);
	close(fd);

	unlink(file);
	rename(tmp_file, file);

	wfi_rewrite_file_head(&head);

	return 0;
}

int WikiFullIndex::wfi_rewrite_file_head(const struct fidx_head *h)
{
	int fd;
	char file[256];
	struct fidx_head head;

	for (int i = 1; i < MAX_FD_TOTAL; i++) {
		wfi_get_fidx_fname(file, m_lang, i);
		if (!dashf(file))
			break;

		if ((fd = open(file, O_RDWR | O_BINARY)) == -1)
			break;
		lseek(fd, 0, SEEK_SET);
		if (read(fd, &head, sizeof(head)) != sizeof(head)) {
			break;
		}
		head.z_flag = h->z_flag;
		head.bitmap_size = h->bitmap_size;
		head.page_index_flag = h->page_index_flag;
		strncpy(head.random, h->random, sizeof(head.random) - 1);
		strncpy(head.lang, h->lang, sizeof(head.lang) - 1);

		lseek(fd, 0, SEEK_SET);
		write(fd, &head, sizeof(head));

		close(fd);
	}

	for (int i = 0; i <= m_curr_flush_fd_idx; i++) {
		_wfi_set_fidx_tmp_fname(file, i);
		close(m_flush_fd[i]);
#ifndef DEBUG
		unlink(file);
#endif
	}
	
	return 0;
}

#define _(x) \
	do { \
		char f[128]; \
		sprintf(f, "%-12s: %%%s\n", #x, ((long)&h->x == (long)h->x) ? "s" : "d"); \
		printf(f, h->x); \
	} while (0)

int WikiFullIndex::wfi_stat()
{
	struct fidx_head *h = &m_head;

	if (m_ro_init_flag == 0)
		return 0;

	_(name);
	_(version);
	_(z_flag);
	_(random);
	_(lang);
	_(file_idx);
	_(bitmap_size);

	_(page_index_flag);
	_(page_index_total);
	_(value_total);
	_(hash_pos);
	_(hash_size);

	_(value_pos);
	_(string_pos);
	_(string_len);
	_(reverse_index_pos);

	_(head_crc32);
	_(crc32);

	printf("\n");

	printf("hash_total: %d\n", m_hash->sh_hash_total());

	return 0;
}

int WikiFullIndex::wfi_unload_all_word(const char *file)
{
	FILE *fp;
	struct word_key key;
	struct word_value value;

	if ((fp = fopen(file, "w+")) == NULL) {
		LOG("open file %s to write fail.\n", file);
		return -1;
	}

	m_hash->sh_fd_reset();
	while (m_hash->sh_fd_read_next(&key, &value) == _SHASH_FOUND) {
		fprintf(fp, "%s\n", key.word);
	}

	fclose(fp);

	return 0;
}

static unsigned char m_bit_hash[] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 
};

#define _WFI_COUNT_TOTAL(x) m_bit_hash[(comp_buf[i] >> (x * 8)) & 0xff]

int WikiFullIndex::wfi_count(const char *file)
{
	FILE *fp;
	struct word_key key;
	struct word_value value;

	int *total = (int *)malloc(m_head.value_total * sizeof(int));
	memset(total, 0, m_head.value_total * sizeof(int));

	if ((fp = fopen(file, "w+")) == NULL) {
		LOG("open file %s to write fail.\n", file);
		return -1;
	}

	int t, len;
	unsigned long long *comp_buf = (unsigned long long *)m_comp_buf;

	m_hash->sh_fd_reset();
	while (m_hash->sh_fd_read_next(&key, &value) == _SHASH_FOUND) {
		idx_value_t v;
		wfi_get_value(value.pos, &v);

		lseek(m_fd[v.file_idx], v.pos, SEEK_SET);
		read(m_fd[v.file_idx], m_tmp_buf, v.len);

		if (v.text_flag == 1) {
			int *p = (int *)m_tmp_buf;
			t = p[0];
			goto out;
		}

		len = m_decompress_func(m_comp_buf, m_comp_buf_len, (char *)m_tmp_buf, v.len);
		if (len != m_head.bitmap_size) {
			LOG("fastwiki fdix file maybe damage\n");
			continue;
		}

		t = 0;
		for (int i = 0; i < len / 8; i++) {
			if (comp_buf[i] & 0xffffffff) {
				t += _WFI_COUNT_TOTAL(0);
				t += _WFI_COUNT_TOTAL(1);
				t += _WFI_COUNT_TOTAL(2);
				t += _WFI_COUNT_TOTAL(3);
			}
			comp_buf[i] >>= 32;
			if (comp_buf[i] & 0xffffffff) {
				t += _WFI_COUNT_TOTAL(0);
				t += _WFI_COUNT_TOTAL(1);
				t += _WFI_COUNT_TOTAL(2);
				t += _WFI_COUNT_TOTAL(3);
			}
		}
out:
		total[t]++;
		printf("%s: %d\n", key.word, t);
	}

	for (int i = 0; i < m_head.value_total; i++) {
		if (total[i] > 0) {
			fprintf(fp, "%d: %d\n", i, total[i]);
		}
	}

	fclose(fp);

	return 0;
}
