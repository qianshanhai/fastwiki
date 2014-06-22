#include "wiki_common.h"

#include "q_util.h"
#include "zim.h"

int main(int argc, char *argv[])
{
	int size;
	char *data;
	struct zim_tmp_title st;
	char title[1024], redirect[1024];

	long long image_size = 0;

	Zim *zim = new Zim();

	zim->zim_init(argv[1]);

	zim->zim_title_reset();
	while (zim->zim_title_read(&st, title, redirect) == 1) {
		if (zim_is_redirect(&st)) {
			printf("%s -> %s, %c\n", title, redirect, st.name_space);
		} else {
			if (st.name_space == 'I') {
				format_image_name(title, sizeof(title));
				printf("I.%s\n", title);
			} else {
				printf("%s, %c\n", title, st.name_space);
			}
		}
	}

	return 0;

	zim->zim_data_reset();
	while (zim->zim_data_read(&st, title, redirect, &data, &size, 0) == 1) {
		if (st.name_space == 'I') {
			image_size += size;
		}
	}

	printf("image_size: %lld\n", image_size);

	delete zim;

	return 0;
}
