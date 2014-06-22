#include <unistd.h> 
#include <string.h>
#include <stdio.h>

#define TOLOWER(c) (((c) >= 'a' && (c) <= 'z') ? ((c) + 'A' - 'a') : (c))
#define TOHEX(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0'): (TOLOWER(c) - 'A' + 10))


int ch2hex(const char *s, unsigned char *buf)
{
	char ch;
	int pos = 0;

	for (; *s; s++) {
		if (*s == '%') {
			if (s[1] != 0 && s[2] != 0) {
				printf("%d, %d\n", TOHEX(s[1]), TOHEX(s[2]));
				buf[pos++] = TOHEX(s[1]) * 16 + TOHEX(s[2]);
				s += 2;
				continue;
			}
		}
		buf[pos++] = *s;
	}

	buf[pos] = 0;

	return pos;
}

int main(int argc, char *argv[])
{
	char tmp[1024];

	ch2hex(argv[1], (unsigned char *)tmp);

	write(1, tmp, strlen(tmp));
	write(1, "\n", 1);

	return 0;
}
