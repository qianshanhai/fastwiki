/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifdef _STARDICT_PERL
#include <sys/time.h>
#include <setjmp.h>
#include <stdlib.h>

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include "q_util.h"

static PerlInterpreter *my_perl;
static char m_script[4096];
static char m_content_script[4096];

#ifndef __FASTWIKI_PERL_TEST__
extern int fastwiki_stardict_find_title(const char *title, int len);
extern char *fastwiki_stardict_curr_title(int *len);
extern char *fastwiki_stardict_curr_content(int *len);
extern int dict_read_file(const char *file, char *buf, int size);
#else
static char *curr_title = NULL;
static int curr_title_len = 0;
static char *curr_content = NULL;
static int curr_content_len = 0;

int dict_read_file(const char *file, char *buf, int size)
{
	int fd;

#ifdef _WIN32
	if ((fd = open(file, O_RDONLY | O_BINARY)) == -1)
		return -1;
#else
	if ((fd = open(file, O_RDONLY)) == -1)
		return -1;
#endif

	read(fd, buf, size);
	close(fd);

	return 0;
}

int fastwiki_stardict_find_title(const char *title, int len)
{
	return 0;
}

char *fastwiki_stardict_curr_title(int *len)
{
	*len = curr_title_len;

	return curr_title;
}

char *fastwiki_stardict_curr_content(int *len)
{
	*len = curr_content_len;

	return curr_content;
}
#endif


XS(XS_find_title);
XS(XS_find_title)
{
	char *p;
	dXSARGS;
	STRLEN len;

	if (items == 1) {
		p = SvPV(ST(0), len);
		XSRETURN_IV(fastwiki_stardict_find_title(p, len));
	} else {
		XSRETURN_IV(-1);
	}
}

SV *sv_curr_title;
SV *sv_curr_content;

XS(XS_curr_title);
XS(XS_curr_title)
{
	int len;
	char *p;
	dXSARGS;

	p = fastwiki_stardict_curr_title(&len);

	ST(0) = newSVpv(p, len);
	sv_curr_title = ST(0);

	XSRETURN(1);
}

XS(XS_curr_content);
XS(XS_curr_content)
{
	int len;
	char *p;
	dXSARGS;

	p = fastwiki_stardict_curr_content(&len);

	ST(0) = newSVpv(p, len);
	sv_curr_content = ST(0);

	XSRETURN(1);
}

EXTERN_C void xs_init (pTHX);
EXTERN_C void boot_DynaLoader (pTHX_ CV* cv);

EXTERN_C void
xs_init(pTHX)
{
	const char *file = __FILE__;
	dXSUB_SYS;

	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
	newXS("find_title", XS_find_title, file);
	newXS("curr_title", XS_curr_title, file);
	newXS("curr_content", XS_curr_content, file);
}

#define CONTENT_SCRIPT  "$title = curr_title();\n$content = curr_content();\n"

int script_new()
{
	char *embedding[] = {"", "-e", "0"};

	my_perl = perl_alloc();
	perl_construct(my_perl);

	perl_parse(my_perl, xs_init, 3, embedding, NULL);
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
	perl_run(my_perl);

	return 0;
}

int script_free()
{
	perl_destruct(my_perl);
	perl_free(my_perl);

	return 0;
}

int script_init(const char *file)
{
	memset(m_script, 0, sizeof(m_script));
	memset(m_content_script, 0, sizeof(m_content_script));

	int n = file_size(file);

	dict_read_file(file, m_script, n);

	memcpy(m_content_script, CONTENT_SCRIPT, sizeof(CONTENT_SCRIPT) - 1);
	memcpy(m_content_script + sizeof(CONTENT_SCRIPT) - 1, m_script, n);

	script_new();

	return 0;
}

int perl_content(char *ret_buf)
{
	eval_pv(m_content_script, TRUE);

	SV *tmp;
	char *p;
	STRLEN len;
	tmp = get_sv("content", 0);
	p = SvPV(tmp, len);

	memcpy(ret_buf, p, len);
	ret_buf[len] = 0;

	FREETMPS; /* free vars */

	return len;
}

#ifdef __FASTWIKI_PERL_TEST__
int main(int argc, char *argv[])
{
	char buf[128], tmp[128];

	script_init(argv[1]);

	for (int i = 0; i < 1000000; i++) {
		memset(buf, 0, sizeof(buf));
		curr_title = tmp;
		curr_title_len = sprintf(tmp, "t%08d", i);
		perl_content(buf);
		printf("%s\n", buf);
	}

	return 0;
}
#endif

#endif
