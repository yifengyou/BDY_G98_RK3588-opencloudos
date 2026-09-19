#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Regression: hook_info_read() must propagate serializer errors
# (-EFBIG from a too-small reader buffer, -EFAULT from a faulting
# reader buffer) instead of returning the accumulated length, which
# is 0 when the first event fails to serialize.

KSFT_SKIP=4
TESTS=2
test_no=0
failures=0
module_loaded=0

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
helper="$script_dir/aegis_readerr_test"
. "$script_dir/tkernel.sh"

SWITCH_SOCK=/proc/sys/kernel/sock_info_switch
SWITCH_EXEC=/proc/sys/kernel/execve_info_switch

skip_all()
{
	echo "1..0 # SKIP $1"
	exit "$KSFT_SKIP"
}

cleanup()
{
	[ -w "$SWITCH_SOCK" ] && printf 0 > "$SWITCH_SOCK" 2>/dev/null
	[ -w "$SWITCH_EXEC" ] && printf 0 > "$SWITCH_EXEC" 2>/dev/null
	# drain both streams so repeated runs start empty
	[ -r /proc/aegis/sock_info ] && cat /proc/aegis/sock_info >/dev/null 2>&1
	[ -r /proc/aegis/execve_info ] && cat /proc/aegis/execve_info >/dev/null 2>&1
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r aegis 2>/dev/null || true
	fi
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || skip_all "root privileges are required"
[ -x "$helper" ] || skip_all "aegis_readerr_test helper is missing"
if [ ! -d /proc/aegis ] && command -v modprobe >/dev/null 2>&1; then
	if modprobe aegis 2>/dev/null; then
		module_loaded=1
	fi
fi
[ -d /proc/aegis ] || skip_all "CONFIG_TKERNEL_AEGIS_MODULE is not enabled"
[ -w "$SWITCH_SOCK" ] && [ -w "$SWITCH_EXEC" ] ||
	skip_all "aegis capture switches are not writable"

trap cleanup EXIT INT TERM
ksft_plan "$TESTS"

# magic-tagged "on" value: (0x5a5a5a5a << 32) | 1
printf '%d\n' 6510615553911029761 > "$SWITCH_SOCK" ||
	skip_all "sock capture cannot be enabled"

"$helper" sockefbig
ksft_result $? "a too-small sock_info buffer reports an error, not 0"

printf '%d\n' 6510615553911029761 > "$SWITCH_EXEC" ||
	skip_all "exec capture cannot be enabled"

"$helper" execfault
ksft_result $? "a faulting execve_info buffer reports an error, not 0"

ksft_finished
