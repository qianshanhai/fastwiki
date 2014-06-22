#include "wiki_stat.h"

int main(int argc, char *argv[])
{
	int n;
	WikiStat *wiki_stat;

	if (argc < 2) {
		printf("usage: %s <total>\n", argv[0]);
		return 0;
	} 

	wiki_stat = new WikiStat();

	if (wiki_stat->st_init(0) == -1)
		return -1;

	wiki_stat->st_print(atoi(argv[1]));

	delete wiki_stat;

	return 0;
}
