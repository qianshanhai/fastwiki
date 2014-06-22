#include <stdlib.h>
#include <iconv.h>
#include "wiki_index.h"

iconv_t conv;

int init()
{
	if ((conv = iconv_open("UTF-8", "GB2312")) == (iconv_t) -1)
		perror("iconv_open");

	return 0;
}

char *TO(char *buf)
{
	char *p2 = buf;
	char *p, tmp[512];
	size_t out, len = strlen(buf);

	memset(tmp, 0, sizeof(tmp));

	p = tmp;
	iconv(conv, &p2, &len, &p, &out);

	memset(buf, 0, 512);

	strcpy(buf, tmp);

	return buf;
}

int main(int argc, char *argv[])
{
	int total = 0;
	sort_idx_t idx[MAX_FIND_RECORD];
	WikiIndex *wiki_idx = new WikiIndex();

	wiki_idx->wi_init(argv[1]);
	wiki_idx->wi_stat();
	wiki_idx->wi_find(argv[2], atoi(argv[3]), atoi(argv[4]), idx, &total);

	printf("%d\n", total);

	int test[10] = {1, 3, 17, 21, 30, 35, 46, 48, 60, 70};

	for (int i = 0; i < 10; i++) {
		printf("%d\n", wiki_idx->wi_binary_find(test, 10, test[i]));
	}

	return 0;
}
