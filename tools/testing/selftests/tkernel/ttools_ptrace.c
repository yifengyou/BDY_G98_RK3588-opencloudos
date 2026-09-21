// SPDX-License-Identifier: GPL-2.0
/*
 * ttools ptrace protection must cover compat (32-bit) ptrace callers.
 *
 * The file is compiled twice from the same source: as the 64-bit
 * driver and as a 32-bit tracer helper (ttools_ptrace_32).  The
 * driver forks a tracee which protects its own thread group through
 * /dev/ttools and pauses; ttools denies ptrace on a tracee whose
 * group leader is protected.  The driver then runs the trace matrix
 * against that tracee:
 *
 *   native PTRACE_ATTACH  (in-process, 64-bit)
 *   native PTRACE_SEIZE   (in-process, 64-bit)
 *   compat PTRACE_ATTACH  (ttools_ptrace_32 helper)
 *   compat PTRACE_SEIZE   (ttools_ptrace_32 helper)
 *
 * Every request must fail with EPERM.  Before the compat fix the
 * 32-bit tracer attached successfully to the protected tracee,
 * because compat_sys_ptrace() never invoked ptrace_pre_hook().
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef TTOOLS_PTRACE_TRACER

/* 32-bit tracer helper: ttools_ptrace_32 <pid> <ATTACH|SEIZE>
 * Exits 0 when the request was allowed, 1 on EPERM, 2 otherwise.
 */
int main(int argc, char **argv)
{
	unsigned long request;
	pid_t pid;

	if (argc != 3)
		return 2;
	request = strcmp(argv[2], "SEIZE") == 0 ? PTRACE_SEIZE : PTRACE_ATTACH;
	pid = (pid_t)atol(argv[1]);
	if (ptrace(request, pid, NULL, NULL) == 0) {
		/* Undo a successful attach without killing the tracee:
		 * the driver reuses it for the remaining checks.
		 */
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		kill(pid, SIGCONT);
		return 0;
	}
	return errno == EPERM ? 1 : 2;
}

#else

#include "../kselftest.h"

#define TTOOLS_PTRACE_PROTECT	_IO(0xEE, 0x00)

static pid_t tracee = -1;

/* The tracee protects its own thread group, pauses, and reports
 * readiness by closing the pipe write end only after PROTECT
 * succeeded.  Returns 0 on a protected tracee.
 */
static int spawn_protected_tracee(int ready_fd)
{
	tracee = fork();
	if (tracee < 0)
		return -1;
	if (tracee == 0) {
		int dev, ret;
		char zero = 0;

		dev = open("/dev/ttools", O_RDWR | O_CLOEXEC);
		if (dev < 0)
			_exit(2);
		ret = ioctl(dev, TTOOLS_PTRACE_PROTECT);
		close(dev);
		if (ret)
			_exit(2);
		(void)write(ready_fd, &zero, sizeof(zero));
		pause();
		_exit(0);
	}
	return 0;
}

static int native_trace(unsigned long request)
{
	if (ptrace(request, tracee, NULL, NULL) == 0) {
		/* Undo a successful attach so the remaining checks run
		 * against the same protected tracee: detach (or KILL the
		 * stop for SEIZE) and resume it.
		 */
		ptrace(PTRACE_DETACH, tracee, NULL, NULL);
		kill(tracee, SIGCONT);
		return 0;
	}
	return errno == EPERM ? 1 : 2;
}

static int helper_trace(const char *helper, const char *request)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid < 0)
		return 2;
	if (pid == 0) {
		char pidstr[16];

		snprintf(pidstr, sizeof(pidstr), "%d", (int)tracee);
		execl(helper, helper, pidstr, request, (char *)NULL);
		_exit(127);
	}
	if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
		return 2;
	return WEXITSTATUS(status);
}

static void cleanup(void)
{
	if (tracee > 0) {
		kill(tracee, SIGKILL);
		waitpid(tracee, NULL, 0);
	}
}

int main(void)
{
	int ready[2];
	char ready_byte;
	const char *helper;
	int rc;

	ksft_print_header();
	ksft_set_plan(4);

	helper = getenv("TTOOLS_PTRACE_32");
	if (!helper || access(helper, X_OK))
		ksft_exit_skip("32-bit tracer helper is unavailable");

	if (pipe(ready))
		ksft_exit_fail_msg("pipe: %s", strerror(errno));

	if (atexit(cleanup))
		ksft_exit_fail_msg("atexit failed");

	switch (spawn_protected_tracee(ready[1])) {
	case 0:
		break;
	default:
		ksft_exit_fail_msg("cannot spawn the tracee");
	}

	if (read(ready[0], &ready_byte, sizeof(ready_byte)) != 1)
		ksft_exit_fail_msg("the tracee failed to protect itself");

	rc = native_trace(PTRACE_ATTACH);
	ksft_test_result(rc == 1, "native PTRACE_ATTACH on a protected tracee is denied\n");

	rc = native_trace(PTRACE_SEIZE);
	ksft_test_result(rc == 1, "native PTRACE_SEIZE on a protected tracee is denied\n");

	rc = helper_trace(helper, "ATTACH");
	ksft_test_result(rc == 1, "compat PTRACE_ATTACH on a protected tracee is denied\n");

	rc = helper_trace(helper, "SEIZE");
	ksft_test_result(rc == 1, "compat PTRACE_SEIZE on a protected tracee is denied\n");

	return ksft_exit_pass();
}

#endif
