#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

#include "soosue_log.h"

static SoosueLog *m_log = NULL;

int LOG_INIT(int flag, const char *dir, const char *prefix)
{
	m_log = new SoosueLog(flag);
	if (m_log->log_init(dir, prefix) == -1) {
		delete m_log;
		m_log = NULL;
		return -1;
	}

	return 0;
}

int LOG_SYS(const char *file, int line, const char *format, ...)
{
	int n = 0;
	va_list ap;

	va_start(ap, format);

	if (m_log == NULL) {
		m_log = new SoosueLog(LOG_MAX);
	}

	n = m_log->log_write(file, line, format, ap);

	va_end(ap);

	return n;
}

int LOG_RELEASE()
{
	if (m_log) {
		delete m_log;
		m_log = NULL;
	}

	return 0;
}

SoosueLog::SoosueLog(int flag)
{
	m_fd = STDERR_FILENO;
	m_len = (flag == LOG_MIN) ? (10*1024) : (100*1024);

	m_buf = (char *)malloc(m_len);
}

SoosueLog::~SoosueLog()
{
	free(m_buf);
}

int SoosueLog::log_init(const char *dir, const char *prefix)
{
	char file[256];

	snprintf(file, sizeof(file), "%s/%s", dir, prefix);

	if ((m_fd = open(file, O_RDWR | O_APPEND | O_CREAT, 0644)) == -1) {
		fprintf(stderr, "open file [%s] to write error: %s\n", file, strerror(errno));
		return -1;
	}

	return 0;
}

int SoosueLog::log_time(const char *file, int line, char *buf, int size)
{
	int len;
	struct tm *tm;
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	
	tm = localtime(&tv.tv_sec);

	len = snprintf(buf, size, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s:%d ",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000), file, line);

	return len;
}

int SoosueLog::log_write(const char *file, int line, const char *fmt, va_list ap)
{
	int len;

	len = log_time(file, line, m_buf, m_len);

	len += vsnprintf(m_buf + len, m_len - len, fmt, ap);

	write(m_fd, m_buf, len);
	fsync(m_fd);

	return len;
}
