// SPDX-License-Identifier: GPL-2.0
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define KSFT_SKIP 4

static int configure_unprivileged_port_start(const char *value)
{
	static const char path[] =
		"/proc/sys/net/ipv4/ip_unprivileged_port_start";
	size_t length = strlen(value);
	int fd;

	fd = open(path, O_WRONLY | O_CLOEXEC);
	if (fd < 0)
		return -1;

	if (write(fd, value, length) != (ssize_t)length) {
		close(fd);
		return -1;
	}

	return close(fd);
}

static int drop_privileges(void)
{
	if (setgroups(0, NULL))
		return -1;
	if (setresgid(65534, 65534, 65534))
		return -1;
	if (setresuid(65534, 65534, 65534))
		return -1;
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0))
		return -1;

	return 0;
}

int main(int argc, char **argv)
{
	struct sockaddr_storage addr = {};
	struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
	socklen_t addr_len;
	char *end;
	long port, port_start;
	int family, fd;

	if (argc != 4) {
		fprintf(stderr, "usage: %s FAMILY PORT UNPRIVILEGED_PORT_START\n",
			argv[0]);
		return 2;
	}

	errno = 0;
	port = strtol(argv[2], &end, 10);
	if (errno || *end || port < 1 || port > 65535) {
		fprintf(stderr, "invalid port: %s\n", argv[2]);
		return 2;
	}
	errno = 0;
	port_start = strtol(argv[3], &end, 10);
	if (errno || *end || port_start < 1 || port_start > 65535) {
		fprintf(stderr, "invalid unprivileged port start: %s\n", argv[3]);
		return 2;
	}

	if (!strcmp(argv[1], "4")) {
		family = AF_INET;
		addr4->sin_family = AF_INET;
		addr4->sin_addr.s_addr = htonl(INADDR_ANY);
		addr4->sin_port = htons(port);
		addr_len = sizeof(*addr4);
	} else if (!strcmp(argv[1], "6")) {
		family = AF_INET6;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_addr = in6addr_any;
		addr6->sin6_port = htons(port);
		addr_len = sizeof(*addr6);
	} else {
		fprintf(stderr, "invalid address family: %s\n", argv[1]);
		return 2;
	}

	if (unshare(CLONE_NEWNET)) {
		perror("unshare(CLONE_NEWNET)");
		return errno == EPERM ? KSFT_SKIP : 2;
	}

	if (configure_unprivileged_port_start(argv[3])) {
		perror("configure ip_unprivileged_port_start");
		return KSFT_SKIP;
	}

	if (drop_privileges()) {
		perror("drop privileges");
		return KSFT_SKIP;
	}

	fd = socket(family, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0) {
		perror("socket");
		return 2;
	}

	if (!bind(fd, (struct sockaddr *)&addr, addr_len)) {
		close(fd);
		return 0;
	}

	if (errno == EACCES || errno == EPERM) {
		close(fd);
		return 1;
	}

	perror("bind");
	close(fd);
	return 2;
}
