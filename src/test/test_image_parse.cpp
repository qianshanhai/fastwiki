#include <stdio.h>

#include "image_parse.h"

int main(int argc, char *argv[])
{
	char url[1024];
	ImageParse *ip = new ImageParse();

	ip->ip_init();

	for (int i = 1; i < argc; i++) {
		ip->ip_parse(argv[1]);
		ip->ip_get_url(url);
		printf("%s\n", url);
	}

	return 0;
}
