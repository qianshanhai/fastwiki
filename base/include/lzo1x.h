/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef __FASTWIKI_LZO1X_H
#define __FASTWIKI_LZO1X_H

class LZO1X {
	private:
		void *wrkmem;

	public:
		LZO1X();
		~LZO1X();

		int init();
		int compress(char *out, size_t out_size, const char *in, size_t in_size);
		int decompress(char *out, size_t out_size, const char *in, size_t in_size);

		int data_len(const char *p);
};


#endif
