#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "soosue_log.h"
#include "wiki_audio.h"

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("usage: %s <audio file> <title> \n", argv[0]);
		return 0;
	}

	char *buf = (char *)malloc(5*1024*1024);

	WikiAudio *audio = new WikiAudio();
	if (audio->wa_init(argv[1]) == -1)
		return 1;

	int len, ret;

	if ((ret = audio->wa_find(argv[2], buf, &len, 5*1024*1024)) != 0) {
		LOG("ret: %d\n", ret);
		return 1;
	}

	write(STDOUT_FILENO, buf, len);

	return 0;
}
