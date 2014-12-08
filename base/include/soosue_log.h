#ifndef __SOOSUE_LOG_H
#define __SOOSUE_LOG_H

#include <stdarg.h>

extern "C" {


enum {
	LOG_MIN = 0,
	LOG_MAX,
};

class SoosueLog {
	private:
		char *m_buf;
		int m_len;
		int m_fd;

	public:
		SoosueLog(int flag);
		~SoosueLog();

	public:
		int log_init(const char *dir, const char *prefix);
		int log_time(const char *file, int line, char *buf, int size);
		int log_write(const char *file, int line, const char *fmt, va_list ap);
};

#define LOG(...) \
	do { \
		LOG_SYS(__FILE__, __LINE__, __VA_ARGS__);  \
	} while (0)

int LOG_INIT(int flag, const char *dir, const char *prefix);
int LOG_SYS(const char *file, int line, const char *format, ...);
int LOG_RELEASE();

};

#endif
