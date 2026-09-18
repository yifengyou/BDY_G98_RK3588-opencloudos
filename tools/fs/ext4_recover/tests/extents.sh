#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
${CC:-cc} ${CPPFLAGS:-} ${CFLAGS:-} "$test_dir/extents.c" ${LDFLAGS:-} \
	-lcom_err -lext2fs -o "$build_dir/extents"
"$build_dir/extents"
