#include <stdio.h>

#include "wiki_parse.h"

int main(int argc, char *argv[])
{
	WikiParse *parse = new WikiParse();

	char *buf;
	int size;

	char input[1024];

	while (1) {
		fgets(input, sizeof(input), stdin);
		if (parse->wp_math(input, &buf, &size) == 0) {
		} else {
			printf("error\n");
		}
	}

	return 0;
}
