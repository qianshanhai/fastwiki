/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef __Q_SOCKET_H
#define __Q_SOCKET_H

#include <stdio.h>

int q_bind(int port, bool local_flag = false);
int q_accept(int sock, char *remote = NULL, int *port = NULL);
int q_read_data(int fd, char *buf, int len, unsigned int timeout = 30000000);
int q_read_data_len(int fd, char *buf, int len, 
			unsigned int timeout = 30000000, int *rlen = NULL);
int q_connect(const char *ip, int port, int timeout);

#endif
