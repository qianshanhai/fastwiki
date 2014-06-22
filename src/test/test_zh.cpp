#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "wiki_zh.h"
#include "q_util.h"

int main(int argc, char *argv[])
{
	mapfile_t mt;

	if (argc < 3) {
		printf("usage: %s <zh2hans.txt> <input>\n", argv[0]);
		return 0;
	}

	q_mmap(argv[2], &mt);

	WikiZh *zh = new WikiZh();
	zh->wz_init();

	char *buf = (char *)malloc(1024*1024);
	int n = zh->wz_convert_2hans((char *)mt.start, mt.size, buf);

	printf("%s", buf);

	return 0;
}
