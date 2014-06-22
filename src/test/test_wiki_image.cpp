#include "wiki_config.h"
#include "wiki_image.h"

int main(int argc, char *argv[])
{
	struct file_st files;

	WikiConfig *config = new WikiConfig();
	config->wc_init(".");
	//config->wc_get_file(&files);

	WikiImage *image = new WikiImage();
	image->we_init(files.image_file, files.image_total);

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

