// SPDX-License-Identifier: GPL-2.0

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	long page_size;
	char *buffer;
	size_t length;
	ssize_t ret;
	int fd;

	if (argc != 2) {
		fprintf(stderr, "usage: %s PROC_FILE\n", argv[0]);
		return 2;
	}

	page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0)
		return 2;
	length = 2 * page_size - 1;

	buffer = malloc(length);
	if (!buffer)
		return 2;
	memset(buffer, 'x', length);
	buffer[length - 1] = '\n';

	fd = open(argv[1], O_WRONLY | O_CLOEXEC);
	if (fd < 0) {
		perror("open shield_mounts");
		free(buffer);
		return 2;
	}

	errno = 0;
	ret = write(fd, buffer, length);
	if (ret >= 0) {
		fprintf(stderr, "oversized command was unexpectedly accepted\n");
		ret = 1;
	} else if (errno == EFAULT || errno == EINVAL) {
		ret = 0;
	} else {
		perror("write shield_mounts");
		ret = 2;
	}

	close(fd);
	free(buffer);
	return ret;
}
