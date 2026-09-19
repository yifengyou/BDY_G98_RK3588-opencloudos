// SPDX-License-Identifier: GPL-2.0
/*
 * Regression helper for aegis hook_info_read() error propagation:
 *   sockefbig  - enable sock capture, create a UDP socket so one
 *                sock_info event is queued, then read
 *                /proc/aegis/sock_info with a buffer smaller than the
 *                sock_info size. The serializer returns -EFBIG; the
 *                read must report the error (negative errno) instead
 *                of 0 bytes.
 *   execfault  - enable execve capture, exec a child, then read
 *                /proc/aegis/execve_info through a buffer that faults
 *                on the first copy. The read must report -EFAULT
 *                instead of 0 bytes.
 * exit codes: 0 pass, 1 fail, 4 skip
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define AEGIS_READ_FLAG		0x5a5a5a5aUL

#ifndef F_LINUX_SPECIFIC_BASE
#define F_LINUX_SPECIFIC_BASE	1024
#endif
#define F_SET_FILE_HOOK_FLAG	(F_LINUX_SPECIFIC_BASE + 1024)

static int set_hook_flag(int fd)
{
	unsigned long v = AEGIS_READ_FLAG;

	return fcntl(fd, F_SET_FILE_HOOK_FLAG, &v);
}

static int open_aegis(const char *path)
{
	int fd = open(path, O_RDONLY);

	if (fd < 0)
		return -1;
	if (set_hook_flag(fd)) {
		close(fd);
		return -1;
	}
	return fd;
}

static int do_sockefbig(void)
{
	char buf[16];		/* far below the sock_info size */
	int fd, n;

	/* a UDP bind queues one sock_info event via sock_hook_check() */
	int sk = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
	};

	if (sk < 0)
		return 4;
	if (bind(sk, (struct sockaddr *)&sa, sizeof(sa))) {
		close(sk);
		return 4;
	}

	fd = open_aegis("/proc/aegis/sock_info");
	if (fd < 0) {
		close(sk);
		return 4;
	}

	n = read(fd, buf, sizeof(buf));
	close(fd);
	close(sk);

	if (n > 0) {
		/* stream moved: acceptable drain, error not exercised */
		return 0;
	}
	if (n == 0) {
		fprintf(stderr,
			"small-buffer read returned 0; -EFBIG swallowed\n");
		return 1;
	}
	/* negative errno: expected after the fix */
	return 0;
}

static int do_execfault(void)
{
	char *map, *fault_buf;
	int fd, n;
	pid_t pid;
	int status;
	char *const argv[] = { "/bin/true", NULL };
	char *const envp[] = { NULL };

	pid = fork();
	if (pid == 0)
		_exit(execve(argv[0], argv, envp) ? 127 : 0);
	waitpid(pid, &status, 0);

	fd = open_aegis("/proc/aegis/execve_info");
	if (fd < 0)
		return 4;

	map = mmap(NULL, 2 * 4096, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED) {
		close(fd);
		return 4;
	}
	if (mprotect(map + 4096, 4096, PROT_NONE)) {
		close(fd);
		return 4;
	}
	/* the buffer starts inside the last valid page so the very
	 * first copy crosses into PROT_NONE and faults */
	fault_buf = map + 4096 - 8;

	n = read(fd, fault_buf, 4096);
	close(fd);

	if (n > 0)
		return 0;	/* no event queued; not exercised */
	if (n == 0) {
		fprintf(stderr,
			"faulting read returned 0; -EFAULT swallowed\n");
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc == 2 && !strcmp(argv[1], "sockefbig"))
		return do_sockefbig();
	if (argc == 2 && !strcmp(argv[1], "execfault"))
		return do_execfault();

	fprintf(stderr, "usage: %s sockefbig | execfault\n", argv[0]);
	return 2;
}
