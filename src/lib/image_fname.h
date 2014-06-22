/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __IMAGE_FNAME_H
#define __IMAGE_FNAME_H

#include "s_hash.h"

#define OUT_IMG_LIST_FNAME "filename.txt"

class ImageFname {
	private:
		SHash *m_hash;
		int m_fd;
		pthread_mutex_t m_mutex;
	public:
		ImageFname();
		~ImageFname();

		int if_init(const char *file = OUT_IMG_LIST_FNAME);
		int if_add_fname(const char *fname);
};

#endif
