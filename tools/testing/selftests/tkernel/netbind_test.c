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

static int configure_unprivileged_port_start(void)
{
	static const char path[] =
		"/proc/sys/net/ipv4/ip_unprivileged_port_start";
	static const char value[] = "1024\n";
	int fd;

	fd = open(path, O_WRONLY | O_CLOEXEC);
	if (fd < 0)
		return -1;

	if (write(fd, value, sizeof(value) - 1) != sizeof(value) - 1) {
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
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
	};
	char *end;
	long port;
	int fd;

	if (argc != 2) {
		fprintf(stderr, "usage: %s PORT\n", argv[0]);
		return 2;
	}

	errno = 0;
	port = strtol(argv[1], &end, 10);
	if (errno || *end || port < 1 || port > 65535) {
		fprintf(stderr, "invalid port: %s\n", argv[1]);
		return 2;
	}

	if (unshare(CLONE_NEWNET)) {
		perror("unshare(CLONE_NEWNET)");
		return errno == EPERM ? KSFT_SKIP : 2;
	}

	if (configure_unprivileged_port_start()) {
		perror("configure ip_unprivileged_port_start");
		return KSFT_SKIP;
	}

	if (drop_privileges()) {
		perror("drop privileges");
		return KSFT_SKIP;
	}

	fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0) {
		perror("socket");
		return 2;
	}

	addr.sin_port = htons(port);
	if (!bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
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
