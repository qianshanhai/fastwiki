#ifdef WIN32

#include <sys/stat.h>
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

int chown(const char *file, int mode)
{
	return 0;
}

int kill(int pid, int sig)
{
	return 0;
}

int setgid(int i)
{
	return 0;
}

int setuid(int i)
{
	return 0;
}
int getuid(void)
{
	return 0;
}

int geteuid(void)
{
	return 0;
}

int getgid(void)
{
	return 0;
}
int getegid(void)
{
	return 0;
}

char *
win32_get_sitelib(const char *pl, int *const len)
{
	*len = 0;
	return (char *)"";
}

char *
win32_get_privlib(const char *pl, int *const len)
{
	*len = 0;

	return (char *)"";
}

void win32_str_os_error(void *sv, long dwErr)
{
	strcpy((char *)sv, "");
}

void
Perl_win32_init(int *argcp, char ***argvp)
{
}

void
Perl_win32_term(void)
{
}

int Stat(const char *path, struct stat *sbuf)
{
	return stat(path, sbuf);
}

int do_exec3(int len, ...)
{
	return 0;
}

bool Perl_do_exec(pTHX_ const char* cmd)
{
	return 0;
}

int sleep(int s)
{
	_sleep(s * 1000);
	return 0;
}

int pipe(int f[2])
{
	return -1;
}

int fork()
{
	return 0;
}

int wait(int *status)
{
	*status = 0;
	return 0;
}

int win32_get_errno(int i)
{
	return 0;
}

int ioctl(int d, int request, ...)
{
	return -1;
}

#endif
