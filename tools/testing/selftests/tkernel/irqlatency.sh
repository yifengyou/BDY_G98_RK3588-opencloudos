#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

PROC_DIR=/proc/irq_latency
ENABLE=$PROC_DIR/enable
FREQ=$PROC_DIR/freq_ms
THRESHOLD=$PROC_DIR/latency_thresh_ms
TRACE_STACK=$PROC_DIR/trace_stack
TRACE_DIST=$PROC_DIR/trace_dist
TESTS=15
module_loaded=0
cpu_offlined=0
hotplug_file=
reader_pid=

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$script_dir/tkernel.sh"

write_value()
{
	printf '%s\n' "$2" > "$1" 2>/dev/null
}

skip_unavailable()
{
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r irqlatency 2>/dev/null || true
	fi
	ksft_skip_all "$1"
}

cleanup()
{
	if [ -n "$reader_pid" ]; then
		kill "$reader_pid" 2>/dev/null || true
		wait "$reader_pid" 2>/dev/null || true
	fi
	if [ "$cpu_offlined" -eq 1 ] && [ -n "$hotplug_file" ]; then
		write_value "$hotplug_file" 1 || true
	fi
	if [ -e "$ENABLE" ]; then
		write_value "$ENABLE" 0 || true
		# Lowering the frequency first makes every valid saved threshold
		# writable again before the original frequency is restored.
		write_value "$FREQ" 5 || true
		write_value "$THRESHOLD" "$old_threshold" || true
		write_value "$FREQ" "$old_freq" || true
		write_value "$ENABLE" "$old_enable" || true
	fi
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r irqlatency 2>/dev/null || true
	fi
}

toggle_enable()
{
	i=0
	while [ "$i" -lt 10 ]; do
		write_value "$ENABLE" 1 || return 1
		write_value "$ENABLE" 2 || return 1
		write_value "$ENABLE" 0 || return 1
		i=$((i + 1))
	done
}

find_hotplug_cpu()
{
	for path in /sys/devices/system/cpu/cpu[0-9]*/online; do
		[ -w "$path" ] || continue
		[ "$(cat "$path" 2>/dev/null)" = 1 ] || continue
		cpu=${path%/online}
		cpu=${cpu##*/}
		cpu=${cpu#cpu}
		[ "$cpu" -ne 0 ] || continue
		printf '%s\n' "$path"
		return 0
	done

	return 1
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || ksft_skip_all "root privileges are required"
if [ ! -e "$ENABLE" ] && command -v modprobe >/dev/null 2>&1; then
	if modprobe irqlatency 2>/dev/null; then
		module_loaded=1
	fi
fi
for file in "$ENABLE" "$FREQ" "$THRESHOLD" "$TRACE_STACK"; do
	[ -r "$file" ] && [ -w "$file" ] || skip_unavailable \
		"CONFIG_TKERNEL_IRQ_LATENCY is not enabled"
done
[ -r "$TRACE_DIST" ] || skip_unavailable \
	"CONFIG_TKERNEL_IRQ_LATENCY is not enabled"

old_enable=$(cat "$ENABLE")
old_freq=$(cat "$FREQ")
old_threshold=$(cat "$THRESHOLD")
trap cleanup EXIT
trap 'exit 1' INT TERM
ksft_plan "$TESTS"

write_value "$ENABLE" 0
rc=$?
[ "$rc" -eq 0 ] && [ "$(cat "$ENABLE")" -eq 0 ]
ksft_result $? "latency detection can be disabled"

if write_value "$ENABLE" 3; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "an out-of-range enable mode is rejected"

write_value "$FREQ" 1
rc=$?
[ "$rc" -eq 0 ] && [ "$(cat "$FREQ")" -eq 5 ]
ksft_result $? "a frequency below the minimum is clamped"

write_value "$THRESHOLD" 1
rc=$?
[ "$rc" -eq 0 ] && [ "$(cat "$THRESHOLD")" -eq 10 ]
ksft_result $? "the threshold remains at least twice the frequency"

write_value "$THRESHOLD" 40 && write_value "$FREQ" 10
rc=$?
[ "$rc" -eq 0 ] && [ "$(cat "$THRESHOLD")" -eq 40 ] &&
	[ "$(cat "$FREQ")" -eq 10 ]
ksft_result $? "frequency and threshold values can be configured"

write_value "$ENABLE" 1
rc=$?
[ "$rc" -eq 0 ] && [ "$(cat "$ENABLE")" -eq 1 ]
ksft_result $? "latency detection can be enabled"

if write_value "$FREQ" 20; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "frequency changes are rejected while enabled"

grep -q '^irq_latency_ms: 40$' "$TRACE_STACK" &&
	grep -q '^ irq:$' "$TRACE_STACK" &&
	grep -q '^ softirq:$' "$TRACE_STACK"
ksft_result $? "the stack report contains the expected sections"

write_value "$TRACE_STACK" 0
ksft_result $? "recorded latency data can be cleared"

if write_value "$TRACE_STACK" 1; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "a nonzero trace clear command is rejected"

cat "$TRACE_DIST" >/dev/null
ksft_result $? "the latency distribution is readable"

write_value "$ENABLE" 0
rc=$?
[ "$rc" -eq 0 ] && [ "$(cat "$ENABLE")" -eq 0 ]
ksft_result $? "latency detection can be stopped after use"

pids=
for worker in 1 2 3 4; do
	toggle_enable &
	pids="$pids $!"
done
rc=0
for pid in $pids; do
	wait "$pid" || rc=1
done
write_value "$ENABLE" 0 || rc=1
[ "$(cat "$ENABLE")" -eq 0 ] || rc=1
ksft_result "$rc" "concurrent state transitions remain consistent"

hotplug_file=$(find_hotplug_cpu || true)
if [ -z "$hotplug_file" ]; then
	ksft_result_skip "CPU hotplug preserves detector state" \
		"no writable secondary CPU online control"
else
	rc=0
	write_value "$ENABLE" 1 || rc=1
	if [ "$rc" -eq 0 ] && write_value "$hotplug_file" 0; then
		cpu_offlined=1
		[ "$(cat "$hotplug_file")" = 0 ] || rc=1
		cat "$TRACE_STACK" >/dev/null || rc=1
		cat "$TRACE_DIST" >/dev/null || rc=1
		if write_value "$hotplug_file" 1; then
			cpu_offlined=0
		else
			rc=1
		fi
		[ "$(cat "$hotplug_file" 2>/dev/null)" = 1 ] || rc=1
		[ "$(cat "$ENABLE")" = 1 ] || rc=1
		write_value "$ENABLE" 0 || rc=1
		ksft_result "$rc" "CPU hotplug preserves detector state"
	else
		write_value "$ENABLE" 0 || true
		hotplug_file=
		ksft_result_skip "CPU hotplug preserves detector state" \
			"the kernel rejected CPU offline"
	fi
fi

if [ "$module_loaded" -ne 1 ]; then
	ksft_result_skip "enabled detector unload is safe" \
		"the test did not load a removable module"
else
	rc=0
	write_value "$ENABLE" 1 || rc=1
	(
		i=0
		while [ "$i" -lt 100 ] && [ -r "$TRACE_STACK" ]; do
			cat "$TRACE_STACK" >/dev/null 2>&1 || break
			i=$((i + 1))
		done
	) &
	reader_pid=$!
	modprobe -r irqlatency 2>/dev/null || rc=1
	wait "$reader_pid" 2>/dev/null || true
	reader_pid=
	if [ "$rc" -eq 0 ]; then
		module_loaded=0
		[ ! -e "$PROC_DIR" ] || rc=1
	fi
	ksft_result "$rc" "enabled detector unload is safe"
fi

ksft_finished
