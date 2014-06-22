/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef __LIBMM_Q_UTIL_H
#define __LIBMM_Q_UTIL_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef WIN32
#include <sys/mman.h>
#endif

#include <fcntl.h>

#define _q_min(x, y) ((x) > (y) ? (y) : (x))
#define _q_max(x, y) ((x) > (y) ? (x) : (y))

class split_t {
public:
	char *sp[128];
	char buf[8192];
	int total;
	int I, J, K;

	inline char *operator[](int i) const {
		return sp[i];
	}
	inline int len(int i) {
		return strlen(sp[i]);
	}
};

inline int split_total(split_t &p)
{
	return p.total;
}

inline char **split_value(split_t &p)
{
	return p.sp;
}

inline char *split_value(split_t &p, int i)
{
	return i >= 0 && i < split_total(p) ? split_value(p)[i] : NULL;
}

#define for_each_split(p, buf) \
	p.I = 0; \
	for (char *buf = NULL; p.I < split_total(p) \
			&& (buf = split_value(p, p.I)); p.I++)

int dashf(const char *fname);
int dashd(const char *fname);

off_t file_size(const char *fname);
off_t file_size(int fd);

char **split(char c, const char *s, int *len);
int split(char c, const char *s, split_t &p);
int split(char c, const char *s, char *buf, int buf_len, char **sp, int sp_len = 0);

int split_two(const char *sub, const char *s, char *buf, int buf_len, char **sp, int sp_len);

int file_append(const char *file, const void *data, int size, int flag = 0);
int safe_file_append(const char *file, const void *data, int size);

int touch(const char *file, int mode);

#ifndef WIN32

typedef struct {
	void *start;
	size_t size;
} mapfile_t;

#define q_mmap mapfile

void *mapfile(const char *file, size_t *size, int flag = 0);
void *mapfile(const char *file, mapfile_t *t, int flag = 0);

void *mapfile(int fd, size_t *size, int flag = 0);
void *mapfile(int fd, mapfile_t *t, int flag = 0);

int q_munmap(mapfile_t *t);

#endif

char *rtrim(char *p, int len);
char *ltrim(char *p);
char *trim(char *p);
char *trim(char *p, int len);
char *chomp(char *p);

void (*q_signal(int sig, void (*func)(int)))(int);

int start_daemon();

long long q_atoll(const char *buf);

int q_mkdir(const char *dir, int mode);

char *q_tolower(char *buf);
char *q_toupper(char *buf);

int q_get_cpu_total();
char *my_strcasestr(const char *s, const char *sub, int max_len);

int q_sleep(int sec);
int q_msleep(int msec);

#ifdef WIN32
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
#endif

#endif
