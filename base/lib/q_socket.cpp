/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock.h>
#include <winsock2.h>
typedef int socklen_t;
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "q_socket.h"

int q_bind(int port, bool local_flag)
{
	struct sockaddr_in addr;
	int sock, on = 1;

#ifdef WIN32
	WSADATA wsaData;
	int listenFd;

	if (WSAStartup(MAKEWORD(1,1), &wsaData) == SOCKET_ERROR) {
		printf ("Error initialising WSA.\n");
		return -1;
	}
#endif

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		return -1;

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	memset(&addr, 0, sizeof(addr));
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = local_flag ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		return -1;

	if (listen(sock, 5) == -1)
		return -1;

	return sock;
}

int q_accept(int sock, char *remote, int *port)
{
	int fd, value;
	struct sockaddr_in client;

	value  = sizeof(struct sockaddr_in);
	if ((fd = accept(sock, (struct sockaddr *)&client, (socklen_t *)&value)) == -1)
		return -1;

	if (remote)
		strcpy(remote, inet_ntoa(client.sin_addr));

	if (port)
		*port = (int)client.sin_port;

	return fd;
}

int q_read_data(int fd, char *buf, int len, unsigned int timeout)
{
	fd_set rfds;
	struct timeval tv;
	int n;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	tv.tv_sec = timeout / 1000000;
	tv.tv_usec = timeout % 1000000;

	n = select(fd + 1, &rfds, NULL, NULL, &tv);

	if (n <= 0)
		return n;

	if (FD_ISSET(fd, &rfds)) {
		return (n = recv(fd, buf, len, 0)) == 0 ? -2 : n;
	}

	return -1;
}

int q_read_data_len(int fd, char *buf, int len, unsigned int timeout, int *rlen)
{
	int n = 0, pos = 0;

	for (;;) {
		if (pos == len)
			break;
		if ((n = q_read_data(fd, buf + pos, len - pos, timeout)) <= 0) {
			if (rlen)
				*rlen = pos;
			return n;
		}
		pos += n;
	}

	if (rlen)
		*rlen = pos;

	return pos;
}

int q_connect(const char *ip, int port, int timeout)
{
	int fd;
	struct timeval tv;
	struct sockaddr_in addr;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		return -1;
	}

	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
#ifndef WIN32
	inet_pton(AF_INET, ip, &addr.sin_addr);
#endif

#ifndef WIN32
	tv.tv_sec  = timeout / 1000000;
	tv.tv_usec = timeout % 1000000;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
		close(fd);
		return -1;
	}
#endif

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		close(fd);
		return -1;
	}

	return fd;
}
