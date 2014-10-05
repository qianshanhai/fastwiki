#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "q_util.h"
#include "gzip_compress.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("usage: %s <size> <pos> > tmpfile.1 \n", argv[0]);
		return 0;
	}

	int size = atoi(argv[1]);
	int pos = atoi(argv[2]);

	unsigned char *p = (unsigned char *)malloc(size);
	unsigned char *t = (unsigned char *)malloc(size);

	memset(p, 0, size);

	//p[pos] |= (1 << 5);

	int n;

	n = gzip((char *)t, size, (char *)p, size);

	write(STDOUT_FILENO, t, n);

	return 0;
}
