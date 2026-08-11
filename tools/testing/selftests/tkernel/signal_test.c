// SPDX-License-Identifier: GPL-2.0
#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

#define TASK_COMM_LEN 16

static int set_comm(const char *comm)
{
	if (!comm[0] || strlen(comm) >= TASK_COMM_LEN) {
		fprintf(stderr, "invalid task name: %s\n", comm);
		return -1;
	}

	if (prctl(PR_SET_NAME, comm)) {
		perror("prctl(PR_SET_NAME)");
		return -1;
	}

	return 0;
}

static int parse_number(const char *value, long min, long max, long *result)
{
	char *end;
	long number;

	errno = 0;
	number = strtol(value, &end, 10);
	if (errno || *end || number < min || number > max)
		return -1;

	*result = number;
	return 0;
}

static int wait_for_signal(const char *comm)
{
	if (set_comm(comm))
		return 2;

	/* Ensure an interrupted test does not leave the target behind. */
	if (prctl(PR_SET_PDEATHSIG, SIGUSR1)) {
		perror("prctl(PR_SET_PDEATHSIG)");
		return 2;
	}
	if (getppid() == 1)
		return 2;

	for (;;)
		pause();
}

static int send_signal(const char *comm, const char *pid_string,
		       const char *signal_string)
{
	long pid, sig;

	if (set_comm(comm))
		return 2;
	if (parse_number(pid_string, 1, INT_MAX, &pid) ||
	    parse_number(signal_string, 1, NSIG - 1, &sig)) {
		fprintf(stderr, "invalid pid or signal\n");
		return 2;
	}

	if (!kill((pid_t)pid, (int)sig))
		return 0;
	if (errno == EPERM)
		return 1;

	perror("kill");
	return 2;
}

int main(int argc, char **argv)
{
	if (argc == 3 && !strcmp(argv[1], "wait"))
		return wait_for_signal(argv[2]);
	if (argc == 5 && !strcmp(argv[1], "send"))
		return send_signal(argv[2], argv[3], argv[4]);

	fprintf(stderr, "usage: %s wait COMM\n", argv[0]);
	fprintf(stderr, "       %s send COMM PID SIGNAL\n", argv[0]);
	return 2;
}
