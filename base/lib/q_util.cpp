/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#include "q_util.h"

/*
 * check fname is or not file
 */
int dashf(const char *fname)
{
	struct stat st;

	return (stat(fname, &st) == 0 && S_ISREG(st.st_mode));
}

int dashl(const char *fname)
{
	struct stat st;

	return (lstat(fname, &st) == 0 && S_ISLNK(st.st_mode));
}

off_t file_size(const char *fname)
{
	struct stat st;

	return stat(fname, &st) == -1 ? -1 : st.st_size;
}

off_t file_size(int fd)
{
	struct stat st;

	return fstat(fd, &st) == -1 ? -1 : st.st_size;
}

/*
 * check fname is or not folder
 */
int dashd(const char *fname)
{
	struct stat st;

	return (stat(fname, &st) == 0 && S_ISDIR(st.st_mode));
}

/*
 * split s with c
 */
char **split(char c, const char *s, int *len)
{
	static char buf[BUFSIZ];
	static char *sp[128];

	*len = split(c, s, buf, sizeof(buf), sp, 128);

	return sp;
}

int split(char c, const char *s, split_t &p)
{
	p.total = split(c, s, p.buf, sizeof(p.buf),
			p.sp, sizeof(p.sp) / sizeof(char *));

	return p.total;
}

/*
 * buf is temp buffer
 * buf_len is sizeof buf 
 */
int split(char c, const char *s, char *buf, int buf_len, char **sp, int sp_len)
{
	char *m = buf, *t = (char *)s;
	int y = 0;

	sp[y] = buf;

	for (; (*m = *t) && (t - s) < buf_len; t++, m++) {
		if (*t == c) {
			*m = 0;
			sp[++y] = m + 1;
			if (sp_len > 0 && y >= sp_len)
				return y;
		}
	}
	*m = 0;
	y++;

	return y;
}

int split_two(const char *sub, const char *s, char *buf, int buf_len, char **sp, int sp_len)
{
	char *m = buf, *t = (char *)s;
	int y = 0;
	int sub_len = strlen(sub);

	sp[y] = buf;

	for (; (*m = *t) && (t - s) < buf_len; t++, m++) {
		if ((t[0] == sub[0] && t[1] == sub[1]) || (sub_len == 4 && t[0] == sub[2] && t[1] == sub[3])) {
			*m = 0;
			sp[++y] = m + 1;
			t++;
			if (sp_len > 0 && y >= sp_len)
				return y;
		}
	}
	*m = 0;
	y++;

	return y;
}

int file_append(const char *file, const void *data, int size, int flag)
{
	int fd, ret = 0;

	if ((fd = open(file, O_RDWR | O_APPEND | O_CREAT, 0644)) == -1)
		return -1;

	if (write(fd, data, size) != size)
		ret = -1;

	if (flag == 1) {
#ifndef WIN32
		if (fsync(fd) == -1)
			return -1;
#endif
	}

	close(fd);

	return ret;
}

int touch(const char *file, int mode)
{
	int fd;

	if ((fd = open(file, O_RDWR | O_APPEND | O_CREAT, mode)) == -1)
		return -1;

	close(fd);

	return 0;
}

#ifndef WIN32

void *mapfile(const char *file, mapfile_t *t, int flag)
{
	return (t->start = mapfile(file, &t->size, flag));
}

void *mapfile(int fd, mapfile_t *t, int flag)
{
	return (t->start = mapfile(fd, &t->size, flag));
}

void *mapfile(const char *file, size_t *size, int flag)
{
	int fd;
	void *p;

	if ((fd = open(file, O_RDONLY | (flag ? O_RDWR : 0))) == -1)
		return NULL;

	p = mapfile(fd, size, flag);

	close(fd);

	return p;
}

void *mapfile(int fd, size_t *size, int flag)
{
	void *p;
	struct stat st;
	int mode = PROT_READ;

	if (fstat(fd, &st) == -1)
		return NULL;

	*size = st.st_size;
	
	if (flag)
		mode |= PROT_WRITE;

	p = mmap(NULL, st.st_size, mode, MAP_SHARED, fd, 0);

	return (p == (void *)-1 ? NULL : p);
}

int q_munmap(mapfile_t *t)
{
	return munmap(t->start, t->size);
}

#endif

char *rtrim(char *p, int len)
{
	for (len--; len >= 0; len--) {
		if (p[len] == '\r' || p[len] == '\n' 
			 	|| p[len] == ' ' || p[len] == '\t')
			p[len] = 0;
		else
			break;
	}

	return p;
}

char *ltrim(char *p)
{
	char *s = p;

	for (; *p == ' ' || *p == '\t'; p++);

	strcpy(s, p);

	return s;
}

char *trim(char *p)
{
	return ltrim(rtrim(p, strlen(p)));
}

char *trim(char *p, int len)
{
	return ltrim(rtrim(p, len));
}

char *chomp(char *p)
{
	int len = strlen(p);

	while (len-- > 0) {
		if (p[len] == '\n' || p[len] == '\r')
			p[len] = 0;
	}

	return p;
}

#ifndef WIN32

void (*q_signal(int sig, void (*func)(int)))(int)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (sig == SIGALRM) {
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
#endif
	} else {
#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;
#endif
	}
	if (sigaction(sig, &act, &oact) == -1)
		return SIG_ERR;
	
	return oact.sa_handler;
}

int start_daemon()
{
	int n, pid;

	if((pid = fork()) == -1)
		return -1;

	if(pid)
		exit(0);

	if(setsid() == -1)
		return -1;

	if((pid = fork()) == -1)
		return -1;

	if(pid)
		exit(0);

	/* close standard input,output,error IO */
	for(n = 0; n <= 2; n++)
		close(n);

	q_signal(SIGQUIT, SIG_IGN);
	q_signal(SIGTERM, SIG_IGN);

	return 0;
}

#endif

#ifdef WIN32
#define __mkdir(dir, mode) mkdir(dir)
#else
#define __mkdir(dir, mode) mkdir(dir, mode)
#endif

int q_mkdir(const char *dir, int mode)
{
	int len, n = 0;
	char *sp[256], buf[1024], tmp[1024];

	len = split('/', dir, buf, sizeof(buf), sp, sizeof(sp) / sizeof(char *));

	if (len == 1) {
		if (!dashd(dir))
			return __mkdir(dir, mode);
		return 0;
	}

	n = snprintf(tmp, sizeof(tmp), "%s", sp[0]);

	for (int i = 1; i < len; i++) {
		if (sp[i][0] == 0)
			continue;
		n += snprintf(tmp + n, sizeof(tmp) - n, "/%s", sp[i]);
		if (!dashd(tmp)) {
			if (__mkdir(tmp, mode) == -1)
				return -1;
		}
	}

	return 0;
}

char *q_tolower(char *buf)
{
	char *p = buf;

	for (; *p; p++) {
		if (*p >= 'A' && *p <= 'Z')
			*p += 'a' - 'A';
	}

	return buf;
}

char *q_toupper(char *buf)
{
	char *p = buf;

	for (; *p; p++) {
		if (*p >= 'a' && *p <= 'z')
			*p -= 'a' - 'A';
	}

	return buf;
}

#ifdef WIN32

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>

int q_get_cpu_total()
{
	SYSTEM_INFO si;
	typedef void (WINAPI *func_t)(LPSYSTEM_INFO);

	func_t func = (func_t)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetNativeSystemInfo");

	if (func) {
		func(&si);
	} else {
		GetSystemInfo(&si);
	}

	return (int)si.dwNumberOfProcessors;
}

#else

int q_get_cpu_total()
{
	FILE  *fp;
	char buf[1024];
	int total = 0;

	if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
		return 1;

	while (fgets(buf, sizeof(buf), fp)) {
		if (strstr(buf, "processor"))
			total++;
	}

	fclose(fp);

	return total == 0 ? 1 : total;
}

#endif

char *my_strcasestr(const char *s, const char *sub, int max_len)
{
	char *p, buf[2048], tmp[2048], *ret = NULL;

	max_len = sizeof(buf);

	for (int i = 0; s[i] && i < max_len; i++) {
		if (s[i] >= 'A' && s[i] <= 'Z') {
			buf[i] = s[i] + 'a' - 'A';
		} else 
			buf[i] = s[i];
	}

	for (int i = 0; sub[i] && i < max_len; i++) {
		if (sub[i] >= 'A' && sub[i] <= 'Z') {
			tmp[i] = sub[i] + 'a' - 'A';
		} else 
			tmp[i] = sub[i];
	}

	buf[max_len - 1] = tmp[max_len - 1] = 0;

	if ((p = strstr(buf, tmp))) {
		ret = (char *)(s + (p - buf));
	}

	return ret;
}

int q_sleep(int sec)
{
#ifdef WIN32
	_sleep(sec  * 1000);
#else
	sleep(sec);
#endif

	return sec;
}

int q_msleep(int msec)
{
#ifdef WIN32
	_sleep(msec);
#else
	usleep(msec * 1000);
#endif
	return msec;
}

#ifdef WIN32

ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
	OVERLAPPED o = {0};
	HANDLE fh = (HANDLE)_get_osfhandle(fd);
	uint64_t off = offset;
	DWORD bytes;
	BOOL ret;

	if (fh == INVALID_HANDLE_VALUE) {
		errno = EBADF;
		return -1;
	}

	o.Offset = off & 0xffffffff;
	o.OffsetHigh = (off >> 32) & 0xffffffff;

	ret = ReadFile(fh, buf, (DWORD)count, &bytes, &o);
	if (!ret) {
		errno = EIO;
		return -1;
	}

	return (ssize_t)bytes;
}

#endif
