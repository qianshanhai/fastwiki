#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include "q_util.h"
#include "wiki_math.h"

WikiMath *m_math;
SHash *m_hash;

int m_count = 0;

int get_file_content(const char *file, char *buf)
{
	int fd;

	if ((fd = open(file, O_RDONLY)) == -1) {
		printf("file = %s: %s\n", file, strerror(errno));
		return -1;
	}

	int size = (int)file_size(file);

	int n = read(fd, buf, size);
	buf[n] = 0;

	close(fd);

	return n;
}

int onefile(const char *file)
{
	int n;
	char buf[8192], tmp[8192];
	struct math_key key;
	struct math_value value, *f;
	
	n = get_file_content(file, buf);
	math_format(buf, &n, tmp);

	m_math->wm_math2crc(buf, n, &key, 0);

	if (m_hash->sh_find(&key, (void **)&f) != _SHM_HASH_FOUND) {
		printf("count=%d\n", m_count++);
		m_hash->sh_add(&key, &value);
	}

	return 0;
}

int onedir(const char *dir)
{
	DIR *dirp;
	struct dirent *d;
	char file[128];

	if ((dirp = opendir(dir)) == NULL)
		return 0;

	while ((d = readdir(dirp))) {
		if (d->d_name[0] == '.')
			continue;
		sprintf(file, "%s/%s", dir, d->d_name);
		onefile(file);
	}

	closedir(dirp);

	return 0;
}


int main(int argc, char *argv[])
{
	m_math = new WikiMath();
	m_hash = new SHash();

	m_hash->sh_init(10*10000, sizeof(struct math_key), sizeof(struct math_value));

	if (dashd(argv[1])) {
		onedir(argv[1]);
	} else {
		m_math->wm_init2();
		m_math->wm_output(argv[1], atoi(argv[2]));
	}

	return 0;
}
