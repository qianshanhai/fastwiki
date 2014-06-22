/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include "prime.h"
#include "wiki_stat.h"

#define _(x) #x

#define my_strncmp(x, y) strncmp(x, y, sizeof(y) - 1)
#define my_strncasecmp(x, y) strncasecmp(x, y, sizeof(y) - 1)

#define parse_not_use_title(title) \
	(my_strncasecmp(title, "Wikipedia:") == 0  || my_strncasecmp(title, "Help:") == 0 \
			|| my_strncasecmp(title, "MediaWiki:") == 0 || my_strncasecmp(title, "Portal:") == 0 \
			|| my_strncasecmp(title, "Template:") == 0 || my_strncasecmp(title, "File:") == 0 \
			|| my_strncasecmp(title, "Category:") == 0)

WikiStat::WikiStat()
{
	m_check_dup = NULL;

	m_debug = 0;
	m_buf = 0;

	m_hash = get_best_hash(170000000);
}

WikiStat::~WikiStat()
{
	if (m_check_dup)
		delete m_check_dup;

	if (m_buf)
		free(m_buf);
}

int WikiStat::st_start()
{
	int ret;

	if (st_init() == -1)
		return -1;

	for (;;) {
		ret = st_onedir(m_indir);
		if (ret == 1 || ret == -1)
			break;
		q_sleep(1);
	}

	return ret;
}

int WikiStat::st_create_file()
{
	int size = m_hash * sizeof(struct key) + 1024;

	int fd;

	if ((fd = open(m_output, O_RDWR | O_CREAT | O_TRUNC, 0644)) == -1) {
		return -1;
	}

	ftruncate(fd, size);

	close(fd);

	return 0;
}

int WikiStat::st_init(int flag)
{
	if (st_check_env() == -1)
		return -1;

	if (!dashf(m_output)) {
		st_create_file();
	}
	if (q_mmap(m_output, &m_mmap, flag) == NULL) {
		perror(m_output);
		return -1;
	}

	m_check_dup = new SHash();
	m_check_dup->sh_init(m_check_dup_file, sizeof(struct dup_key), sizeof(struct dup_value), 5*10000, flag);

	m_buf = (char *)malloc(MAX_STAT_BUF_SIZE + 1024);

	return 0;
}

int WikiStat::st_print(int total)
{
	int all = 0, count = 0;
	struct key *p = (struct key *)m_mmap.start;

	for (unsigned int i = 0; i < m_hash; i++) {
		if (p[i].total > 0) {
			if (p[i].total > (unsigned short)total) {
				count++;
			}
			all++;
		}
	}

	printf("all:%d, count:%d\n", all, count);

	return 0;
}

#define my_getenv(v, env) \
	do { \
		if ((p = getenv(env)) == NULL) { \
			printf("export %s=? \n", env); \
			return -1; \
		} \
		strcpy(v, p); \
	} while (0)


int WikiStat::st_check_env()
{
	char *p;

	if ((p = getenv("WIKI_STAT_DEBUG")) && atoi(p) == 1) {
		m_debug = 1;
	}

	my_getenv(m_output, _(WIKI_STAT_OUTPUT));
	my_getenv(m_indir,  _(WIKI_STAT_INDIR));
	my_getenv(m_check_dup_file,  _(WIKI_STAT_CHECK_DUP));

	if (!dashd(m_indir))
		return -1;

	return 0;
}

int WikiStat::st_convert_title(const char *title, char *buf)
{
	ch2hex(title, (unsigned char *)buf);

	return 0;
}

int WikiStat::st_convert_title2(char *buf)
{
	char tmp[4096];

	if (strcmp(buf, "'") == 0 || strcmp(buf, "_") == 0
			|| strcmp(buf, "\"") == 0 || strcmp(buf, " ") == 0)
		return 0;

	strcpy(tmp, buf);

	int len = 0;

	for (int j = 0; tmp[j]; j++) {
		if (tmp[j] != '"' && tmp[j] != ' ' && tmp[j] != '_' && tmp[j] != '\'') {
			buf[len++] = tmp[j];
		}
	}

	buf[len] = 0;

	q_tolower(buf);

	return 0;
}

int WikiStat::st_check_title(char *buf)
{
	char *p, tmp[2048];

	if (strstr(buf, "/upload"))
		return 0;

	strcpy(tmp, buf);

	if ((p = strrchr(tmp, '/'))) {
		*p++ = 0;
		strcpy(buf, p);
	}

	return 1;
}

int WikiStat::st_add_hash(const char *buf, int len, int total)
{
	struct key *p;
	unsigned int crc32, r_crc32;
	int tmp;

	crc32sum(buf, len, &crc32, &r_crc32);

	p = (struct key *)m_mmap.start;
	p = &p[crc32 % m_hash];

	tmp = p->total;
	if (tmp >= 65534)
		return 0;

	if (tmp + total >= 65534)
		p->total = 65534;
	else
		p->total += (unsigned short)total;

	return 0;
}

int WikiStat::st_one_record(const char *title, int total)
{
	char buf[4096];

	st_convert_title(title, buf);
	st_convert_title2(buf);

	if (buf[0] == 0)
		return 0;

	if (parse_not_use_title(buf))
		return 0;

	if (st_check_title(buf) == 0)
		return 0;


	if (m_debug == 1) {
		printf("%s\n", buf);
	} else {
		st_add_hash(buf, strlen(buf), total);
	}

	return 0;
}

int WikiStat::st_onefile(const char *file)
{
	char buf[4096];
	split_t sp;

	FileIO *io = new FileIO();

	io->fi_init(file, m_buf, MAX_STAT_BUF_SIZE);

	while (io->fi_gets(buf, sizeof(buf))) {
		split(' ', buf, sp);
		if (strcmp(sp[0], "en") == 0) {
			st_one_record(sp[1], atoi(sp[2]));
		}
	}

	delete io;

	return 0;
}

int WikiStat::st_check_dup(const char *fname)
{
	struct dup_key k;
	struct dup_value v, *f;

	memset(&k, 0, sizeof(k));

	strncpy(k.fname, fname, sizeof(k.fname) - 1);

	if (m_check_dup->sh_find(&k, (void **)&f) == _SHASH_FOUND)
		return 1;

	v.value = 1;
	m_check_dup->sh_add(&k, &v);

	return 0;
}

int WikiStat::st_onedir(const char *dir)
{
	DIR *dirp;
	struct dirent *d;
	char file[256];
	int ret = 0;

	if ((dirp = opendir(dir)) == NULL)
		return -1;

	while ((d = readdir(dirp))) {
		sprintf(file, "%s/%s", dir, d->d_name);
		if (my_strncmp(d->d_name, "pagecounts") == 0) {
			if (st_check_dup(d->d_name) == 0) {
				st_onefile(file);
			}
			unlink(file);
		} else if (strcmp(d->d_name, "exit") == 0) {
			ret = 1;
		}
	}

	closedir(dirp);

	return ret;
}

struct find_key {
	int idx;
	unsigned short total;
	unsigned short r;
};

int cmp_st(const void *a, const void *b)
{
	return 0;
}

int WikiStat::st_convert_file(const char *file, const char *output)
{
	mapfile_t mt;

	if (q_mmap(file, &mt) == NULL) {
		return -1;
	}

	struct key *start = (struct key *)mt.start;

	struct find_key *buf = (struct find_key *)calloc(m_hash + 1024, sizeof(struct find_key));
	char *out = (char *)malloc(m_hash / 2 + 10);

	for (int i = 0; i < m_hash; i++) {
		buf[i].idx = i;
		buf[i].total = start[i].total;
	}
	qsort(buf, m_hash, sizeof(struct find_key), cmp_st);

	int hash[] = {
		0, 5, 10, 15, 20,
		25, 30, 35, 40, 45,
		50, 60, 70, 80, 90,
		101,
	};

	for (int i = 0; i < (int)m_hash; i++) {
		int rate = (int)((float)i  * 1.0 / (m_hash / 100.0));
		int find_idx = 0;
		for (int j = 0; j < 16; j++) {
			if (rate < hash[j]) {
				find_idx = 15 - j;
				break;
			}
		}
		unsigned char ch = find_idx;
	}
}
