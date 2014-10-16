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
#include "wiki_image.h"

#define MAX_ONE_IMAGE_FILE (5*1024*1024)

struct fi_option {
	char date[32];
	char lang[32];
	char dir[128];
	char file_type[128];
	split_t sp;
	char buf[MAX_ONE_IMAGE_FILE];;
};

struct fi_option *m_option = NULL;
WikiImage *m_wiki_image = NULL;

void usage(const char *name)
{
	print_usage_head();

	printf("Usage: fastwiki-image <-d date> <-l lang> <-r dir> <-t file type> \n");
	printf( "       -d date \n"
			"       -l lang \n"
			"       -r image folder \n"
			"       -t file type, such as .jpg, use ',' to split more type.\n"
			"          for example: .git,.jpg  etc.\n"
			".\n");
	exit(0);
}

int fi_init_option(int argc, char *argv[])
{
	int opt;
	struct fi_option *p = m_option;

	memset(p, 0, sizeof(p));

	while ((opt = getopt(argc, argv, "d:l:r:t:")) != -1) {
		switch (opt) {
			case 'd':
				strncpy(p->date, optarg, sizeof(p->date) - 1);
				break;
			case 'l':
				strncpy(p->lang, optarg, sizeof(p->lang) - 1);
				break;
			case 'r':
				strncpy(p->dir, optarg, sizeof(p->dir) - 1);
				break;
			case 't':
				strncpy(p->file_type, optarg, sizeof(p->file_type) - 1);
				break;
			default:
				break;
		}
	}

	if (p->date[0] == 0 || p->lang[0] == 0 || p->dir[0] == 0 || p->file_type[0] == 0)
		usage(argv[0]);

	split(',', p->file_type, p->sp);

	for_each_split(p->sp, T) {
		trim(T);
	}

	return 0;
}

/*
 * below fa_* function copy from fastwiki-audio
 */

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

int fi_add_one_file(const char *file)
{
	int len;
	char *p = (char *)file, buf[256];
	
	if ((p = (char *)strrchr(file, '/')))
		p++;
	else
		p = (char *)file;

	strcpy(buf, p);
	format_image_name(buf, sizeof(buf));

	if ((len = q_read_file(file, m_option->buf, MAX_ONE_IMAGE_FILE)) > 0) {
		m_wiki_image->we_add_one_image(buf, m_option->buf, len);
	}

	return 0;
}

int fi_create_image()
{
	char buf[1024];

	m_wiki_image = new WikiImage();
	m_wiki_image->we_init(m_option->lang, m_option->date);

	fa_scan_dir(m_option->dir, m_option->sp, fi_add_one_file);

	m_wiki_image->we_add_done();
	m_wiki_image->we_output(buf);

	printf("%s\n", buf);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		usage(argv[0]);

	m_option = (struct fi_option *)malloc(sizeof(struct fi_option));

	fi_init_option(argc, argv);

	fi_create_image();

	return 0;
}
