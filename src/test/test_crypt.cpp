#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "wiki_crypt.h"

int main(int argc, char *argv[])
{
	WikiCrypt *crypt = new WikiCrypt();
	
	crypt->wt_init();

	char buf[128];

	if (argc > 1) {
		if (crypt->wt_check_key(argv[1])) {
			printf("ok\n");
		} else {
			printf("not ok\n");
		}
		return 0;
	}

	for (int i = 0; i < 1; i++) {
		crypt->wt_create_key(buf, sizeof(buf));
		printf("%s\n", buf);
	}

	if (crypt->wt_check_key(buf))
		printf("ok\n");

	return 0;
}
