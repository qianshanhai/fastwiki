#include "wiki_config.h"
#include "wiki_image.h"

int main(int argc, char *argv[])
{
	int total;
	char default_lang[64];
	struct file_st *f, *files;

	if (argc < 2) {
		printf("usage: %s <fname>\n", argv[0]);
		return 0;
	}

	WikiConfig *config = new WikiConfig();
	config->wc_init(".");

	config->wc_get_lang_list(default_lang, &files, &total);
	f = &files[0];

	WikiImage *image = new WikiImage();
	image->we_init(f->image_file, f->image_total);

	char *buf = (char *)malloc(1024*1024);
	int size, len;

	if (image->we_reset(0, argv[1], &size) == 0) {
		while (image->we_read_next(0, buf, &len)) {
			write(STDOUT_FILENO, buf, len);
		}
	} else {
		printf("not found\n");
	}

	return 0;
}
