#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

PROC_DIR=/proc/irq_latency
ENABLE=$PROC_DIR/enable
FREQ=$PROC_DIR/freq_ms
THRESHOLD=$PROC_DIR/latency_thresh_ms
TRACE_STACK=$PROC_DIR/trace_stack
TRACE_DIST=$PROC_DIR/trace_dist
TESTS=12
module_loaded=0

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
	write_value "$ENABLE" 0 || true
	# Lowering the frequency first makes every valid saved threshold
	# writable again before the original frequency is restored.
	write_value "$FREQ" 5 || true
	write_value "$THRESHOLD" "$old_threshold" || true
	write_value "$FREQ" "$old_freq" || true
	write_value "$ENABLE" "$old_enable" || true
	if [ "$module_loaded" -eq 1 ]; then
		modprobe -r irqlatency 2>/dev/null || true
	fi
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

ksft_finished
