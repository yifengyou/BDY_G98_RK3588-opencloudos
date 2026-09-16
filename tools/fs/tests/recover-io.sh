#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
xfs_dir=${XFS_PROGS_DIR:-"$test_dir/../xfs_recover/xfsprogs-5.9.0"}
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

# CPPFLAGS can point at a private libext2fs development-header installation.
# Compile the full production sources; discard unused CLI sections at link time.
${CC:-cc} ${CPPFLAGS:-} ${CFLAGS:-} -O0 -ffunction-sections -fdata-sections \
	-DTEST_EXT4 "$test_dir/recover-io.c" -Wl,--gc-sections \
	-o "$build_dir/ext4-io"
"$build_dir/ext4-io"

# First run make in tools/fs/xfs_recover to configure its pinned xfsprogs tree.
${CC:-cc} ${CPPFLAGS:-} ${CFLAGS:-} -O0 -ffunction-sections -fdata-sections \
	-I"$xfs_dir/include" -I"$xfs_dir/libxfs" -I"$xfs_dir" -I"$xfs_dir/db" \
	"$test_dir/recover-io.c" -Wl,--gc-sections -o "$build_dir/xfs-io"
"$build_dir/xfs-io"
