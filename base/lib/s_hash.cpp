/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "q_util.h"

#include "crc32sum.h"
#include "s_hash.h"
#include "prime.h"

#ifndef _SHASH_RATE
#define _SHASH_RATE (0.6f)
#endif

#define _SHASH_NOT_FOUND_NEXT 2

#define _SHASH_MEM_FLAG 0x2
#define _SHASH_FILE_FLAG 0x4
#define _SHASH_READ_ONLY_MEM_FLAG 0x8
#define _SHASH_FD_FLAG 0x10

/*
 * hash structure:
 * [0 1 2 ...  n][r1 r2 r3 r4 ... rx]
 *               [---------> curr
 * one of [0 1 2 ... n] is integer,
 * r1 .. rx is record
 * 
 */
struct shm_hash_head {
	int uptime;
	int remalloc_flag;
	int curr; /* used */
	int total;
	int hash; /* == n */
	int hash_used; /* */
	int data; /* max_total */
	int max_same; /* */
	int key_len;
	int value_len;
	char reserve[64];
	char user_data[SHASH_USER_DATA_LEN]; /* out data*/
};

#define _RECORD_USED_FLAG 0x80000000 /* = 1 << 31 */

#define sh_get_pos(_r, _n) \
	(void *)((char *)(_r) + (_n - 1) * ((key_len + value_len) + sizeof(unsigned int)))

#define sh_get_key(_p) (_p)
#define sh_get_value(_p) (void *)((char *)(_p) + key_len)
#define sh_get_key_from_value(_p) (void *)((char *)(_p) - key_len)
#define _sh_get_next(_p) *((unsigned int *)((char *)(_p) + value_len + key_len))
#define sh_get_next(_p) (_sh_get_next(_p) & (~ _RECORD_USED_FLAG))
#define sh_set_next(_p, n) \
	do { \
		_sh_get_next(_p) &= _RECORD_USED_FLAG; \
		_sh_get_next(_p) |= ((unsigned int)(n) & (~_RECORD_USED_FLAG)); \
	} while (0)

#define sh_record_is_used(_p) (_sh_get_next(_p) & _RECORD_USED_FLAG)
#define sh_delete_a_record(_p) \
	do { \
		_sh_get_next(_p) &= ~_RECORD_USED_FLAG; \
	} while (0)

#define sh_get_rec(_h) \
	(void *)((char *)(_h) + (_h)->hash * sizeof(unsigned int) + sizeof(struct shm_hash_head))

#define sh_get_hash(_h) \
	(int *)((char *)(_h) + sizeof(struct shm_hash_head))

#define sh_get_head() (struct shm_hash_head *)mem
#define sh_get_size() old_size

#define _sh_check_head(_h, ret) _h = sh_get_head();

#ifdef DEBUG

#define debug_log(x) \
	do { \
		if (log) \
			log x; \
	} while (0)
#else

#define debug_log(x)

#endif

/** @endcond */

/*
 * default compare function
 */
inline int _sh_cmp_func(SHM_CMP_ARG(type, old_key, key, len))
{
	return memcmp(old_key, key, len) == 0 ? 0 : 1;
}

inline int _sh_add_func(SHM_ADD_ARG(type, key, new_key, len))
{
	memcpy(key, new_key, len);

	return 0;
}

inline int _sh_key_func(SHM_KEY_ARG(type, key, len, new_key, ret_len))
{
	*new_key = (void *)key;
	*ret_len = len;

	return 0;
}

void SHash::sh_var_init()
{
	fd = -1;

	rec = find = used = NULL;

	log = printf;
	value_len = key_len = 0;
	shm_flag = 0;

	key_func = _sh_key_func;

	cmp_func = _sh_cmp_func;
	add_func = _sh_add_func;
	func_type = (void *)this;

	hash_magic = 0;
	hash_rate = _SHASH_RATE;

	old_size = 0;
}

SHash::SHash()
{
	sh_var_init();
}

SHash::SHash(int flag)
{
	sh_var_init();

	if (flag == SHASH_FAST)
		hash_rate = 1.0f;
}

SHash::~SHash()
{
	if (shm_flag & _SHASH_FD_FLAG) {
		close(fd);
		free(find);
		free(m_head);
		return;
	}

	if (mem) {
		if (shm_flag & _SHASH_MEM_FLAG)
			free(mem);
		else if (shm_flag & _SHASH_FILE_FLAG) {
#ifndef WIN32
			munmap(mem, old_size);
#endif
			close(fd);
		} else if (shm_flag & _SHASH_READ_ONLY_MEM_FLAG) {
			; // do nothing
		}
		mem = NULL;
		old_size = 0;
	}
}

#define _SH_MIN_HASH (1000)

int SHash::sh_fd_init_ro(int _fd, unsigned int pos)
{
	struct shm_hash_head *p;

	fd = _fd;
	m_pos = pos;

	m_head = (void *)malloc(sizeof(struct shm_hash_head));

	lseek(fd, pos, SEEK_SET);
	read(fd, m_head, sizeof(struct shm_hash_head));

	p = (struct shm_hash_head *)m_head;

	key_len = p->key_len;
	value_len = p->value_len;

	shm_flag = _SHASH_FD_FLAG;

	return 0;
}

#define sh_fd_get_rec(_h) ((_h)->hash * sizeof(unsigned int) + sizeof(struct shm_hash_head))
#define sh_fd_get_pos(_n) ((_n - 1) * ((key_len + value_len) + sizeof(unsigned int)))

#include "q_util.h"

int SHash::sh_fd_find(const void *key, void *value)
{
	char buf[1024];
	unsigned int next;
	unsigned int hash_value = 0;
	struct shm_hash_head *head = (struct shm_hash_head *)m_head;

	unsigned int offset;

	crc32 = crc32sum((char *)key, head->key_len) % head->hash;

	offset = m_pos + sizeof(struct shm_hash_head) + crc32 * sizeof(int);
	lseek(fd, offset, SEEK_SET);
	read(fd, &hash_value, sizeof(hash_value));

	if (hash_value == 0)
		return _SHASH_NOT_FOUND;

	unsigned int rec_pos = sh_fd_get_rec(head);
	unsigned int rec_seek = sh_fd_get_pos(hash_value);

	for (;;) {
		memset(buf, 0, key_len + value_len + sizeof(int));

		offset = m_pos + rec_pos + rec_seek;
		pread(fd, buf, key_len + value_len + sizeof(int), offset);

		if (sh_record_is_used(buf)) {
			if (cmp_func(func_type, sh_get_key(buf), key, key_len) == 0) {
				memcpy(value, sh_get_value(buf), value_len);
				return _SHASH_FOUND;
			}
		}

		if ((next = sh_get_next(buf)) == 0)
			break;

		rec_seek = sh_fd_get_pos(next);
	}

	return _SHASH_NOT_FOUND;
}

int SHash::sh_init(const char *file, int k_len, int v_len, int total, int write_flag)
{
	int mode = O_RDONLY;

	if (write_flag)
		mode |= O_RDWR | O_CREAT;

	if ((fd = open(file, mode, 0644)) == -1) {
		debug_log(("file = %s: %s\n", file, strerror(errno)));
		return -1;
	}

	return sh_fd_init(fd, total, k_len, v_len, write_flag);
}

int SHash::sh_init_addr(void *addr, int size, int k_len, int v_len)
{
	shm_flag = _SHASH_READ_ONLY_MEM_FLAG;

	key_len = k_len;
	value_len = v_len;

	mem = (void *)addr;
	old_size = size;

	if (sh_init_head(0) == -1)
		return -1;

	return 0;
}

int SHash::sh_fd_init(int _fd, int total, int k_len, int v_len, int write_flag)
{
	fd = _fd;
	shm_flag = _SHASH_FILE_FLAG;

	key_len = k_len;
	value_len = v_len;

	if (sh_check_key_value_len(total) == -1)
		return -1;
	
	if (write_flag && file_size(fd) == 0) {
		if (total == 0)
			return -1;
		if (ftruncate(fd, old_size) == -1) {
			debug_log(("ftruncate fd=%d size=%llu: %s\n", fd,
						(unsigned long long)old_size, strerror(errno)));
			return -1;
		}
	}

#ifndef WIN32
	if ((mem = q_mmap(fd, &old_size, write_flag)) == NULL) {
		debug_log(("fd: %s\n", strerror(errno)));
		return -1;
	}
#endif

	if (sh_init_head(write_flag) == -1)
		return -1;

	return 0;
}

void SHash::sh_set_hash_magic(int magic)
{
	hash_magic = magic;
}

int SHash::sh_check_key_value_len(int total)
{
	if (key_len <= 0)
		return -1;

	if (value_len <= 0)
		value_len = 0;

	if (hash_magic <= 0) {
		hash_magic = get_best_hash((int)(total * hash_rate));
		if (hash_magic < _SH_MIN_HASH)
			hash_magic = get_best_hash(_SH_MIN_HASH);
	}

	if (total > 0)
		old_size = sizeof(struct shm_hash_head) + sizeof(int) * hash_magic
				+ (unsigned int)((total * (key_len + value_len + sizeof(int)) * 1.1f));
	else {
		old_size = 0;
	}

	return 0;
}

/*
 * total  max record total
 * k_len  key length
 * v_len  value length
 */
int SHash::sh_init(int total, int k_len, int v_len, int mode)
{
	int flag = mode & 0600 ? 1 : 0;
	shm_flag =  _SHASH_MEM_FLAG;

	key_len = k_len;
	value_len = v_len;

	if (sh_check_key_value_len(total) == -1) {
		debug_log(("sh_check_key_value_len == -1\n"));
		return -1;
	}

	if (total <= 0)
		flag = 0;

	if ((mem = (void *)malloc(old_size)) == NULL) {
		debug_log(("error: malloc: size=%llu, %s\n", old_size, strerror(errno)));
		return -1;
	}
	memset(mem, 0, old_size);
	flag = 1;

	if (sh_init_head(flag) == -1)
		return -1;

	return 0;
}

void SHash::sh_set_func(void *type, shm_cmp_func_t _cmp,
		shm_key_func_t _key, shm_add_func_t _add)
{
	if (type)
		func_type = type;

	if (_cmp)
		cmp_func = _cmp;

	if (_key)
		key_func = _key;

	if (_add)
		add_func = _add;
}

void SHash::sh_set_log_func(log_func_t lt)
{
	log = lt;
}

int SHash::sh_init_head(int flag)
{
	size_t size = 0;
	int max_total;

	if ((size = sh_get_size()) == 0) {
		debug_log(("get_size=0 %s\n", strerror(errno)));
		return -1;
	}

	struct shm_hash_head *head = sh_get_head();

	if (head->hash == 0)
		head->hash = hash_magic;

	max_total = (size - sizeof(*head) - head->hash * sizeof(int))
				/ ((value_len + key_len) + sizeof(int));
	
	if (flag) {
		head->remalloc_flag = 0;
		if (head->uptime == 0) {
			head->uptime = (int)time(NULL);
			head->key_len = key_len;
			head->value_len = value_len;
		}
		head->data = max_total;
	}

	if (head->key_len != key_len || head->value_len != value_len) {
		debug_log(("head.key_len=%d,head.value_len=%d, input.key_len="
					"%d,input.value_len=%d\n", head->key_len,
					head->value_len, key_len, value_len));
		return -1;
	}

	return 0;
}

int SHash::sh_extern_shm()
{
	size_t size;

	struct shm_hash_head *head = sh_get_head();

	size = head->data * (value_len + key_len + sizeof(int)) / 10;

	if (size < 8192)
		size = 8192 - 8192 % (value_len + key_len + sizeof(int));

	debug_log(("extern shm start: add size=%llu ...\n", (unsigned long long)size));

	if (shm_flag & _SHASH_MEM_FLAG) {
		old_size += size;
		if ((mem = realloc(mem, old_size)) == NULL) {
			debug_log(("extern memory: realloc error: +size=%llu, %s\n", 
					(unsigned long long)size, strerror(errno)));
			return -1;
		}
		memset((char *)mem + old_size - size, 0, size);
	} else if (shm_flag & _SHASH_FILE_FLAG) {
#ifndef WIN32
		if (munmap(mem, old_size) == -1)
			return -1;
#endif
		if (ftruncate(fd, old_size + size) == -1) {
			debug_log(("ftruncate fd=%d size=%llu: %s\n", fd,
						(unsigned long long)(old_size + size), strerror(errno)));
			return -1;
		}
#ifndef WIN32
		if ((mem = mapfile(fd, &old_size, 1)) == NULL) {
			debug_log(("extern memory: realloc error: +size=%llu, %s\n", 
					(unsigned long long)size, strerror(errno)));
			return -1;
		}
#endif
		memset((char *)mem + old_size - size, 0, size);
	} 

	if (sh_init_head(1) == -1)
		return -1;

	debug_log(("extern shm done.\n"));

	return 0;
}

int SHash::sh_destroy()
{
	if (shm_flag & _SHASH_MEM_FLAG)
		free(mem);
	else {
#ifndef WIN32
		if (munmap(mem, old_size) == -1)
			return -1;;
#endif
	}
	mem = NULL;

	return 0;
}

/*
 * return:
 * _SHASH_SYS_ERROR system error
 * _SHASH_NOT_FOUND not found
 * _SHASH_FOUND     found
 */
int SHash::sh_find(const void *key, void **value)
{
	int n = sh_sys_find(key, value);

	if (n == _SHASH_NOT_FOUND_NEXT)
		n = _SHASH_NOT_FOUND;

	return n;
}

/*
 * _SHASH_FOUND
 * _SHASH_NOT_FOUND
 * _SHASH_NOT_FOUND_NEXT
 * _SHASH_SYS_ERROR
 */
int SHash::sh_sys_find(const void *key, void **value)
{
	void *p;
	unsigned int next;
	int *hash;
	void *new_key;
	int ret_len;

	struct shm_hash_head *head = sh_get_head();

	rec = sh_get_rec(head);

	key_func(func_type, key, key_len, (void **)&new_key, &ret_len);
	
	crc32 = crc32sum((char *)new_key, ret_len) % head->hash;
	hash = sh_get_hash(head) + crc32;

	if (*hash == 0) {
		return _SHASH_NOT_FOUND;
	}

	find = p = sh_get_pos(rec, *hash);
	used = NULL;

	for (;;) {
		find = p;
		count_same++;

		if (sh_record_is_used(find)) {
			if (cmp_func(func_type, sh_get_key(p), key, key_len) == 0) {
				if (value)
					*value = sh_get_value(p);
				return _SHASH_FOUND;
			}
		} else {
			if (used == NULL)
				used = find;
		}

		if ((next = sh_get_next(p)) == 0)
			break;

		p = sh_get_pos(rec, next);
	}

	return _SHASH_NOT_FOUND_NEXT;
}

int SHash::sh_add(const void *key, const void *value, void **ret)
{
	return sh_sys_add(key, value, 0, ret);
}

int SHash::sh_replace(const void *key, const void *value, void **ret)
{
	return sh_sys_add(key, value, 1, ret);
}

/*
 * flag = 0 : add
 * flag = 1 : replace
 */
int SHash::sh_sys_add(const void *key, const void *value, int flag, void **ret)
{
	void *tmp;
	int *hash, n;
	struct shm_hash_head *head;

	count_same = 0;

	n = sh_sys_find(key, &tmp);

	if (n == _SHASH_SYS_ERROR)
		return -1;

	head = sh_get_head();
	rec = sh_get_rec(head);

	if (head->curr >= head->data) {
		if (sh_extern_shm() == -1) {
			return _SHASH_SYS_ERROR;
		}
		return sh_sys_add(key, value, flag);
	}

	switch (n) {
		case _SHASH_FOUND:
			if (flag) {
				if (value_len > 0)
					memcpy(sh_get_value(find), value, value_len);
				if (ret)
					*ret = sh_get_value(find);
				return 0;
			} else {
				while (sh_get_next(find) > 0) {
					find = sh_get_pos(rec, sh_get_next(find));
					count_same++;
				}
			}
		case _SHASH_NOT_FOUND_NEXT:

			if (count_same > head->max_same)
				head->max_same = count_same;

			head->total++;
			if (used == NULL) {
				head->curr++;
				sh_set_next(find, head->curr);
				find = sh_get_pos(rec, sh_get_next(find));
				sh_set_next(find, 0);
			} else
				find = used;

			_sh_get_next(find) |= _RECORD_USED_FLAG;

			add_func(func_type, sh_get_key(find), key, key_len);

			if (value_len > 0)
				memcpy(sh_get_value(find), value, value_len);
			if (ret)
				*ret = sh_get_value(find);
			break;

		case _SHASH_NOT_FOUND:
			head->total++;
			head->curr++;
			head->hash_used++;
			hash = sh_get_hash(head);
			hash[crc32] = head->curr;
			find = sh_get_pos(rec, hash[crc32]);

			sh_set_next(find, 0);
			_sh_get_next(find) |= _RECORD_USED_FLAG;

			add_func(func_type, sh_get_key(find), key, key_len);

			if (value_len > 0)
				memcpy(sh_get_value(find), value, value_len);
			if (ret)
				*ret = sh_get_value(find);
			break;

		default:
			return -1;
			break;
	}
	
	return 0;
}

int SHash::sh_delete(const void *key)
{
	return sh_sys_delete(key, 0);
}

int SHash::sh_delete_all(const void *key)
{
	return sh_sys_delete(key, 1);
}

/*
 * flag = 0 : delete 1
 * flag = 1 : delete all
 */
int SHash::sh_sys_delete(const void *key, int flag)
{
	void *value;
	int total = 0, n = 0;
	struct shm_hash_head *head = sh_get_head();

	rec = sh_get_rec(head);

 	if ((n = sh_sys_find(key, &value)) == _SHASH_SYS_ERROR)
		return -1;

	if (n == _SHASH_NOT_FOUND || n == _SHASH_NOT_FOUND_NEXT) {
		return 0;
	}

	for (;;) {
		if (sh_record_is_used(find) && cmp_func(func_type, sh_get_key(find), key, key_len) == 0) {
			memset(sh_get_key(find), 0, key_len);
			if (value_len > 0)
				memset(sh_get_value(find), 0, value_len);
			sh_delete_a_record(find);
			head->total--;
			total++;
			if (flag == 0)
				break;
		}
		if (sh_get_next(find) == 0)
			break;
		find = sh_get_pos(rec, sh_get_next(find));
	}
	
	return total;
}

int SHash::sh_begin()
{
	find = NULL;

	return 0;
}

/*
 * return:
 * _SHASH_NOT_FOUND  not found
 * _SHASH_FOUND      found
 * _SHASH_SYS_ERROR  system error
 */
int SHash::sh_next(const void *key, void **value)
{
	if (find == NULL) 
		return sh_find(key, value);

	if (sh_get_next(find) == 0)
		return _SHASH_NOT_FOUND;

	struct shm_hash_head *head = sh_get_head();

	rec = sh_get_rec(head);

	find = sh_get_pos(rec, sh_get_next(find));

	for (;;) {
		if (sh_record_is_used(find) && cmp_func(func_type, sh_get_key(find), key, key_len) == 0) {
			if (value)
				*value = sh_get_value(find);
			return _SHASH_FOUND;
		}
		if (sh_get_next(find) == 0)
			break;
		find = sh_get_pos(rec, sh_get_next(find));
	}

	return _SHASH_NOT_FOUND;
}

int SHash::sh_fd_reset()
{
	read_index = 0;

	return 0;
}

int SHash::sh_fd_read_next(void *key, void *value)
{
	char buf[1024];
	struct shm_hash_head *head = (struct shm_hash_head *)m_head;
	off_t offset;

	for (;;) {
		read_index++;

		if (read_index > head->curr)
			return _SHASH_NOT_FOUND;

		offset = m_pos + sizeof(struct shm_hash_head)
				+ head->hash * sizeof(int) 
				+ (read_index - 1) * (head->key_len + head->value_len + sizeof(int));
		pread(fd, buf, head->key_len + head->value_len + sizeof(int), offset);

		if (sh_record_is_used(buf)) {
			memcpy(key, sh_get_key(buf), head->key_len);
			memcpy(value, sh_get_value(buf), head->value_len);
			break;
		}
	}

	return _SHASH_FOUND;
}

/*
 * sh_reset() use with sh_read_next() 
 */
int SHash::sh_reset()
{
	read_index = 0;

	return 0;
}

/*
 * return:
 * _SHASH_NOT_FOUND  not found
 * _SHASH_FOUND      found
 * _SHASH_SYS_ERROR  system found
 */
int SHash::sh_read_next(void *key, void *value)
{
	int n;
	void *v;

	if ((n = sh_read_next(key, &v)) == _SHASH_FOUND) {
		if (value_len > 0 && value != NULL) {
			memcpy(value, v, value_len);
		}
	}

	return n;
}

int SHash::sh_read_next(void *key, void **value)
{
	void *next;
	struct shm_hash_head *head;
	
	head = sh_get_head();
	_sh_check_head(head, _SHASH_SYS_ERROR);
	rec = sh_get_rec(head);

	for (;;) {
		read_index++;
		if (read_index > head->curr)
			return _SHASH_NOT_FOUND;
		next = sh_get_pos(rec, read_index);
		if (sh_record_is_used(next)) {
			memcpy(key, sh_get_key(next), key_len);
			if (value_len > 0)
				*value = sh_get_value(next);
			break;
		}
	}

	return _SHASH_FOUND;
}

int SHash::sh_delete(const void *key, const void *value)
{
	int total = 0;
	void *p, *real_key;
	void *tmp_find = find;
	struct shm_hash_head *head = sh_get_head();

	sh_begin();

	while (sh_next(key, &p) == _SHASH_FOUND) {
		if (cmp_func(func_type, value, p, value_len) == 0) {
			real_key = sh_get_key_from_value(p);
			sh_delete_a_record(real_key);
			head->total--;
			total++;
		}
	}

	find = tmp_find;

	return total;
}

int SHash::sh_get_key_by_value(const void *value, void **key)
{
	*key = sh_get_key_from_value(value);

	return 0;
}

int SHash::sh_get_user_data(void **data)
{
	struct shm_hash_head *head;

	_sh_check_head(head, -1);

	(*data) = (void *)head->user_data;

	return 0;
}

int SHash::sh_hash_total()
{
	struct shm_hash_head *head;

	_sh_check_head(head, -1);

	return head->total;
}

int SHash::sh_clean()
{
	struct shm_hash_head *head;

	_sh_check_head(head, -1);

	head->hash_used = head->total = head->curr = 0;

	memset(sh_get_hash(head), 0, head->hash * sizeof(unsigned int));

	return 0;
}

int SHash::sh_get_addr(void **addr, int *size)
{
	*addr = (void *)sh_get_head();
	*size = (int)sh_get_size();

	return 0;
}

int SHash::sh_random(void *key, void *value)
{
	void *p;
	struct shm_hash_head *head = sh_get_head();

	rec = sh_get_rec(head);

	for (int i = 0; i < 256; i++) {
		int r = rand() % head->curr;
		p = sh_get_pos(rec, r);
		if (sh_record_is_used(p)) {
			memcpy(key, sh_get_key(p), key_len);
			memcpy(value, sh_get_value(p), value_len);
			return 1;
		}
	}

	return 0;
}

