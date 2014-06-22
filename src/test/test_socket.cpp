#include <stdio.h>
#include <stdlib.h>

#include "q_util.h"

#include "wiki_socket.h"

void my_wait_func()
{
	for (;;) {
		sleep(1);
	}
}

static int my_do_url(void *type, int sock, const char *url)
{
	int n;
	char buf[1024], file[128];
	mapfile_t mt;
	WikiSocket *ws = (WikiSocket *)type;

	sprintf(file, "/dev/shm/hace/%s", url);		

	if (q_mmap(file, &mt) == NULL) {
		n = sprintf(buf, "<html>file: %s : %s</html>", file, strerror(errno));
		ws->ws_http_output_head(sock, "text/html", n);
		ws->ws_http_output_body(sock, buf, n);
		return 0;
	}
	ws->ws_http_output_head(sock, "image/png", (int)mt.size);
	ws->ws_http_output_body(sock, (char *)mt.start, (int)mt.size);

	q_munmap(&mt);

	return 0;
}

int main(int argc, char *argv[])
{
	WikiSocket *ws = new WikiSocket();

	ws->ws_init(my_wait_func, my_do_url);
	ws->ws_start_http_server();

	delete ws;

	return 0;
}
