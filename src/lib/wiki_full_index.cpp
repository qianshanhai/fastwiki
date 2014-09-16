/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "q_util.h"

#include "wiki_full_index.h"

#define _wfi_page_index_size() (m_index_total * sizeof(int))

WikiFullIndex::WikiFullIndex()
{
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
		if ((fd = open(file[i], O_RDONLY | O_BINARY)) == -1)
			return -1;
		read(fd, &m_head, sizeof(m_head));
		m_fd[m_head.file_idx] = fd;
	}

	if (m_fd[0] == -1) {
		return -1;
	}

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

int WikiFullIndex::wfi_find(const char *string, int *page_idx, int max_total)
{
	split_t sp;
	struct fidx_key key;
	struct fidx_value value;
	int find_flag, real_total = 0;

	unsigned long long *bitmap = (unsigned long long *)m_bitmap;
	unsigned long long *comp_buf = (unsigned long long *)m_comp_buf;

	memset(bitmap, 0, m_head.bitmap_size);

	if (m_ro_init_flag == 0)
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

		lseek(m_fd[value.file_idx], value.pos, SEEK_SET);
		read(m_fd[value.file_idx], m_tmp_buf, value.len);

		int len = m_decompress_func(m_comp_buf, m_comp_buf_len, (char *)m_tmp_buf, value.len);
		if (len != m_head.bitmap_size)
			return -1; /* 文件可能已经损坏 */

		if (sp.I == 0) {
			memcpy(bitmap, comp_buf, len);
			continue;
		}

		find_flag = 0;

		for (int j = 0; j < len / 8; j++) {
			bitmap[j] &= comp_buf[j];
			if (bitmap[j]) {
				find_flag = 1;
				break;
			}
		}

		if (find_flag == 0)
			return 0;
	}

	unsigned char *p;

	for (int find_idx = 0; find_idx < m_head.bitmap_size / 8; find_idx++) {
		if (bitmap[find_idx] == 0)
			continue;

		p = (unsigned char *)&bitmap[find_idx];
		for (int i = 0; i < 8; i++) {
			if (p[i]) {
				_wfi_check_char();
			}
		}
	}

	return real_total;
}

inline int WikiFullIndex::wfi_convert_index(int idx)
{
	int tmp = 0;

	if (m_head.index_flag == 1)
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

	m_page_index_pos = 0;
	m_page_index = (int *)malloc(_wfi_page_index_size());
	memset(m_page_index, 0, _wfi_page_index_size());

	strcpy(m_lang, lang);
	strcpy(m_tmp_dir, tmp_dir);

	m_pthread_total = pthread_total;
	pthread_mutex_init(&m_mutex, NULL);

	wfi_init_tmp_mem(mem_size);

	m_compress_func = get_compress_func(&m_z_flag, z_flag);
	if (m_z_flag == -1)
		return -1;

	return 0;
}

int WikiFullIndex::wfi_init_tmp_mem(size_t mem_size)
{
	m_tmp_mem = (void *)malloc(mem_size);
	memset(m_tmp_mem, 0, mem_size);

	m_tmp_head = (struct wfi_tmp_head *)m_tmp_mem;
	m_tmp_head->max_total = (mem_size - sizeof(*m_tmp_head)) / (sizeof(struct wfi_tmp_rec) + sizeof(unsigned int)); 

	m_tmp_rec = (struct wfi_tmp_rec *)((char *)m_tmp_mem + sizeof(struct wfi_tmp_head)
				+ m_tmp_head->max_total * sizeof(unsigned int));

	m_tmp_hash = new SHash();
	m_tmp_hash->sh_init(30*10000, sizeof(struct wfi_tmp_key), sizeof(struct wfi_tmp_value));

	m_flush_hash = new SHash();
	m_flush_hash->sh_init(30*10000, sizeof(struct fidx_key), sizeof(fidx_value));

	for (int i = 0; i < m_pthread_total; i++) {
		m_key_hash[i] = new SHash();
		m_key_hash[i]->sh_init(30*10000, sizeof(struct wfi_tmp_key), 0);
	}

	return 0;
}

#define _WFI_VALID_KEY_CHAR(x) (((x) >= 'a' && (x) <= 'z') \
		|| ((x) >= 'A' && (x) <= 'Z') \
		|| (x) == '-')

#define _WFI_VALID_DIGIT(x) ((x) >= '0' && (x) <= '9')

#define _WFI_ADD_PAGE_CHECK_POS() \
	do { \
		if (key_pos > 0) { \
			memset(&key, 0, sizeof(key)); \
			strncpy(key.word, curr_key, key_pos); \
			key_hash->sh_replace(&key, NULL); \
			key_pos = 0; \
		} \
	} while (0)
	
#define _WFI_CHECK_WORD_LEN 3

#define _WFI_CHECK_WORD_1(p) (((unsigned char)(0[p])) & 0x80)
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
	if (_WFI_CHECK_WORD_##x(p)) { \
		_WFI_ADD_PAGE_CHECK_POS(); \
		memset(&key, 0, sizeof(key)); \
		strncpy(key.word, p, x * _WFI_CHECK_WORD_LEN); \
		if (m_word_hash->sh_find(&key, (void **)NULL) == _SHASH_FOUND) { \
			strncpy(curr_key, p, x * _WFI_CHECK_WORD_LEN); \
			key_pos = x * _WFI_CHECK_WORD_LEN; \
			_WFI_ADD_PAGE_CHECK_POS(); \
		} \
	}

#define _WFI_CHECK_DIGIT_LEN() \
	do { \
		if (key_pos > 0 && _WFI_VALID_DIGIT(curr_key[0])) { \
			if (key_pos <= 4) \
				_WFI_ADD_PAGE_CHECK_POS(); \
			else \
				key_pos = 0; \
		} \
	} while (0)

#ifdef DEBUG
static int m_page_total = 0;
#endif

int WikiFullIndex::wfi_add_page(int page_idx, const char *page, int page_len, int pthread_idx)
{
	int key_pos = 0;
	char curr_key[_WFI_IDX_TITLE_LEN];
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
		if (_WFI_VALID_KEY_CHAR(p[i])) {
			_WFI_CHECK_DIGIT_LEN();
			if (key_pos < _WFI_IDX_TITLE_LEN - 1)
				curr_key[key_pos++] = p[i];
			else {
				key_pos = 0;
			}
		} else if (_WFI_VALID_DIGIT(p[i])) {
			if (key_pos > 0 && !_WFI_VALID_DIGIT(curr_key[0])) {
				_WFI_ADD_PAGE_CHECK_POS();
			}
			if (key_pos < _WFI_IDX_TITLE_LEN - 1)
				curr_key[key_pos++] = p[i];
		} else if (_WFI_CHECK_WORD_1(p + i)) {
			_WFI_CHECK_DIGIT_LEN();
			_WFI_ADD_PAGE_CHECK_POS();

			_WFI_CHECK_ALL_WORD(p + i, 10);
			_WFI_CHECK_ALL_WORD(p + i, 9);
			_WFI_CHECK_ALL_WORD(p + i, 8);
			_WFI_CHECK_ALL_WORD(p + i, 7);
			_WFI_CHECK_ALL_WORD(p + i, 6);
			_WFI_CHECK_ALL_WORD(p + i, 5);
			_WFI_CHECK_ALL_WORD(p + i, 4);
			_WFI_CHECK_ALL_WORD(p + i, 3);
			_WFI_CHECK_ALL_WORD(p + i, 2);

			strncpy(curr_key, p + i, _WFI_CHECK_WORD_LEN);
			key_pos = _WFI_CHECK_WORD_LEN;
			_WFI_ADD_PAGE_CHECK_POS();

			i += _WFI_CHECK_WORD_LEN - 1;
		} else {
			_WFI_ADD_PAGE_CHECK_POS();
		}
	}

	_WFI_ADD_PAGE_CHECK_POS();

	pthread_mutex_lock(&m_mutex);

	key_hash->sh_reset();
	while (key_hash->sh_read_next(&key, (void **)NULL) == _SHASH_FOUND) {
		wfi_add_one_word(key.word, m_page_index_pos);
	}

	m_page_index[m_page_index_pos++] = page_idx;
	m_page_total++;

	pthread_mutex_unlock(&m_mutex);

#ifdef DEBUG
	if (m_page_total % 1000 == 0) {
		printf("hash total:%d\n", m_tmp_hash->sh_hash_total());
		fflush(stdout);
	}
#endif

	return 0;
}

int WikiFullIndex::wfi_add_page_done()
{
	wfi_flush_all_data();
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

int WikiFullIndex::wfi_add_one_word(const char *word, int page_idx)
{
	struct wfi_tmp_key key;
	struct wfi_tmp_value *f, value;

	memset(&key, 0, sizeof(key));
	strncpy(key.word, word, sizeof(key.word) - 1);

	q_tolower(key.word);

	if (m_tmp_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		struct wfi_tmp_rec *r = &m_tmp_rec[f->last_rec_idx];
		if (r->total < _MAX_TMP_PAGE_IDX_TOTAL - 1) {
			r->page_idx[r->total++] = page_idx;
		} else {
			if (wfi_new_tmp_rec(f->last_rec_idx, page_idx, &f->last_rec_idx) == -1) {
				wfi_flush_some_data();
				return wfi_add_one_word(word, page_idx);
			}
		}
		f->total++;
	} else {
		if (wfi_new_tmp_rec(0, page_idx, &value.last_rec_idx) == -1) {
			wfi_flush_some_data();
			return wfi_add_one_word(word, page_idx);
		}

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

		wfi_flush_one_data(&p->key, p->v);
	}

	return 0;
}

#define _wfi_update_bitmap(_b, _r, _i) \
		_b[_r->page_idx[_i] / 8] |= 1 << (_r->page_idx[_i] % 8)

int WikiFullIndex::wfi_update_one_bitmap(unsigned int idx, unsigned char *bitmap)
{
	struct wfi_tmp_rec *r;

	for (;;) {
		r = &m_tmp_rec[idx];
		for (int i = 0; i < r->total; i++)
			_wfi_update_bitmap(bitmap, r, i);

		if ((idx = r->next) == 0)
			break;
	}

	return 0;
}

int WikiFullIndex::wfi_update_one_bitmap_to_file(struct fidx_value *f, struct wfi_tmp_value *v)
{
	int fd = m_flush_fd[f->file_idx];

	lseek(fd, f->pos, SEEK_SET);

	if (read(fd, m_bitmap, m_bitmap_size) != m_bitmap_size)
		memset(m_bitmap, 0, m_bitmap_size);

	wfi_update_one_bitmap(v->start_rec_idx, m_bitmap);

	lseek(fd, f->pos, SEEK_SET);
	if (write(fd, m_bitmap, m_bitmap_size) != m_bitmap_size)
		return -1;

	wfi_delete_tmp_rec(v->start_rec_idx);
	memset(v, 0, sizeof(struct wfi_tmp_value));

	return 0;
}

int WikiFullIndex::wfi_flush_one_data(struct wfi_tmp_key *k, struct wfi_tmp_value *v)
{
	struct fidx_key key;
	struct fidx_value *f, value;

	memcpy(&key, k, sizeof(key));

	if (m_flush_hash->sh_find(&key, (void **)&f) != _SHASH_FOUND) {
		wfi_new_flush_record(&value);
		m_flush_hash->sh_add(&key, &value);
		f = &value;
	}

	wfi_update_one_bitmap_to_file(f, v);

	return 0;
}

#define _WFI_MAX_FLUSH_FILE_SIZE (1800000000) /* 1.8G */

#define _wfi_set_fidx_tmp_fname(_buf, _idx) \
		sprintf(_buf, "%s/fastwiki.full.index.tmp.%d", m_tmp_dir, _idx)

int WikiFullIndex::wfi_new_flush_record(struct fidx_value *v)
{
	int fd;
	char file[256];

	memset(v, 0, sizeof(struct fidx_value));

	if (m_curr_flush_fd_idx == -1 || m_flush_size[m_curr_flush_fd_idx] >= _WFI_MAX_FLUSH_FILE_SIZE) {
		m_curr_flush_fd_idx++;

		_wfi_set_fidx_tmp_fname(file, m_curr_flush_fd_idx);

		if ((fd = open(file, O_RDWR | O_CREAT | O_BINARY | O_TRUNC, 0644)) == -1)
			return -1;

		m_flush_fd[m_curr_flush_fd_idx] = fd;
		m_flush_size[m_curr_flush_fd_idx] = 0;
	}

	v->file_idx = m_curr_flush_fd_idx;
	v->pos = m_flush_size[m_curr_flush_fd_idx];
	v->len = m_bitmap_size;

	m_flush_size[m_curr_flush_fd_idx] += m_bitmap_size;

	return 0;
}

int WikiFullIndex::wfi_flush_all_data()
{
	struct wfi_tmp_key key;
	struct wfi_tmp_value *v;
	struct fidx_value *f;

	int fd = -1, curr_fd_idx = -1;
	unsigned int curr_pos = sizeof(struct fidx_head);

	m_hash = new SHash();
	m_hash->sh_init(m_tmp_hash->sh_hash_total() + 1, sizeof(struct fidx_key), sizeof(struct fidx_value));

	m_tmp_hash->sh_reset();

	while (m_tmp_hash->sh_read_next(&key, (void **)&v) == _SHASH_FOUND) {
		if (curr_fd_idx == -1 || curr_pos > (1600*1024*1024)) {
			curr_fd_idx++;
			if (fd >= 0)
				close(fd);
			fd = wfi_new_index_file(curr_fd_idx);
			curr_pos = sizeof(struct fidx_head);
		}
		if (m_flush_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
			lseek(m_flush_fd[f->file_idx], f->pos, SEEK_SET);
			read(m_flush_fd[f->file_idx], m_bitmap, f->len);
		} else {
			memset(m_bitmap, 0, m_bitmap_size);
		}
		if (v->total > 0) {
			wfi_update_one_bitmap(v->start_rec_idx, m_bitmap);
		}
		struct fidx_value value;

		memset(&value, 0, sizeof(value));

		value.pos = curr_pos;
		value.file_idx = curr_fd_idx;

		value.len = m_compress_func((char *)m_tmp_buf, m_comp_buf_len, (char *)m_bitmap, m_bitmap_size);
		write(fd, m_tmp_buf, value.len);
		curr_pos += value.len;

		m_hash->sh_add(&key, &value);
	}

	close(fd);

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

int WikiFullIndex::wfi_flush_full_index()
{
	int fd;
	char tmp_file[] = ".tmp.fw.fidx";
	struct fidx_head head;

	if ((fd = open(tmp_file, O_RDWR | O_CREAT | O_BINARY | O_TRUNC, 0644)) == -1)
		return -1;

	memset(&head, 0, sizeof(head));

	void *addr;
	struct fidx_key key;
	struct fidx_value *f;

	m_hash->sh_get_addr(&addr, &head.hash_size);

	head.bitmap_size = m_bitmap_size;
	head.z_flag = m_z_flag;
	head.hash_pos = sizeof(head) + _wfi_page_index_size();
	head.file_idx = 0;
	head.index_flag = wfi_check_inline_index();

	strncpy(head.lang, m_lang, sizeof(head.lang) - 1);

	write(fd, &head, sizeof(head));
	write(fd, m_page_index, _wfi_page_index_size());

	m_hash->sh_reset();
	while (m_hash->sh_read_next(&key, (void **)&f) == _SHASH_FOUND) {
		if (f->file_idx == 0)
			f->pos += _wfi_page_index_size() + head.hash_size;
	}

	write(fd, addr, head.hash_size);

	int n, fd2;
	char file[256];

	wfi_get_fidx_fname(file, m_lang, 0);

	if ((fd2 = open(file, O_RDONLY)) == -1)
		return -1;

	while ((n = read(fd2, m_tmp_buf, m_bitmap_size)) > 0)
		write(fd, m_tmp_buf, n);

	close(fd2);
	close(fd);

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
		head.index_flag = h->index_flag;
		strncpy(head.random, h->random, sizeof(head.random) - 1);
		strncpy(head.lang, h->lang, sizeof(head.lang) - 1);

		lseek(fd, 0, SEEK_SET);
		write(fd, &head, sizeof(head));

		close(fd);
	}

	for (int i = 0; i <= m_curr_flush_fd_idx; i++) {
		_wfi_set_fidx_tmp_fname(file, i);
		close(m_flush_fd[i]);
		//unlink(file);
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

	_(z_flag);
	_(hash_pos);
	_(hash_size);
	_(file_idx);
	_(bitmap_size);
	_(index_flag);
	_(random);
	_(lang);
	_(head_crc32);
	_(crc32);

	printf("\n");

	printf("hash_total: %d\n", m_hash->sh_hash_total());

	return 0;
}
