#include "q_util.h"
#include "wiki_math.h"

#include "wiki_common.h"

int main(int argc, char *argv[])
{
	char ret[8192];
	int size;

	if (argc < 3) {
		printf("usage: %s <input> <name>\n", argv[0]);
		return 0;
	}

	WikiMath *math = new WikiMath();

	math->wm_init(argv[1]);
	if (math->wm_find(argv[2], ret, &size)) {
		file_append(argv[2], ret, size);
	}

	return 0;
}
