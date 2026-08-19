#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

KSFT_SKIP=4
PROC_FILE=/proc/tkernel/shield_mounts
DEV_NAME=/dev/codex-shield
MOUNT_PATH=/mnt/codex-shield
TESTS=8
test_no=0
failures=0

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
helper="$script_dir/shield_mounts_test"

skip_all()
{
	echo "1..0 # SKIP $1"
	exit "$KSFT_SKIP"
}

result()
{
	test_no=$((test_no + 1))
	if [ "$1" -eq 0 ]; then
		echo "ok $test_no - $2"
	else
		echo "not ok $test_no - $2"
		failures=$((failures + 1))
	fi
}

entry_is_listed()
{
	grep -Fqx "$DEV_NAME on $MOUNT_PATH" "$PROC_FILE"
}

clear_entry()
{
	printf 'clear %s %s\n' "$DEV_NAME" "$MOUNT_PATH" \
		> "$PROC_FILE" 2>/dev/null || true
}

cleanup()
{
	[ -e "$PROC_FILE" ] && clear_entry
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || skip_all "root privileges are required"
[ -e "$PROC_FILE" ] || skip_all \
	"CONFIG_TKERNEL_SHIELD_MOUNTS is not enabled"
[ -x "$helper" ] || skip_all "shield_mounts_test helper is missing"

trap cleanup EXIT INT TERM
clear_entry
echo "1..$TESTS"

printf 'set %s %s\n' "$DEV_NAME" "$MOUNT_PATH" > "$PROC_FILE"
result $? "a newline-terminated entry is accepted"

entry_is_listed
result $? "the entry is reported through procfs"

printf 'clear %s %s\n' "$DEV_NAME" "$MOUNT_PATH" > "$PROC_FILE"
result $? "the entry can be cleared"

if entry_is_listed; then
	listed_rc=1
else
	listed_rc=0
fi
result "$listed_rc" "the cleared entry is absent"

printf 'set %s %s\t\n\t\n' "$DEV_NAME" "$MOUNT_PATH" > "$PROC_FILE"
result $? "consecutive trailing escape characters are accepted"

entry_is_listed
result $? "the escaped entry is parsed without extra bytes"

clear_entry
if entry_is_listed; then
	listed_rc=1
else
	listed_rc=0
fi
result "$listed_rc" "the escaped entry can be cleared"

"$helper" "$PROC_FILE"
result $? "a maximum-size trailing escape is rejected safely"

[ "$failures" -eq 0 ]
