#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

SYSCTL=/proc/sys/kernel/sig_kill_protect
RULES=/proc/kill_protect/blacklist
STAT=/proc/kill_protect/stat
TESTS=9
target_pid=
module_loaded=0

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
helper="$script_dir/signal_test"
. "$script_dir/tkernel.sh"

write_rule()
{
	printf '%s\n' "$1" > "$RULES" 2>/dev/null
}

write_sysctl()
{
	printf '%s\n' "$1" > "$SYSCTL" 2>/dev/null
}

skip_unavailable()
{
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r kill_protect 2>/dev/null || true
	fi
	ksft_skip_all "$1"
}

start_target()
{
	"$helper" wait "$target_comm" &
	target_pid=$!

	i=0
	while [ "$i" -lt 100 ]; do
		if [ -r "/proc/$target_pid/comm" ] &&
		   [ "$(cat "/proc/$target_pid/comm")" = "$target_comm" ]; then
			return 0
		fi
		kill -0 "$target_pid" 2>/dev/null || return 1
		i=$((i + 1))
		sleep 0.01
	done

	return 1
}

wait_for_exit()
{
	i=0
	while [ "$i" -lt 100 ]; do
		kill -0 "$target_pid" 2>/dev/null || {
			wait "$target_pid" 2>/dev/null || true
			return 0
		}
		i=$((i + 1))
		sleep 0.01
	done

	return 1
}

cleanup()
{
	write_sysctl 0 || true
	write_rule "del $target_comm" || true
	if [ -n "$target_pid" ] && kill -0 "$target_pid" 2>/dev/null; then
		kill -KILL "$target_pid" 2>/dev/null || true
		wait "$target_pid" 2>/dev/null || true
	fi
	write_sysctl "$old_sysctl" || true
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r kill_protect 2>/dev/null || true
	fi
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || ksft_skip_all "root privileges are required"
[ -x "$helper" ] || ksft_skip_all "signal_test helper is missing"
if [ ! -e "$RULES" ] && command -v modprobe >/dev/null 2>&1; then
	if modprobe kill_protect 2>/dev/null; then
		module_loaded=1
	fi
fi
[ -w "$SYSCTL" ] && [ -r "$RULES" ] && [ -w "$RULES" ] &&
	[ -r "$STAT" ] || skip_unavailable \
	"CONFIG_TKERNEL_KILL_PROTECT is not enabled"

old_sysctl=$(cat "$SYSCTL")
target_comm="kp_t_$$"
sender_comm="kp_s_$$"
trap cleanup EXIT
trap 'exit 1' INT TERM

write_sysctl 0 || ksft_skip_all "kill protect sysctl is not writable"
start_target || ksft_skip_all "could not start the signal target"
ksft_plan "$TESTS"

write_rule "add $target_comm"
ksft_result $? "a process name can be added to the blacklist"

grep -qx "$target_comm" "$RULES"
ksft_result $? "the blacklist reports the added process name"

if write_rule "add $target_comm"; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "a duplicate blacklist rule is rejected"

write_sysctl 1
ksft_result $? "kill protection can be enabled"

protect_before=$(awk '/^protect count:/ { print $3 }' "$STAT")
"$helper" send "$sender_comm" "$target_pid" 15 >/dev/null 2>&1
rc=$?
[ "$rc" -eq 1 ]
ksft_result $? "SIGTERM for a blacklisted process is rejected"

kill -0 "$target_pid" 2>/dev/null
ksft_result $? "the protected process remains alive"

protect_after=$(awk '/^protect count:/ { print $3 }' "$STAT")
[ -n "$protect_before" ] && [ -n "$protect_after" ] &&
	[ "$protect_after" -gt "$protect_before" ]
ksft_result $? "the protection counter is incremented"

write_rule "del $target_comm"
ksft_result $? "the blacklist rule can be removed"

"$helper" send "$sender_comm" "$target_pid" 15 >/dev/null 2>&1
rc=$?
if [ "$rc" -eq 0 ] && wait_for_exit; then
	target_pid=
	rc=0
else
	rc=1
fi
ksft_result "$rc" "SIGTERM is delivered after removing the rule"

ksft_finished
