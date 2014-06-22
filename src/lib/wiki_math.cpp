/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

#include "crc32sum.h"
#include "prime.h"
#include "q_util.h"
#include "wiki_math.h"

#include <sys/types.h>

#if defined(__linux__) || defined(__MACH__)
#include <sys/wait.h>
#endif

#include "wiki_common.h"

int math_format(char *buf, int *_size, char *tmp)
{
	int i, pos = 0, size = *_size;

	for (i = 0; i < size; i++) {
		if (buf[i] == '&') {
			if (strncmp(buf + i + 1, "amp;", 4) == 0) {
				tmp[pos++] = '&';
				i += 4;
				continue;
			} else if (strncmp(buf + i + 1, "gt;", 3) == 0) {
				tmp[pos++] = '>';
				i += 3;
				continue;
			} else if (strncmp(buf + i + 1, "lt;", 3) == 0) {
				tmp[pos++] = '<';
				i += 3;
				continue;
			}
		}
		tmp[pos++] = buf[i];
	}

	memcpy(buf, tmp, pos);
	buf[pos] = 0;
	*_size = pos;

	return pos;
}

WikiMath::WikiMath()
{
	m_fp = NULL;
	m_init_flag = 0;
}

WikiMath::~WikiMath()
{
	if (m_init_flag) {
		delete m_hash;
		close(m_fd);
	}
}

int WikiMath::wm_init(const char *input)
{
	if ((m_fd = open(input, O_RDONLY | O_BINARY)) == -1)
		return -1;

	read(m_fd, &m_head, sizeof(m_head));

	m_hash = new SHash();
	if (m_hash->sh_fd_init_ro(m_fd, sizeof(m_head)) == -1)
		return -1;

#ifndef WIN32
	pthread_mutex_init(&m_mutex, NULL);
#endif

	m_init_flag = 1;
	
	return 0;
}

	extern "C" int LOG(const char *fmt, ...);


int WikiMath::wm_find(const char *name, char *ret, int *len, int flag)
{
	char tmp[256];
	split_t sp;
	struct math_key key;
	struct math_value v;
	int n;

	memset(&key, 0, sizeof(key));

	if (m_init_flag == 0)
		return 0;

	if (name[0] == 'M')
		name++;

	strncpy(tmp, name, sizeof(tmp));
	if (split('.', tmp, sp) < 3)
		return 0;

	key.math_len = atoi(sp[0]);
	key.crc32 = (unsigned int)atoi(sp[1]);
	key.r_crc32 = (unsigned int)atoi(sp[2]);

#ifndef WIN32
	if (flag == 0)
		pthread_mutex_lock(&m_mutex);
#endif

	n = m_hash->sh_fd_find(&key, &v);

#ifndef WIN32
	if (flag == 0)
		pthread_mutex_unlock(&m_mutex);
#endif

	if (n == _SHASH_FOUND) {
		off_t offset = sizeof(m_head) + m_head.hash_size + v.data_pos;
		if (pread(m_fd, ret, v.data_len, offset) == v.data_len) {
			ret[v.data_len] = 0;
			*len = v.data_len;
		} else
			*len = 0;
	}

	return n == _SHASH_FOUND ? 1 : 0;
}

int WikiMath::wm_init2()
{
#ifdef __linux__
	strcpy(m_math_tmp_dir, "/dev/shm/wiki");
	if (!dashd(m_math_tmp_dir) && q_mkdir(m_math_tmp_dir, 0755) == -1) {
		strcpy(m_math_tmp_dir, "tmp");
		q_mkdir(m_math_tmp_dir, 0755);
	}
#else
	strcpy(m_math_tmp_dir, "tmp");
	q_mkdir(m_math_tmp_dir, 0755);
#endif
	
	sprintf(m_math_tmpfile, "%s/wiki_math.txt", m_math_tmp_dir);
	sprintf(m_math_tmp_png, "%s/wiki_png.dat", m_math_tmp_dir);

	m_data_pos = 0;
	m_math_count = 0;
	m_error_count = 0;
	m_read_count = 0;

#ifndef WIN32
	pthread_mutex_init(&m_file_mutex, NULL);
	pthread_mutex_init(&m_mutex, NULL);
#endif

	return 0;
}

int WikiMath::wm_init_parse(int flag, const char *file)
{
	wm_init2();

	if (file)
		strcpy(m_math_tmpfile, file);

	if (flag == 0 || flag == 2) {
		if ((m_fp = fopen(m_math_tmpfile, "w+")) == NULL) {
			printf("error: %s: %s\n", m_math_tmpfile, strerror(errno));
			return -1;
		}
	}

	return 0;
}

int WikiMath::wm_add(char *math, int math_len)
{
	if (math_len <= 0)
		return 0;

	fwrite(&math_len, sizeof(int), 1, m_fp);
	fwrite(math, math_len, 1, m_fp);
	fflush(m_fp);

	m_math_count++;

	return 1;
}	

int WikiMath::wm_get_one_txt(char *math)
{
	int len;
	
	if (read(m_fd, &len, sizeof(int)) < 4)
		return 0;
	
	if (read(m_fd, math, len) != len)
		return 0;

	m_read_count++;

	return len;
}

#ifndef WIN32

struct wiki_math_thread {
	void *type;
	int idx;
};

static void *wiki_thread(void *arg)
{
	struct wiki_math_thread *p = (struct wiki_math_thread *)arg;

	WikiMath *math = (WikiMath *)p->type;

	math->wm_start_one_thread(p->idx);

	return NULL;
}

int WikiMath::wm_start_one_thread(int idx)
{
	int len;
	char *math = (char *)malloc(2*1024*1024);
	char *buf = (char *)malloc(2*1024*1024);

#ifdef DEBUG
	printf("idx = %d\n", idx);
#endif

	for (;;) {
		if (pthread_mutex_lock(&m_file_mutex) != 0) {
			printf("%d: %s\n", errno, strerror(errno));;
			break;
		}
		len = wm_get_one_txt(math);
		pthread_mutex_unlock(&m_file_mutex);

		if (len == 0)
			break;
		wm_do_one_math(idx, math, len, buf);
	}

	free(math);
	free(buf);

	return 0;
}

int WikiMath::wm_output(const char *output, int max_thread)
{
	if (m_fp)
		fclose(m_fp);

	if ((m_fd = open(m_math_tmpfile, O_RDONLY | O_BINARY)) == -1) {
		printf("%s: %d: %s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	m_hash = new SHash();
	m_hash->sh_set_hash_magic(get_best_hash(m_math_count));
	m_hash->sh_init(100000, sizeof(struct math_key), sizeof(struct math_value));

	if ((m_png_fd = open(m_math_tmp_png, O_RDWR | O_APPEND | O_TRUNC | O_CREAT | O_BINARY, 0644)) == -1) {
		printf("error: %s : %s\n", m_math_tmp_png, strerror(errno));
		return -1;
	}

	struct wiki_math_thread arg;
	pthread_t id[128];

	for (int i = 0; i < max_thread; i++) {
		arg.type = (void *)this;
		arg.idx = i;
		int n = pthread_create(&id[i], NULL, wiki_thread, &arg);
		if (n != 0){
			return -1;
		}
		usleep(10000);
	}

	for (int i = 0; i < max_thread; i++)
		pthread_join(id[i], NULL);

	close(m_png_fd);

	wm_do_output(output);

	return 0;
}

int WikiMath::wm_do_one_math(int idx, char *math, int math_len, char *buf)
{
	int size;
	struct math_key key;
	struct math_value value, *f;

	math[math_len] = 0;

	wm_math2crc(math, math_len, &key, 0);

	if (pthread_mutex_lock(&m_mutex) != 0) {;
		printf("%d: %d: %s\n", __LINE__, errno, strerror(errno));
		return -1;
	}

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		pthread_mutex_unlock(&m_mutex);
		return 0;
	}

	pthread_mutex_unlock(&m_mutex);

	if ((size = wm_math2png(idx, math, buf)) == -1) {
		pthread_mutex_lock(&m_file_mutex);
		m_error_count++;
		pthread_mutex_unlock(&m_file_mutex);
		return 0;
	}

	if (pthread_mutex_lock(&m_mutex) != 0) {
		printf("%d: %d: %s\n", __LINE__, errno, strerror(errno));
	}

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		pthread_mutex_unlock(&m_mutex);
		return 0;
	}

	value.data_pos = m_data_pos;
	value.data_len = size;
	m_data_pos += size;
	m_hash->sh_add(&key, &value);

	write(m_png_fd, buf, size);

	pthread_mutex_unlock(&m_mutex);

	return 0;
}

void sig_chld(int signo)
{
	pid_t   pid;
	int     stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
}

int WikiMath::wm_system(char *tmp_dir, char *math_dir,
		const char *math, char *out_fname)
{
	int status;
	pid_t pid;
	int fd[2];
	char buf[128*1024];

	if (pipe(fd) == -1)
		return 0;

	//q_signal(SIGCHLD, sig_chld);
	
	char *bin = get_texvc_file();
	char tmp1[] = "iso-8859-1";
	char tmp2[] = "rgb 1.0 1.0 1.0";

	if ((pid = fork()) == 0) {
		char *arg[128] = {
			bin,
			tmp_dir, math_dir, (char *)math,
			tmp1, tmp2,
		};

		close(0);
		close(1);
		close(2);
		close(fd[0]);

		dup2(fd[1], 0);
		dup2(fd[1], 1);
		dup2(fd[1], 2);

		if (fd[1] > 2)
			close(fd[1]);

		execv(arg[0], arg);
		exit(0);
	}
	close(fd[1]);

	memset(buf, 0, sizeof(buf));
	read(fd[0], buf, sizeof(buf));
	close(fd[0]);
	
	//printf("buf=%s.\n", buf);

	//waitpid(pid, &status, WNOHANG | WUNTRACED);
	wait(&status);

	if (strchr("+cmlCMLX", buf[0])) {
		strncpy(out_fname, buf + 1, 32);
		out_fname[32] = 0;
		return 1;
	}

	return 0;
}

int fname_count = 0;

int WikiMath::wm_math2png(int idx, const char *math, char *ret)
{
	char tmp_dir[512], math_dir[512];
	char error_fname[256], png[256], out_fname[128];

	sprintf(tmp_dir, "%s/%d/tmp", m_math_tmp_dir, idx);
	sprintf(math_dir, "%s/%d/math", m_math_tmp_dir, idx);

	if (!dashd(tmp_dir)) {
		q_mkdir(tmp_dir, 0755);
	}

	if (!dashd(math_dir)) {
		q_mkdir(math_dir, 0755);
	}

	if (wm_system(tmp_dir, math_dir, math, out_fname) == 0)
		return -1;

	sprintf(png, "%s/%s.png", math_dir, out_fname);

	if (!dashf(png)) {
		pthread_mutex_lock(&m_file_mutex);
		fname_count++;
		pthread_mutex_unlock(&m_file_mutex);
		/*
		sprintf(error_fname, "/dev/shm/wiki/err/%d.tex", fname_count);
		rename(tex, error_fname);
		*/
		return -1;
	}

	int n, fd, size;
	
	size = (int)file_size(png);
	if ((fd = open(png, O_RDONLY | O_BINARY)) == -1)
		return -1;

	n = read(fd, ret, size);
	close(fd);

	unlink(png);

	if (n != size)
		return -1;

	ret[n] = 0;

	return size;
}

#endif

int WikiMath::wm_do_output(const char *output)
{
	int out_fd, len, hash_size;
	char *hash_addr, buf[4096];
	math_head_t head;

	if (m_math_count > 0) {
		printf("all count=%d, read_count=%d, error=%d\n", m_math_count, m_read_count, m_error_count);
		printf("PUB: %s count=%d, error=%d\n", output, m_read_count, m_error_count);
	}

	if ((m_fd = open(m_math_tmp_png, O_RDONLY | O_BINARY)) == -1)
		return -1;

	if ((out_fd = open(output, O_RDWR | O_APPEND | O_TRUNC | O_BINARY | O_CREAT, 0644)) == -1)
		return -1;

	m_hash->sh_get_addr((void **)&hash_addr, &hash_size);
	head.hash_size = hash_size;
	head.data_size = (int)file_size(m_fd);

	write(out_fd, &head, sizeof(head));
	write(out_fd, hash_addr, hash_size);

	while ((len = read(m_fd, buf, sizeof(buf))) > 0) {
		write(out_fd, buf, len);
	}

	close(out_fd);
	close(m_fd);

#ifndef DEBUG
	unlink(m_math_tmp_png);
#endif

	return 0;
}


int WikiMath::wm_zim_init(const char *lang, const char *date)
{
	m_data_pos = 0;

	sprintf(m_math_tmpfile, "fastwiki.math.%s.%s", lang, date);
	strcpy(m_math_tmp_png, "wiki_png.dat");

	m_hash = new SHash();
	m_hash->sh_set_hash_magic(get_best_hash(500000));
	m_hash->sh_init(100000, sizeof(struct math_key), sizeof(struct math_value));

	m_png_fd = open(m_math_tmp_png, O_RDWR | O_CREAT | O_TRUNC | O_APPEND | O_BINARY , 0644);

	return 0;
}

int WikiMath::wm_zim_add_math(const char *fname, const char *data, int size)
{
	struct math_key key;
	struct math_value value, *f;

	memset(&key, 0, sizeof(key));

	wm_math2crc((char *)fname, strlen(fname), &key, 1);

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND)
		return 0;

	value.data_pos = m_data_pos;
	value.data_len = size;
	m_data_pos += size;

	m_hash->sh_add(&key, &value);

	write(m_png_fd, data, size);

	return 0;
}

int WikiMath::wm_zim_output()
{
	close(m_png_fd);

	wm_do_output(m_math_tmpfile);

	return 0;
}

int wm_math2crc(char *math, int math_len, struct math_key *k, int flag)
{
	int tmp_size = 0;
	char buf[2048], *tmp = buf;
	
	if (math_len > 128) 
		tmp = (char *)malloc(2*1024*1024);

	for (int i = 0; i < math_len; i++) {
		if (math[i] == ' ' || math[i] == '\t' || math[i] == '\r' || math[i] == '\n')
			continue;
		tmp[tmp_size++] = math[i];
		if (flag) {
			if (math[i] == '"')
				math[i] = '\'';
		}
	}

	crc32sum(tmp, tmp_size, &k->crc32, &k->r_crc32);
	k->math_len = tmp_size;

	if (math_len > 128)
		free(tmp);

	return 0;
}

int wm_math2fname(char *math, int math_len, char *buf)
{
	struct math_key key;

	wm_math2crc(math, math_len, &key, 1);

	return sprintf(buf, "M%d.%u.%u.png", key.math_len, key.crc32, key.r_crc32);
}
