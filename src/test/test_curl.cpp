#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#include "wiki_curl.h"

int main(int argc, char *argv[])
{
	WikiCurl *curl = new WikiCurl();

	curl->curl_init();
	curl->curl_set_proxy("http://127.0.0.1:58213");

	char *data;
	int len;

	curl->curl_get_data(0, argv[1], &data, &len);

	write(STDOUT_FILENO, data, len);

	delete curl;

	return 0;
}
