#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "wiki_index.h"
#include "wiki_data.h"

static WikiConfig *m_wiki_config = NULL;

static WikiIndex *m_wiki_index = NULL;
static WikiData *m_wiki_data = NULL;
static struct file_st *m_file;

static char *m_buf;

#define MAX_DATA_BUF (2*1024*1024)

int find_init()
{
	m_wiki_data = new WikiData();
	if (m_wiki_data->wd_init(m_file->data_file, m_file->data_total) == -1) {
		printf("init data error: %s\n", m_file->data_file[0]);
		return -1;
	}

	m_wiki_index = new WikiIndex();
	if (m_wiki_index->wi_init(m_file->index_file) == -1)
		return -1;

	return 0;
}

int find_data(const sort_idx_t *idx)
{
	int len;

	printf("idx=%d, pos=%d, data_len=%d\n", idx->data_file_idx, idx->data_pos, idx->data_len);
	fflush(stdout);

	if ((len = m_wiki_data->wd_sys_read(idx->data_file_idx, idx->data_pos, idx->data_len,
					m_buf, MAX_DATA_BUF)) > 0) {
		write(STDOUT_FILENO, m_buf, len);
	}

	return 0;
}

int find(const char *key, int flag, int index)
{
	int total;
	char buf[MAX_KEY_LEN];
	sort_idx_t idx[MAX_FIND_RECORD];

	m_wiki_index->wi_find(key, flag, index, idx, &total);

	printf("total=%d\n", total);

	for (int i = 0; i < total; i++) {
		m_wiki_index->wi_get_key(&idx[i], buf);
		printf("%s.\n", buf);
		fflush(stdout);
		//idx[i].data_file_idx = 2;
		find_data(&idx[i]);
	}

	return 0;
}

int debug_find()
{
	struct file_st *p = m_file;

	printf("data_total=%d, image_total=%d\n", p->data_total, p->image_total);

	for (int i = 0; i < p->data_total; i++) {
		printf("dir[%d] = %s\n", i, p->data_file[i]);
	}
	printf("index file=%s\n", p->index_file);
	printf("math  file=%s\n", p->math_file);

	for (int i = 0; i < p->image_total; i++) {
		printf("image[%d] = %s\n", i, p->image_file[i]);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char default_lang[64];
	struct file_st *f;
	int total;

	m_wiki_config = new WikiConfig();
	m_wiki_config->wc_init(".");

	m_wiki_config->wc_get_lang_list(default_lang, &f, &total);
	m_file = &f[0];


	m_buf = (char *)malloc(MAX_DATA_BUF);

	if (m_file->data_total <= 0 || m_file->index_file[0] == 0) {
		debug_find();
		return 0;
	}

	if (find_init() == -1)
		return 1;

	find(argv[1], atoi(argv[2]), atoi(argv[3]));

	return 0;
}
