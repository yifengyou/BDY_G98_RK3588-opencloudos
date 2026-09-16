// SPDX-License-Identifier: GPL-2.0
/* LD_PRELOAD fault injection, active only while a RECOVER output is open. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int output_fd = -1, interrupted;
static size_t read_bytes, write_bytes;

#define WRAP_OPEN(name) \
int name(const char *path, int flags, ...) \
{ \
	int (*real_open)(const char *, int, ...) = dlsym(RTLD_NEXT, #name); \
	mode_t mode = 0; \
	va_list ap; \
	int fd; \
	if (flags & O_CREAT) { \
		va_start(ap, flags); \
		mode = va_arg(ap, int); \
		va_end(ap); \
	} \
	fd = real_open(path, flags, mode); \
	if (fd >= 0 && strstr(path, "RECOVER/")) \
		output_fd = fd; \
	return fd; \
}

WRAP_OPEN(open)
WRAP_OPEN(open64)

int close(int fd)
{
	int (*real_close)(int) = dlsym(RTLD_NEXT, "close");

	if (fd == output_fd)
		output_fd = -1;
	return real_close(fd);
}

static ssize_t transfer(int fd, void *buf, size_t count, int writing)
{
	ssize_t (*real_io)(int, void *, size_t) =
		dlsym(RTLD_NEXT, writing ? "write" : "read");
	const char *mode = getenv("RECOVER_FAULT");
	size_t *done = writing ? &write_bytes : &read_bytes;
	ssize_t ret;

	if (!mode || output_fd < 0 || (writing && fd != output_fd))
		return real_io(fd, buf, count);
	if (*done >= 23 && !strncmp(mode, writing ? "write-" : "read-",
				  writing ? 6 : 5)) {
		if (strstr(mode, "eintr") && !interrupted) {
			interrupted = 1;
			errno = EINTR;
			return -1;
		}
		if (strstr(mode, "error")) {
			errno = writing ? ENOSPC : EIO;
			return -1;
		}
		if (strstr(mode, "zero"))
			return 0;
	}
	if (count > 17)
		count = 17;
	if (*done < 23 && count > 23 - *done)
		count = 23 - *done;
	ret = real_io(fd, buf, count);
	if (ret > 0)
		*done += ret;
	return ret;
}

ssize_t read(int fd, void *buf, size_t count)
{
	return transfer(fd, buf, count, 0);
}

ssize_t __read_chk(int fd, void *buf, size_t count, size_t buflen)
{
	if (count > buflen)
		abort();
	return transfer(fd, buf, count, 0);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return transfer(fd, (void *)buf, count, 1);
}
