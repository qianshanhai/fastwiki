/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __IMEI_CRC32SUM_H
#define __IMEI_CRC32SUM_H

unsigned int crc32sum(const char *buf);
unsigned int crc32sum(const char *buf, int len);
unsigned int r_crc32sum(const char *buf, int len);
unsigned int crc32sum(const char *buf, int len, unsigned int *crc32, unsigned int *r_crc32);

#endif
