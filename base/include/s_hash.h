/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef __GENERAL_SHASH_H
#define __GENERAL_SHASH_H

#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

typedef int (*log_func_t)(const char *fmt, ...);

#define SHM_CMP_ARG(type, old_key, key, len) void *type, const void *old_key, const void *key, size_t len
#define SHM_ADD_ARG(type, key, new_key, len) void *type, void *key, const void *new_key, size_t len
#define SHM_KEY_ARG(type, key, len, new_key, ret_len) \
	void *type, const void *key, int len, void **new_key, int *ret_len

#define SHM_READ_ARG(type, key, value) void *type, const void *key, const void *value

typedef int (*shm_cmp_func_t)(SHM_CMP_ARG(type, old_key, key, len));
typedef int (*shm_add_func_t)(SHM_ADD_ARG(type, key, new_key, len));
typedef int (*shm_key_func_t)(SHM_KEY_ARG(type, key, len, new_key, ret_len));
typedef int (*shm_read_func_t)(SHM_READ_ARG(type, key, value));

#define _SHASH_FOUND 0
#define _SHASH_NOT_FOUND 1
#define _SHASH_SYS_ERROR -1

#define SHASH_USER_DATA_LEN 1984

#define SHASH_FAST 0x100

class SHash {

private:
	int value_len;
	int key_len;
	int hash_magic;

	int count_same;

	void *rec, *find, *used;
	unsigned int crc32;

	void *m_head;
	unsigned int m_pos;

	int fd;
	int file_lock_flag;

	void *mem;
	size_t old_size;
	int shm_flag;
	log_func_t log;
	float hash_rate;

	shm_cmp_func_t cmp_func;
	shm_add_func_t add_func;
	shm_key_func_t key_func;
	void *func_type;

	int read_index;

private:
	void sh_var_init();

	int sh_init_head(int flag);
	int sh_extern_shm();
	int sh_sys_find(const void *key, void **value);
	int sh_sys_add(const void *key, const void *value, int flag, void **ret = NULL);
	int sh_sys_delete(const void *key, int flag);
	int sh_check_key_value_len(int total);

public:
	SHash();
	SHash(int flag);
	~SHash();

	int sh_init(const char *file, int _k_len, int _v_len,
				int total = 0, int write_flag = 0);

	int sh_init_addr(void *addr, int size, int k_len, int v_len);

	int sh_init(int total, int _k_len, int _v_len, int mode = 0);

	int sh_fd_init(int fd, int total, int _k_len, int _v_len, int write_flag);


	void sh_set_func(void *type, shm_cmp_func_t _cmp,
				shm_key_func_t _key, shm_add_func_t _add = NULL);
	void sh_set_log_func(log_func_t lt = NULL);

	int sh_find(const void *key, void **value = NULL);

	int sh_begin();
	int sh_next(const void *key, void **value);

	int sh_add(const void *key, const void *value, void **ret = NULL);
	int sh_replace(const void *key, const void *value, void **ret = NULL);

	int sh_delete(const void *key, const void *value);
	int sh_delete(const void *key);
	int sh_delete_all(const void *key);

	int sh_reset();
	int sh_read_next(void *key, void *value);
	int sh_read_next(void *key, void **value);

	int sh_get_key_by_value(const void *value, void **key);
	int sh_get_user_data(void **data);
	int sh_hash_total();

	int sh_clean();
	int sh_destroy();

	void sh_set_hash_magic(int magic);
	int sh_get_addr(void **addr, int *size);

	int sh_fd_init_ro(int _fd, unsigned int pos);
	int sh_fd_find(const void *key, void *value);
		
	int sh_fd_reset();
	int sh_fd_read_next(void *key, void *value);

};

#endif
