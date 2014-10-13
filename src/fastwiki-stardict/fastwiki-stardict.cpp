/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <stdlib.h>

#include "q_util.h"
#include "wiki_common.h"

#include "stardict.h"

static struct fw_stardict_st m_dict_option;

void usage(const char *name)
{
	print_usage_head();
	printf("Usage: fastwiki-stardict <-l lang> <-d date> <-i idx> <-t dict> [-s lua] [-m max_total]\n");
	printf( "       -l lang\n"
			"       -d date\n"
			"       -i stardict idx file\n"
			"       -t stardict dict file\n"
			"       -s lua script file\n"
			"       -m max total. this is debug option, just output article total for -m \n"
			".\n");

	exit(0);
}

#define getopt_str(x, buf) \
	case x: strncpy(buf, optarg, sizeof(buf) - 1); break

#define getopt_int(x, buf) \
	case x: buf = atoi(optarg); break

static int init_option(struct fw_stardict_st *st, int argc, char **argv)
{
	int opt;

	memset(st, 0, sizeof(*st));

	if (argc == 1)
		usage(argv[0]);

	while ((opt = getopt(argc, argv, "l:d:i:t:s:m:c:")) != -1) {
		switch (opt) {
			getopt_str('l', st->lang);
			getopt_str('d', st->date);
			getopt_str('i', st->idx);
			getopt_str('t', st->dict);
			getopt_str('s', st->script_file);
			getopt_str('c', st->compress);
			getopt_int('m', st->max_total);
		}
	}

	if (st->lang[0] == 0 || st->date[0] == 0 || st->idx[0] == 0 || st->dict[0] == 0)
		usage(argv[0]);

	if (st->compress[0] == 0)
		strcpy(st->compress, "gzip");

	if (!dashf(st->script_file)) {
		printf("Not found %s\n", st->script_file);
		usage(argv[0]);
	}

	if (!dashf(st->idx)) {
		printf("%s: %s\n", st->idx, strerror(errno));
	}

	if (!dashf(st->dict)) {
		printf("%s: %s\n", st->dict, strerror(errno));
	}

	return 0;
}

int main(int argc, char *argv[])
{
	init_option(&m_dict_option, argc, argv);

	convert_dict(&m_dict_option);

	return 0;
}
