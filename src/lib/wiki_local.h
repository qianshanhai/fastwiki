/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_LOCAL
#define __WIKI_LOCAL

struct local {
	const char *flag;
	const char *s;
};

struct lang {
	const char *name;
	struct local *l;
};

#ifdef __WIKI_LOCAL_CPP
#include "wiki_local_auto.h"
struct lang all_lang[] = { ALL_LANG };
#else
extern struct lang all_lang[];
#endif


#ifdef __cplusplus
extern "C" {
#endif

const char *local_msg(const char *name);
char *get_month_day(char *key);
char *get_month_day_from_lang(char *key, const char *lang);

void set_lang(const char *l);
char *get_lang();

#define _(x) local_msg(x)

#ifdef __cplusplus
};
#endif

#endif
