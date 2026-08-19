#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

KSFT_SKIP=4
PROC_FILE=/proc/tkernel/nonpriv_netbind
PORT=83
OUT_OF_RANGE_PORT=1500
DEFAULT_PORT_START=1024
RAISED_PORT_START=2048
TESTS=16
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

result_or_skip()
{
	rc=$1
	expected=$2
	description=$3

	if [ "$rc" -eq "$KSFT_SKIP" ]; then
		test_no=$((test_no + 1))
		echo "ok $test_no - $description # SKIP protocol unavailable"
		return
	fi

	[ "$rc" -eq "$expected" ]
	result $? "$description"
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
	family=${1:-4}
	port=${2:-$PORT}
	port_start=${3:-$DEFAULT_PORT_START}
	protocol=${4:-tcp}
	"$helper" "$family" "$port" "$port_start" "$protocol"
	return $?
}

stress_state_access()
{
	(
		i=0
		while [ "$i" -lt 200 ]; do
			printf '+%s\n' "$PORT" > "$PROC_FILE" || exit 1
			printf -- '-%s\n' "$PORT" > "$PROC_FILE" || exit 1
			i=$((i + 1))
		done
	) &
	writer=$!

	workers=
	for worker in 1 2 3 4; do
		(
			i=0
			while [ "$i" -lt 25 ]; do
				run_helper 4 >/dev/null 2>&1
				rc=$?
				[ "$rc" -eq 0 ] || [ "$rc" -eq 1 ] || exit 1
				i=$((i + 1))
			done
		) &
		workers="$workers $!"
	done

	failed=0
	while kill -0 "$writer" 2>/dev/null; do
		cat "$PROC_FILE" >/dev/null || failed=1
	done
	wait "$writer" || failed=1
	for worker in $workers; do
		wait "$worker" || failed=1
	done

	remove_port
	return "$failed"
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

run_helper 6
result $? "IPv6 bind succeeds for an allowlisted port"

run_helper 4 "$PORT" "$DEFAULT_PORT_START" sctp
result_or_skip $? 1 "SCTP/IPv4 bind remains denied with a TCP allowlist"

run_helper 6 "$PORT" "$DEFAULT_PORT_START" sctp
result_or_skip $? 1 "SCTP/IPv6 bind remains denied with a TCP allowlist"

stress_state_access
result $? "concurrent procfs access and bind checks complete"

remove_port
run_helper
rc=$?
[ "$rc" -eq 1 ]
result $? "unprivileged bind is denied after allowlist removal"

run_helper 4 "$PORT" "$DEFAULT_PORT_START" sctp
result_or_skip $? 1 "SCTP bind is denied after allowlist removal"

run_helper 4 "$OUT_OF_RANGE_PORT" "$RAISED_PORT_START"
rc=$?
[ "$rc" -eq 1 ]
result $? "IPv4 port above the allowlist is denied with a raised threshold"

run_helper 6 "$OUT_OF_RANGE_PORT" "$RAISED_PORT_START"
rc=$?
[ "$rc" -eq 1 ]
result $? "IPv6 port above the allowlist is denied with a raised threshold"

[ "$failures" -eq 0 ]
