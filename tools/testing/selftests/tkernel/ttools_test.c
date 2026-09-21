// SPDX-License-Identifier: GPL-2.0
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define KSFT_SKIP 4

#define TTOOLS_IO 0xEE

struct ttools_fd_ref {
	int fd;
	long ref_cnt;
};

#define TTOOLS_PTRACE_PROTECT		_IO(TTOOLS_IO, 0x00)
#define TTOOLS_PTRACE_UNPROTECT		_IO(TTOOLS_IO, 0x01)
#define TTOOLS_GET_FD_REFS_CNT		_IOWR(TTOOLS_IO, 0x02, struct ttools_fd_ref)

static int dev_fd = -1;

static int open_device(void)
{
	dev_fd = open("/dev/ttools", O_RDWR | O_CLOEXEC);
	return dev_fd < 0 ? -1 : 0;
}

/*
 * ptrace attach a child and report whether the attach was allowed.
 * Returns 0 when the child was traced (and detached again), 1 when
 * the attach was denied, 2 on harness errors.
 */
static int try_attach(pid_t pid)
{
	int status;

	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == 0) {
		if (waitpid(pid, &status, 0) != pid)
			return 2;
		if (ptrace(PTRACE_DETACH, pid, NULL, NULL))
			return 2;
		return 0;
	}
	if (errno == EPERM)
		return 1;
	return 2;
}

/*
 * Fork a child that optionally marks itself ptrace-protected (and
 * optionally unprotects itself again), then try to attach to it from
 * the parent.  The protection covers the task that issued the ioctl,
 * so the child must be the one to protect itself for the attach to be
 * denied.  A pipe handshake makes sure the parent only attaches after
 * the child performed its ioctls, otherwise the attach could race
 * ahead of the protection.
 * Returns 0 when the child was traced (and detached again), 1 when
 * the attach was denied, 2 on harness errors.
 */
static int run_attach_child(int protect, int unprotect)
{
	int syncfd[2];
	pid_t pid;
	int rc;
	char b;

	if (pipe(syncfd))
		return 2;

	pid = fork();
	if (pid < 0) {
		close(syncfd[0]);
		close(syncfd[1]);
		return 2;
	}
	if (pid == 0) {
		close(syncfd[0]);
		if (protect && ioctl(dev_fd, TTOOLS_PTRACE_PROTECT)) {
			perror("TTOOLS_PTRACE_PROTECT");
			_exit(2);
		}
		if (unprotect && ioctl(dev_fd, TTOOLS_PTRACE_UNPROTECT)) {
			perror("TTOOLS_PTRACE_UNPROTECT");
			_exit(2);
		}
		if (write(syncfd[1], "r", 1) != 1)
			_exit(2);
		pause();
		_exit(0);
	}
	close(syncfd[1]);

	if (read(syncfd[0], &b, 1) != 1) {
		close(syncfd[0]);
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		return 2;
	}
	close(syncfd[0]);

	rc = try_attach(pid);

	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);
	return rc;
}

int main(int argc, char **argv)
{
	struct ttools_fd_ref ref;
	const char *command;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <command>\n", argv[0]);
		return 2;
	}
	command = argv[1];

	if (open_device()) {
		if (errno == ENOENT)
			return KSFT_SKIP;
		perror("open /dev/ttools");
		return 2;
	}

	if (!strcmp(command, "attach_unprotected"))
		return run_attach_child(0, 0);

	if (!strcmp(command, "attach_protected"))
		return run_attach_child(1, 0);

	if (!strcmp(command, "attach_unprotected_again"))
		return run_attach_child(1, 1);

	if (!strcmp(command, "fd_refs")) {
		memset(&ref, 0, sizeof(ref));
		ref.fd = dev_fd;
		if (ioctl(dev_fd, TTOOLS_GET_FD_REFS_CNT, &ref)) {
			perror("TTOOLS_GET_FD_REFS_CNT");
			return 2;
		}
		if (ref.ref_cnt < 1) {
			fprintf(stderr, "unexpected ref count %ld\n", ref.ref_cnt);
			return 1;
		}
		return 0;
	}

	fprintf(stderr, "unknown command: %s\n", command);
	return 2;
}
