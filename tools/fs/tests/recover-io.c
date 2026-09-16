// SPDX-License-Identifier: GPL-2.0
/* Exercise the production copy functions using ordinary temporary files. */
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static ssize_t test_read(int fd, void *buf, size_t count);
static ssize_t test_write(int fd, const void *buf, size_t count);

#define read test_read
#define write test_write
#define main recovery_main
/* features.h already enabled these before including the legacy source. */
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE
#ifndef RECOVERY_SOURCE
#ifdef TEST_EXT4
#define RECOVERY_SOURCE "../ext4_recover/src/ext4recover.c"
#else
#define RECOVERY_SOURCE "../xfs_recover/recover/init.c"
#endif
#endif
#include RECOVERY_SOURCE
#undef main
#undef read
#undef write

enum fault { NONE, INTERRUPT_READ, INTERRUPT_WRITE, END_READ, ERROR_READ,
	     ERROR_WRITE, ZERO_WRITE };
static enum fault fault;
static size_t after, read_bytes, written_bytes, limit;
static int injected, calls;
#ifndef TEST_EXT4
static struct xfs_sb test_super;
#endif

static int inject(int writing)
{
	size_t done = writing ? written_bytes : read_bytes;
	int applicable = writing ? (fault == INTERRUPT_WRITE ||
		fault == ERROR_WRITE || fault == ZERO_WRITE) :
		(fault == INTERRUPT_READ || fault == END_READ || fault == ERROR_READ);

	assert(++calls < 10000);
	if (!applicable || done < after || injected)
		return 0;
	injected = 1;
	errno = (fault == INTERRUPT_READ || fault == INTERRUPT_WRITE) ? EINTR :
		(writing ? ENOSPC : EIO);
	return 1;
}

static size_t request_size(size_t count, size_t done)
{
	if (limit && count > limit)
		count = limit;
	if (done < after && count > after - done)
		count = after - done;
	return count;
}

static ssize_t test_read(int fd, void *buf, size_t count)
{
	ssize_t ret;

	if (inject(0))
		return fault == END_READ ? 0 : -1;
	ret = read(fd, buf, request_size(count, read_bytes));
	if (ret > 0)
		read_bytes += ret;
	return ret;
}

static ssize_t test_write(int fd, const void *buf, size_t count)
{
	ssize_t ret;

	if (inject(1))
		return fault == ZERO_WRITE ? 0 : -1;
	ret = write(fd, buf, request_size(count, written_bytes));
	if (ret > 0)
		written_bytes += ret;
	return ret;
}

static void run_case(size_t bs, enum fault mode, size_t position, size_t chunk)
{
	FILE *src = tmpfile(), *dst = tmpfile();
	unsigned char *input = malloc(6 * bs), *output = calloc(6, bs);
	struct stat st;
	size_t i;
	int ret, success = mode <= INTERRUPT_WRITE;

	assert(src && dst && input && output);
	for (i = 0; i < 6 * bs; i++)
		input[i] = (i * 31 + i / bs) % 251 + 1;
	assert(write(fileno(src), input, 6 * bs) == (ssize_t)(6 * bs));
#ifdef TEST_EXT4
	blocksize = bs;
#else
	test_super.sb_blocksize = bs;
	sbp = &test_super;
#endif
	fault = mode;
	after = position;
	limit = chunk;
	injected = calls = 0;
	read_bytes = written_bytes = 0;
	alarm(5);
	ret = recover_block_to_file(fileno(src), fileno(dst), 3, 3, 2);
	alarm(0);
	assert(ret == success);
	assert(fstat(fileno(dst), &st) == 0);
	if (success) {
		assert(st.st_size == (off_t)(6 * bs));
		assert(pread(fileno(dst), output, 6 * bs, 0) == (ssize_t)(6 * bs));
		for (i = 0; i < 3 * bs; i++)
			assert(output[i] == 0);
		assert(!memcmp(output + 3 * bs, input + 2 * bs, 3 * bs));
	} else {
		assert(injected);
		assert(st.st_size < (off_t)(6 * bs));
	}
	fclose(src);
	fclose(dst);
	free(input);
	free(output);
}

int main(void)
{
	size_t bs, positions[3], i;
	enum fault mode;
	int cases = 0;

	for (bs = 1024; bs <= 4096; bs *= 2) {
		run_case(bs, NONE, 0, 0);
		run_case(bs, NONE, 0, 17);
		cases += 2;
		positions[0] = 0;
		positions[1] = 23;
		positions[2] = bs + 17;
		for (mode = INTERRUPT_READ; mode <= ZERO_WRITE; mode++) {
			for (i = 0; i < 3; i++) {
				run_case(bs, mode, positions[i], 17);
				cases++;
			}
		}
	}
#ifndef TEST_EXT4
	mflag = 1;
	assert(recover_block_to_file(-1, -1, 0, 1, 0) == 1);
	cases++;
#endif
	printf("ok - %d recovery copy cases\n", cases);
	return 0;
}
