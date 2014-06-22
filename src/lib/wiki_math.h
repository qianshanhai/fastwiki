/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_MATH_H
#define __WIKI_MATH_H

#include <stdio.h>
#include <pthread.h>

#include "s_hash.h"

#include "q_util.h"

/*
 math file format:
 [ head ] [ SHash ] [ data ]

 */

typedef struct math_head {
	char version[32];
	int hash_size;
	int data_size;
} math_head_t;

struct math_key {
	unsigned int crc32;
	unsigned int r_crc32;
	int math_len;
};

struct math_value {
	int data_pos;
	int data_len;
};

class WikiMath {
	private:
		/* write */

		FILE *m_fp;
		int m_fd;

		SHash *m_hash;/* rw */

		char m_math_tmp_dir[128];
		char m_math_tmpfile[128];
		char m_math_tmp_png[128];

		int m_data_pos;
		int m_math_count;
		int m_error_count;

		pthread_mutex_t m_mutex;
		pthread_mutex_t m_file_mutex;
		int m_png_fd;

		int m_read_count;

		/* read */

		math_head_t m_head;
		int m_init_flag;

	public:
		WikiMath();
		~WikiMath();
	
	public:
		int wm_init(const char *outfile);
		int wm_find(const char *name, char *ret, int *len, int flag = 0);

		/*
		 * buf = M + len + crc32(math) + r_crc32(math) + png
		 *     M25.2984023.2983492.png
		 */

		int wm_init_parse(int flag, const char *file = NULL);
		int wm_init2();
		int wm_add(char *math, int math_len);
		int wm_output(const char *output, int max_thread);

		int wm_do_output(const char *output);

		int wm_zim_init(const char *lang, const char *date);
		int wm_zim_add_math(const char *fname, const char *data, int size);
		int wm_zim_output();

#ifdef __linux__
		int wm_start_one_thread(int idx);
#endif

	private:
#ifdef __linux__
		int wm_system(char *tmp_dir, char *math_dir,
				const char *math, char *out_fname);
		int wm_do_one_math(int idx, char *math, int math_len, char *buf);

		int wm_math2png(int idx, const char *math, char *ret);
		int wm_math2str(char *math, int math_len, char *buf);
#endif
		int wm_get_one_txt(char *math);
};

int math_format(char *buf, int *size, char *tmp);
int wm_math2crc(char *math, int math_len, struct math_key *k, int flag);
int wm_math2fname(char *math, int math_len, char *buf);

#endif
