#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

DEV=/dev/ttools
TESTS=4
helper32=

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
driver="$script_dir/ttools_ptrace"
helper32="$script_dir/ttools_ptrace_32"
. "$script_dir/tkernel.sh"

skip_unavailable()
{
	ksft_skip_all "$1"
}

if [ "$(id -u)" -ne 0 ]; then
	skip_unavailable "root privileges are required"
fi
[ -x "$driver" ] || skip_unavailable "ttools_ptrace driver is missing"
[ -x "$helper32" ] || skip_unavailable \
	"32-bit tracer helper is missing; rebuild with a working CC32 toolchain"

if [ ! -e "$DEV" ] && command -v modprobe >/dev/null 2>&1; then
	modprobe ttools 2>/dev/null
fi
[ -c "$DEV" ] || skip_unavailable "CONFIG_TKERNEL_TTOOLS is not enabled"

echo "TAP version 13"
TTOOLS_PTRACE_32="$helper32" "$driver"
rc=$?
[ "$rc" -ne 2 ] || ksft_skip_all "the ttools_ptrace driver hit a harness error"
exit "$rc"
