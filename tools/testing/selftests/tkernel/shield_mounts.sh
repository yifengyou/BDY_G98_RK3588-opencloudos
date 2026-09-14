#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

KSFT_SKIP=4
PROC_FILE=/proc/tkernel/shield_mounts
DEV_NAME=/dev/codex-shield
MOUNT_PATH=
TESTS=14
test_no=0
failures=0
mounted=0

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

result_skip()
{
	test_no=$((test_no + 1))
	echo "ok $test_no - $1 # SKIP $2"
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

mount_is_listed()
{
	grep -F "$MOUNT_PATH" "$1" | grep -Fq "$DEV_NAME"
}

container_hides_mount()
{
	SHIELD_DEV_NAME="$DEV_NAME" SHIELD_MOUNT_PATH="$MOUNT_PATH" \
		unshare --mount --pid --fork --mount-proc sh -c '
			if grep -F "$SHIELD_MOUNT_PATH" /proc/self/mounts |
				grep -Fq "$SHIELD_DEV_NAME"; then
				exit 1
			fi
			if grep -F "$SHIELD_MOUNT_PATH" /proc/self/mountinfo |
				grep -Fq "$SHIELD_DEV_NAME"; then
				exit 1
			fi
		'
}

container_clear_restores_mount()
{
	SHIELD_DEV_NAME="$DEV_NAME" SHIELD_MOUNT_PATH="$MOUNT_PATH" \
		unshare --mount --pid --fork --mount-proc sh -c '
			printf "clear %s %s\n" "$SHIELD_DEV_NAME" \
				"$SHIELD_MOUNT_PATH" > /proc/tkernel/shield_mounts ||
				exit 1
			if grep -Fqx "$SHIELD_DEV_NAME on $SHIELD_MOUNT_PATH" \
				/proc/tkernel/shield_mounts; then
				exit 1
			fi
			grep -F "$SHIELD_MOUNT_PATH" /proc/self/mounts |
				grep -Fq "$SHIELD_DEV_NAME" || exit 1
			grep -F "$SHIELD_MOUNT_PATH" /proc/self/mountinfo |
				grep -Fq "$SHIELD_DEV_NAME"
		'
}

cleanup()
{
	[ -e "$PROC_FILE" ] && clear_entry
	[ "$mounted" -eq 1 ] && umount "$MOUNT_PATH" 2>/dev/null
	[ -n "$MOUNT_PATH" ] && rmdir "$MOUNT_PATH" 2>/dev/null
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || skip_all "root privileges are required"
[ -e "$PROC_FILE" ] || skip_all \
	"CONFIG_TKERNEL_SHIELD_MOUNTS is not enabled"
[ -x "$helper" ] || skip_all "shield_mounts_test helper is missing"
mkdir -p "${TMPDIR:-/tmp}" || skip_all "temporary directory is unavailable"
MOUNT_PATH=$(mktemp -d "${TMPDIR:-/tmp}/codex-shield.XXXXXX") ||
	skip_all "temporary mount point cannot be created"

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

mount -t tmpfs "$DEV_NAME" "$MOUNT_PATH"
mount_rc=$?
if [ "$mount_rc" -eq 0 ]; then
	mounted=1
fi
result "$mount_rc" "a dedicated tmpfs mount can be created"

printf 'set %s %s\n' "$DEV_NAME" "$MOUNT_PATH" > "$PROC_FILE"

mount_is_listed /proc/self/mounts
result $? "the host PID namespace retains mounts visibility"

mount_is_listed /proc/self/mountinfo
result $? "the host PID namespace retains mountinfo visibility"

if command -v unshare >/dev/null 2>&1 &&
	unshare --mount --pid --fork --mount-proc true 2>/dev/null; then
	container_hides_mount
	result $? "a child mount and PID namespace hides the mount"

	container_clear_restores_mount
	result $? "clear restores both container procfs mount views"

	if entry_is_listed; then
		listed_rc=1
	else
		listed_rc=0
	fi
	result "$listed_rc" "the host observes the container clear"
else
	result_skip "container mount visibility" \
		"mount and PID namespaces are unavailable"
	result_skip "container clear restores procfs views" \
		"mount and PID namespaces are unavailable"
	result_skip "host observes the container clear" \
		"mount and PID namespaces are unavailable"
fi

[ "$failures" -eq 0 ]
