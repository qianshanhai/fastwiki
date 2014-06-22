#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "q_util.h"
#include "image_tmp.h"

int main(int argc, char *argv[])
{
	int len;
	char *buf = (char *)malloc(5*1024*1024);

	if (argc < 3) {
		printf("usage: %s <dir> <fname>\n", argv[0]);
		return 0;
	}

	ImageTmp *image_tmp = new ImageTmp();

	image_tmp->it_init(argv[1]);

	if (image_tmp->it_find(argv[2], buf, &len) == 0) {
		write(STDOUT_FILENO, buf, len);
	}

	return 0;
}
