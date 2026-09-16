// SPDX-License-Identifier: GPL-2.0
/* Exercise the native and compat layouts using the same userspace source. */
#define _GNU_SOURCE
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "../kselftest.h"
#include "../../../../kernel/tkernel/ttools/ttools.h"

/* Existing 32-bit applications can explicitly use the 64-bit layout. */
struct ttools_fd_ref64 {
	int32_t fd;
	int32_t padding;
	int64_t ref_cnt;
};

#define TTOOLS_GET_FD_REFS_CNT64 _IOWR(TTOOLS_IO, 0x02, struct ttools_fd_ref64)
#define TTOOLS_UNKNOWN _IO(TTOOLS_IO, 0x7f)

static int device;

static int expect_error(unsigned int cmd, void *arg, int error)
{
	int ret;

	errno = 0;
	ret = ioctl(device, cmd, arg);
	return ret == -1 && errno == error;
}

static int refs_equal(int fd, long count)
{
	struct ttools_fd_ref ref = { .fd = fd, .ref_cnt = LONG_MIN };

	return ioctl(device, TTOOLS_GET_FD_REFS_CNT, &ref) == 0 &&
		ref.fd == fd && ref.ref_cnt == count;
}

static void test_permissions(void)
{
	pid_t child;
	int status = 0;

	fflush(stdout);
	child = fork();
	if (!child) {
		if (setuid(65534))
			_exit(1);
		/* Permission checks must precede copying the NULL argument. */
		_exit(!(expect_error(TTOOLS_GET_FD_REFS_CNT, NULL, EPERM) &&
			expect_error(TTOOLS_PTRACE_PROTECT, NULL, EPERM) &&
			expect_error(TTOOLS_PTRACE_UNPROTECT, NULL, EPERM) &&
			expect_error(TTOOLS_UNKNOWN, NULL, EPERM)));
	}
	ksft_test_result(child > 0 && waitpid(child, &status, 0) == child &&
			 WIFEXITED(status) && WEXITSTATUS(status) == 0,
			 "CAP_SYS_ADMIN is required before dispatch or user access\n");
}

int main(void)
{
	struct ttools_fd_ref invalid = { .fd = -1, .ref_cnt = LONG_MIN };
	struct ttools_fd_ref64 wide = { .ref_cnt = INT64_MIN };
	struct ttools_fd_ref *edge;
	struct utsname uts;
	long page_size;
	char *mapping;
	int fd, duplicate, ret;

	ksft_print_header();
	device = open("/dev/ttools", O_RDONLY);
	if (device < 0)
		ksft_exit_skip("/dev/ttools unavailable; load the ttools module\n");
	if (geteuid() || expect_error(TTOOLS_UNKNOWN, NULL, EPERM))
		ksft_exit_skip("root with CAP_SYS_ADMIN is required\n");

	page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0)
		ksft_exit_fail_msg("invalid page size\n");
	mapping = mmap(NULL, 2 * page_size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED ||
	    mprotect(mapping + page_size, page_size, PROT_NONE))
		ksft_exit_fail_msg("guard mapping: %s\n", strerror(errno));
	fd = eventfd(0, EFD_CLOEXEC);
	if (fd < 0)
		ksft_exit_fail_msg("eventfd: %s\n", strerror(errno));

	ksft_set_plan(12);
	ksft_print_msg("long=%zu struct=%zu command=%#lx\n", sizeof(long),
		       sizeof(struct ttools_fd_ref),
		       (unsigned long)TTOOLS_GET_FD_REFS_CNT);
	ksft_test_result(refs_equal(fd, 1), "one eventfd reference\n");
	duplicate = dup(fd);
	ksft_test_result(duplicate >= 0 && refs_equal(fd, 2),
			 "duplicating the descriptor increases its count\n");
	if (duplicate >= 0)
		close(duplicate);
	ksft_test_result(refs_equal(fd, 1), "closing the duplicate restores its count\n");
	ksft_test_result(expect_error(TTOOLS_GET_FD_REFS_CNT, &invalid, EBADF) &&
			 invalid.fd == -1 && invalid.ref_cnt == LONG_MIN,
			 "invalid fd returns EBADF without writing the argument\n");
	ksft_test_result(expect_error(TTOOLS_GET_FD_REFS_CNT, NULL, EFAULT),
			 "inaccessible input returns EFAULT\n");

	edge = (void *)(mapping + page_size - sizeof(*edge));
	edge->fd = fd;
	edge->ref_cnt = LONG_MIN;
	ksft_test_result(ioctl(device, TTOOLS_GET_FD_REFS_CNT, edge) == 0 &&
			 edge->fd == fd && edge->ref_cnt == 1,
			 "a structure ending at a guard page is copied at its ABI size\n");
	if (mprotect(mapping, page_size, PROT_READ))
		ksft_exit_fail_msg("read-only mapping: %s\n", strerror(errno));
	ksft_test_result(expect_error(TTOOLS_GET_FD_REFS_CNT, edge, EFAULT),
			 "readable but unwritable output returns EFAULT\n");
	ksft_test_result(expect_error(TTOOLS_UNKNOWN, NULL, EINVAL),
			 "unknown commands retain EINVAL\n");

	/* The native-size command already worked through the old compat hook. */
	if (!uname(&uts) && (sizeof(long) == 8 || strstr(uts.machine, "64"))) {
		wide.fd = fd;
		ksft_test_result(ioctl(device, TTOOLS_GET_FD_REFS_CNT64, &wide) == 0 &&
				 wide.fd == fd && wide.ref_cnt == 1,
				 "the explicit 64-bit layout remains supported\n");
	} else {
		ksft_test_result_skip("64-bit layout requires a 64-bit kernel\n");
	}

	/* These commands ignore arg; exercise compat dispatch with nonzero arg. */
	ret = ioctl(device, TTOOLS_PTRACE_PROTECT, (void *)UINTPTR_MAX);
	ksft_test_result(ret == 0 &&
			 expect_error(TTOOLS_PTRACE_PROTECT, NULL, EEXIST),
			 "argument-free protection commands still work\n");
	ret = ioctl(device, TTOOLS_PTRACE_UNPROTECT, (void *)UINTPTR_MAX);
	ksft_test_result(ret == 0, "argument-free unprotection still works\n");
	test_permissions();

	close(fd);
	close(device);
	munmap(mapping, 2 * page_size);
	ksft_finished();
}
