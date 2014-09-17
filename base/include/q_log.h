#ifndef __FASTWIKI_Q_LOG_H
#define __FASTWIKI_Q_LOG_H

#include <stdio.h>
#include <stdarg.h>

inline int log_sys_format(char *buf, int size, const char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(buf, size, fmt, ap);
	va_end(ap);

	return len;
}

#define LOG(...) \
	do { \
		int __len; \
		char __buf[2048]; \
		__len = log_sys_format(__buf, sizeof(__buf), __VA_ARGS__);  \
		__buf[__len] = 0; \
		printf("%s: %d: %s", __FILE__, __LINE__, __buf); \
		fflush(stdout); \
	} while (0)

#endif
