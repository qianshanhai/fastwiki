#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "q_util.h"
#include "image_tmp.h"

#define MAX_SIZE (256*1024)

int main(int argc, char *argv[])
{
	int n;
	char fname[1024], *image;

	ImageTmp *image_tmp = new ImageTmp();

	image_tmp->it_init(argv[1]);

	image_tmp->it_redo_hash();

	delete image_tmp;

	return 0;

	image = (char *)malloc(10*1024*1024);

	int total = 0;
	char buf[128];

	long long size = 0;
	int big_total = 0;

	while (image_tmp->it_read_next(fname, image, &n)) {
		if (n > MAX_SIZE) {
			size += MAX_SIZE / 2;
			big_total++;
		} else {
			size += n;
		}
	}

	printf("all size = %lld\n", size);
	printf("big_total = %d\n", big_total);

	return 0;
}
