#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

SYSCTL=/proc/sys/kernel/sig_kill_block
RULES=/proc/kill_block/whitelist
STAT=/proc/kill_block/stat
PARALLEL_SENDERS=8
TESTS=19
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
		modprobe -r kill_block 2>/dev/null || true
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

run_parallel_senders()
{
	sender_pids=
	i=0
	while [ "$i" -lt "$PARALLEL_SENDERS" ]; do
		"$helper" send "$sender_comm" "$target_pid" 15 \
			>/dev/null 2>&1 &
		sender_pids="$sender_pids $!"
		i=$((i + 1))
	done

	rc=0
	for sender_pid in $sender_pids; do
		if wait "$sender_pid"; then
			sender_rc=0
		else
			sender_rc=$?
		fi
		[ "$sender_rc" -eq 1 ] || rc=1
	done

	return "$rc"
}

reload_module()
{
	[ "$module_loaded" -eq 1 ] || return "$KSFT_SKIP"
	write_sysctl 0 || return 1
	write_rule "del $sender_comm $target_comm *" || true

	i=0
	while [ "$i" -lt 3 ]; do
		modprobe -r kill_block 2>/dev/null || return 1
		module_loaded=0
		modprobe kill_block 2>/dev/null || return 1
		module_loaded=1
		[ -w "$SYSCTL" ] && [ -r "$RULES" ] && [ -w "$RULES" ] &&
			[ -r "$STAT" ] || return 1
		i=$((i + 1))
	done

	return 0
}

cleanup()
{
	write_sysctl 0 || true
	write_rule "del $sender_comm $target_comm *" || true
	if [ -n "$target_pid" ] && kill -0 "$target_pid" 2>/dev/null; then
		kill -KILL "$target_pid" 2>/dev/null || true
		wait "$target_pid" 2>/dev/null || true
	fi
	write_sysctl "$old_sysctl" || true
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r kill_block 2>/dev/null || true
	fi
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || ksft_skip_all "root privileges are required"
[ -x "$helper" ] || ksft_skip_all "signal_test helper is missing"
if [ ! -e "$RULES" ] && command -v modprobe >/dev/null 2>&1; then
	if modprobe kill_block 2>/dev/null; then
		module_loaded=1
	fi
fi
[ -w "$SYSCTL" ] && [ -r "$RULES" ] && [ -w "$RULES" ] &&
	[ -r "$STAT" ] || skip_unavailable \
	"CONFIG_TKERNEL_KILL_BLOCK is not enabled"

old_sysctl=$(cat "$SYSCTL")
target_comm="kb_t_$$"
sender_comm="kb_s_$$"
trap cleanup EXIT
trap 'exit 1' INT TERM

write_sysctl 0 || ksft_skip_all "kill block sysctl is not writable"
start_target || ksft_skip_all "could not start the signal target"
cat "$STAT" >/dev/null
ksft_plan "$TESTS"

write_sysctl 1
ksft_result $? "kill blocking can be enabled"

"$helper" send "$sender_comm" "$target_pid" 15 >/dev/null 2>&1
rc=$?
[ "$rc" -eq 1 ]
ksft_result $? "SIGTERM without a matching whitelist rule is rejected"

kill -0 "$target_pid" 2>/dev/null
ksft_result $? "the blocked signal leaves the process alive"

blocked=$(awk '{ total += $2 } END { print total + 0 }' "$STAT")
[ "$blocked" -gt 0 ]
ksft_result $? "the blocked-signal counter is incremented"

run_parallel_senders
rc=$?
kill -0 "$target_pid" 2>/dev/null || rc=1
ksft_result "$rc" "concurrent SIGTERM attempts are blocked"

parallel_after=$(awk '{ total += $2 } END { print total + 0 }' "$STAT")
[ "$parallel_after" -ge "$PARALLEL_SENDERS" ]
ksft_result $? "concurrent blocked signals are all accounted"

write_rule "add $sender_comm $target_comm *"
ksft_result $? "a matching whitelist rule can be added"

awk -v src="$sender_comm" -v dst="$target_comm" \
	'NR > 1 && $1 == src && $2 == dst && $3 == "*" { found = 1 }
	 END { exit !found }' "$RULES"
ksft_result $? "the whitelist reports the added rule"

"$helper" send "$sender_comm" "$target_pid" 15 >/dev/null 2>&1
rc=$?
if [ "$rc" -eq 0 ] && wait_for_exit; then
	target_pid=
	rc=0
else
	rc=1
fi
ksft_result "$rc" "a matching whitelist rule allows SIGTERM"

write_rule "del $sender_comm $target_comm *"
ksft_result $? "the whitelist rule can be removed"

if write_rule "del $sender_comm $target_comm *"; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "removing a missing whitelist rule is rejected"

write_rule "add $sender_comm $target_comm *"
if write_rule "add $sender_comm $target_comm *"; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "a duplicate whitelist rule is rejected"

rule_cnt=$(awk 'END { print NR - 1 }' "$RULES")
[ "$rule_cnt" -eq 1 ]
ksft_result $? "a rejected duplicate leaves the rule list unchanged"

long_cgrp=$(printf 'a%.0s' $(seq 70))
write_rule "add $sender_comm $target_comm $long_cgrp"
cut -f3 "$RULES" | tail -n +2 | grep -qx "$(printf '%s' "$long_cgrp" | cut -c 1-63)"
rc=$?
if [ "$rc" -eq 0 ]; then
	write_rule "del $sender_comm $target_comm $(printf '%s' "$long_cgrp" | cut -c 1-63)" ||
		rc=1
fi
ksft_result "$rc" "an oversized cgroup token is truncated to 63 chars"

write_rule "flush"
rc=$?
rule_cnt=$(awk 'END { print NR - 1 }' "$RULES")
[ "$rule_cnt" -eq 0 ] || rc=1
ksft_result "$rc" "flush clears the whitelist"

if write_rule "del $sender_comm $target_comm *"; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "rules are gone after flush"

start_target || ksft_skip_all "could not restart the signal target"
write_rule "add $sender_comm $target_comm *"
"$helper" send "$sender_comm" "$target_pid" 15 >/dev/null 2>&1
rc=$?
[ "$rc" -eq 0 ] && wait_for_exit && { target_pid=; rc=0; } || rc=1
ksft_result "$rc" "a rule added after flush works again"

start_target || ksft_skip_all "could not restart the signal target"
write_sysctl 1 >/dev/null

if write_sysctl 3; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "an out-of-range mode is rejected"

reload_module
rc=$?
if [ "$rc" -eq "$KSFT_SKIP" ]; then
	ksft_result_skip "kill block survives repeated module reloads" \
		"the module was loaded before the test"
else
	ksft_result "$rc" "kill block survives repeated module reloads"
fi

ksft_finished
