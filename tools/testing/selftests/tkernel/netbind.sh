#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

KSFT_SKIP=4
PROC_FILE=/proc/tkernel/nonpriv_netbind
PORT=83
TESTS=9
test_no=0
failures=0

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
helper="$script_dir/netbind_test"

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

port_is_listed()
{
	grep -qx "$PORT" "$PROC_FILE"
}

remove_port()
{
	printf -- '-%s\n' "$PORT" > "$PROC_FILE" 2>/dev/null || true
}

run_helper()
{
	"$helper" "$PORT"
	return $?
}

invalid_write_is_rejected()
{
	before=$(cat "$PROC_FILE")
	if printf '%s\n' "$1" > "$PROC_FILE" 2>/dev/null; then
		return 1
	fi
	after=$(cat "$PROC_FILE")
	[ "$before" = "$after" ]
}

cleanup()
{
	[ -e "$PROC_FILE" ] && remove_port
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || skip_all "root privileges are required"
[ -e "$PROC_FILE" ] || skip_all \
	"CONFIG_TKERNEL_NONPRIV_NETBIND is not enabled"
[ -x "$helper" ] || skip_all "netbind_test helper is missing"

trap cleanup EXIT INT TERM
remove_port

run_helper
initial_bind_rc=$?
if [ "$initial_bind_rc" -eq "$KSFT_SKIP" ]; then
	skip_all "network namespaces are unavailable"
fi

echo "1..$TESTS"

if port_is_listed; then
	listed_rc=1
else
	listed_rc=0
fi
result "$listed_rc" "port is absent after reset"

invalid_write_is_rejected "+0"
result $? "port zero is rejected"

invalid_write_is_rejected "+1024"
result $? "a port outside the privileged range is rejected"

invalid_write_is_rejected "+${PORT}junk"
result $? "trailing garbage is rejected"

[ "$initial_bind_rc" -eq 1 ]
result $? "unprivileged bind is denied by default"

printf '+%s\n' "$PORT" > "$PROC_FILE"
result $? "port can be added to the allowlist"

port_is_listed
result $? "allowlisted port is reported through procfs"

run_helper
result $? "unprivileged bind succeeds for an allowlisted port"

remove_port
run_helper
rc=$?
[ "$rc" -eq 1 ]
result $? "unprivileged bind is denied after allowlist removal"

[ "$failures" -eq 0 ]
