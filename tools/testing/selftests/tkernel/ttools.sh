#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

DEV=/dev/ttools
TESTS=4
module_loaded=0

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
helper="$script_dir/ttools_test"
. "$script_dir/tkernel.sh"

skip_unavailable()
{
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r ttools 2>/dev/null || true
	fi
	ksft_skip_all "$1"
}

cleanup()
{
	if [ -n "$was_protected" ]; then
		"$helper" attach_unprotected_again >/dev/null 2>&1 || true
	fi
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r ttools 2>/dev/null || true
	fi
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || skip_unavailable "root privileges are required"
[ -x "$helper" ] || skip_unavailable "ttools_test helper is missing"

if [ ! -e "$DEV" ] && command -v modprobe >/dev/null 2>&1; then
	if modprobe ttools 2>/dev/null; then
		module_loaded=1
	fi
fi
[ -c "$DEV" ] || skip_unavailable "CONFIG_TKERNEL_TTOOLS is not enabled"

trap cleanup EXIT INT TERM
ksft_plan "$TESTS"

"$helper" attach_unprotected
rc=$?
[ "$rc" -ne 2 ]
ksft_result $? "an unprotected process can be ptraced"

"$helper" attach_protected
rc=$?
[ "$rc" -eq 1 ]
assert_rc=$?
was_protected=1
ksft_result "$assert_rc" "a protected process rejects PTRACE_ATTACH"

"$helper" attach_unprotected_again
rc=$?
was_protected=""
[ "$rc" -eq 0 ]
ksft_result $? "protection can be removed again"

"$helper" fd_refs
rc=$?
[ "$rc" -eq 0 ]
ksft_result $? "fd reference counts can be queried"

ksft_finished
