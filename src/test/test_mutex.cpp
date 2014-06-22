#include <pthread.h>

pthread_mutex_t m_key_mutex;

int main(int argc, char *argv[])
{
	int j = 0;
	pthread_mutex_init(&m_key_mutex, NULL);

	for (int i = 0; i < 100000000; i++) {
		pthread_mutex_lock(&m_key_mutex);
		j++;
		pthread_mutex_unlock(&m_key_mutex);
	}

	return j;
}

