// SPDX-License-Identifier: GPL-2.0
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE
#include <assert.h>
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE
#define main recovery_main
#ifndef RECOVERY_SOURCE
#define RECOVERY_SOURCE "../src/ext4recover.c"
#endif
#include RECOVERY_SOURCE
#undef main

static void set_extent(struct ext3_extent *extent, unsigned int logical,
		       unsigned int encoded, unsigned int physical)
{
	extent->ee_block = ext2fs_cpu_to_le32(logical);
	extent->ee_len = ext2fs_cpu_to_le16(encoded);
	extent->ee_start = ext2fs_cpu_to_le32(physical);
}

static struct ext3_extent_header *new_leaf(size_t bs, unsigned int entries)
{
	struct ext3_extent_header *header = calloc(1, bs);

	assert(header);
	header->eh_magic = ext2fs_cpu_to_le16(EXT3_EXT_MAGIC);
	header->eh_entries = ext2fs_cpu_to_le16(entries);
	header->eh_max = ext2fs_cpu_to_le16((bs - sizeof(*header)) /
					 sizeof(struct ext3_extent));
	return header;
}

static void length_case(size_t bs, unsigned int encoded)
{
	unsigned int length = encoded > 0x8000 ? encoded - 0x8000 : encoded;
	FILE *input = tmpfile(), *output = tmpfile();
	struct ext3_extent_header *header = new_leaf(bs, 1);
	unsigned char *data = malloc(bs), *actual = malloc(bs);
	struct stat st;
	unsigned int i;

	assert(input && output && data && actual);
	blocksize = bs;
	device_fd = fileno(input);
	recover_fd = fileno(output);
	assert(ftruncate(device_fd, (off_t)(length + 1) * bs) == 0);
	memset(data, 0x5a, bs);
	assert(pwrite(device_fd, data, bs, bs) == (ssize_t)bs);
	assert(pwrite(device_fd, data, bs, (off_t)length * bs) == (ssize_t)bs);
	set_extent(EXT_FIRST_EXTENT(header), 3, encoded, 1);
	/* An unwritten extent must never read physical data. */
	if (encoded > 0x8000)
		device_fd = -1;
	alarm(15);
	assert(dump_dir_extent(header) == 1);
	alarm(0);
	assert(fstat(recover_fd, &st) == 0);
	assert(st.st_size == (off_t)(length + 3) * bs);
	for (i = 0; i < length + 3; i++) {
		assert(pread(recover_fd, actual, bs, (off_t)i * bs) == (ssize_t)bs);
		memset(data, encoded <= 0x8000 && (i == 3 || i == length + 2)
		       ? 0x5a : 0, bs);
		assert(!memcmp(data, actual, bs));
	}
	fclose(input);
	fclose(output);
	free(data);
	free(actual);
	free(header);
}

static void mixed_case(size_t bs)
{
	FILE *input = tmpfile(), *output = tmpfile();
	struct ext3_extent_header *header = new_leaf(bs, 4);
	struct ext3_extent *extent = EXT_FIRST_EXTENT(header);
	unsigned char *data = malloc(bs), *actual = malloc(bs);
	struct stat st;
	unsigned int i;

	assert(input && output && data && actual);
	blocksize = bs;
	device_fd = fileno(input);
	recover_fd = fileno(output);
	memset(data, 0x39, bs);
	assert(write(device_fd, data, bs) == (ssize_t)bs);
	set_extent(extent, 0, 1, 0);
	set_extent(extent + 1, 1, 0x8002, 0);
	set_extent(extent + 2, 4, 1, 0);
	set_extent(extent + 3, 7, 0x8001, 0);
	assert(dump_dir_extent(header) == 1);
	assert(fstat(recover_fd, &st) == 0 && st.st_size == (off_t)(8 * bs));
	for (i = 0; i < 8; i++) {
		assert(pread(recover_fd, actual, bs, (off_t)i * bs) == (ssize_t)bs);
		memset(data, i == 0 || i == 4 ? 0x39 : 0, bs);
		assert(!memcmp(data, actual, bs));
	}
	/* Reject a zero-length extent and propagate unwritten output errors. */
	header->eh_entries = ext2fs_cpu_to_le16(1);
	set_extent(extent, 0, 0, 0);
	assert(dump_dir_extent(header) == 0);
	set_extent(extent, 0, 0x8001, 0);
	recover_fd = -1;
	assert(dump_dir_extent(header) == 0);
	fclose(input);
	fclose(output);
	free(data);
	free(actual);
	free(header);
}

int main(void)
{
	unsigned int encoded[] = {1, 0x7fff, 0x8000, 0x8001, 0xffff};
	size_t bs, i;

	for (bs = 1024; bs <= 4096; bs *= 2) {
		for (i = 0; i < sizeof(encoded) / sizeof(encoded[0]); i++)
			length_case(bs, encoded[i]);
		mixed_case(bs);
	}
	puts("ok - 24 extent length, content and failure cases");
	return 0;
}
