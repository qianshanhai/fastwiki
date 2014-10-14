/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#include "q_util.h"
#include "q_log.h"

#include "wiki_common.h"
#include "wiki_audio.h"

#define MAX_ONE_AUDIO_FILE (10*1024*1024)

struct fa_option {
	char dir[128];
	char file_type[128];
	char outfile[128];
	split_t sp;
	int total;
	char buf[MAX_ONE_AUDIO_FILE];
};

struct fa_option *m_option = NULL;
WikiAudio *m_audio = NULL;

void usage(const char *name)
{
	print_usage_head();
	printf("usage: fastwiki-audio <-r dir> <-t file type> <-o output file>\n");
	printf( "       -r audio folder \n"
			"       -t file type, such as .wav, use ',' to split, \n"
			"          for example: .wav,.mp3 , etc.\n"
			"       -o output file name\n"
			".\n");

	exit(0);
}

int fa_init_option(int argc, char *argv[])
{
	int opt;
	struct fa_option *p = m_option;

	memset(p, 0, sizeof(*p));

	while ((opt = getopt(argc, argv, "r:t:o:")) != -1) {
		switch (opt) {
			case 'r':
				strncpy(p->dir, optarg, sizeof(p->dir) - 1);
				break;
			case 't':
				strncpy(p->file_type, optarg, sizeof(p->file_type) - 1);
				break;
			case 'o':
				strncpy(p->outfile, optarg, sizeof(p->outfile) - 1);
				break;
			default:
				break;
		}
	}

	if (p->dir[0] == 0 || p->file_type[0] == 0 || p->outfile[0] == 0)
		usage(argv[0]);

	split(',', p->file_type, p->sp);

	for_each_split(p->sp, T) {
		trim(T);
	}

	return 0;
}

int fa_match_fname(const char *fname, split_t &sp)
{
	int len = strlen(fname);

	for_each_split(sp, T) {
		int k = strlen(T);
		if (len >= k && strncasecmp(fname + len - k, T, k) == 0)
			return 1;
	}

	return 0;
}

int fa_count_total(const char *file)
{
	m_option->total++;

	return 0;
}

int fa_parse_fname(const char *file, char *name, char *type)
{
	char *p, *t, buf[256];

	strcpy(buf, file);
	if ((p = strrchr(buf, '.')) == NULL)
		return -1;

	*p++ = 0;

	if ((t = strrchr(buf, '/')))
		t++;
	else
		t= buf;

	strcpy(name, t);
	strcpy(type, p);

	return 0;
}

int fa_add_one_file(const char *file)
{
	int len;
	char title[256], type[64];

	if (fa_parse_fname(file, title, type) == -1)
		return -1;

	if ((len = q_read_file(file, m_option->buf, MAX_ONE_AUDIO_FILE)) > 0) {
		m_audio->wa_add_one_audio(title, m_option->buf, len, type);
	}

	return 0;
}

int fa_scan_dir(const char *dir, split_t &sp, int (*func)(const char *file))
{
	DIR *dirp;
	struct dirent *d;
	char file[256];

	if ((dirp = opendir(dir)) == NULL) {
		LOG("open dir %s: %s\n", dir, strerror(errno));
		return -1;
	}

	while ((d = readdir(dirp))) {
		if (d->d_name[0] == '.')
			continue;

		sprintf(file, "%s/%s", dir, d->d_name);
		if (dashd(file)) {
			fa_scan_dir(file, sp, func);
		} else {
			if (fa_match_fname(d->d_name, sp)) {
				if (func(file) == -1)
					break;
			}
		}
	}

	closedir(dirp);

	return 0;
}

int fa_create_audio()
{
	fa_scan_dir(m_option->dir, m_option->sp, fa_count_total);

	m_audio = new WikiAudio();
	m_audio->wa_init_rw(m_option->total, m_option->outfile);

	fa_scan_dir(m_option->dir, m_option->sp, fa_add_one_file);

	m_audio->wa_output();

	delete m_audio;

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) 
		usage(argv[0]);

	m_option = (struct fa_option *)malloc(sizeof(struct fa_option));

	fa_init_option(argc, argv);
	fa_create_audio();

	return 0;
}

