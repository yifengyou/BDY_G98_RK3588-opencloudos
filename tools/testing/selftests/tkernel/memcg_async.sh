#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

ASYNC=/proc/sys/vm/memcg_async
MEMORY_QOS=/proc/sys/vm/memory_qos
MEMCG_ASYNC_QOS_MAGIC=58324299
CGROUP_ROOT=/sys/fs/cgroup
PRESSURE_HIGH=67108864
PRESSURE_SIZE_MB=48
TESTS=10
rue_testmod_loaded=0
cgroup_dir=
pressure_file=

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$script_dir/tkernel.sh"

write_value()
{
	printf '%s\n' "$2" > "$1" 2>/dev/null
}

cleanup()
{
	if [ -n "$pressure_file" ]; then
		rm -f "$pressure_file"
	fi
	if [ -n "$cgroup_dir" ]; then
		rmdir "$cgroup_dir" 2>/dev/null || true
	fi
	if [ "$rue_testmod_loaded" -eq 1 ]; then
		write_value "$ASYNC" 0 || true
		rmmod rue_testmod 2>/dev/null || true
	else
		write_value "$ASYNC" "$old_async" || true
	fi
}

setup_memory_pressure()
{
	[ -f "$CGROUP_ROOT/cgroup.controllers" ] || return "$KSFT_SKIP"
	grep -qw memory "$CGROUP_ROOT/cgroup.controllers" ||
		return "$KSFT_SKIP"
	grep -qw memory "$CGROUP_ROOT/cgroup.subtree_control" ||
		return "$KSFT_SKIP"

	cgroup_dir="$CGROUP_ROOT/tkernel_memcg_$$"
	pressure_file="/tmp/tkernel_memcg_$$"
	mkdir "$cgroup_dir" 2>/dev/null || return "$KSFT_SKIP"
	for file in memory.high memory.current memory.events \
		memory.async_ratio memory.async_high memory.async_low \
		memory.async_distance_factor; do
		[ -r "$cgroup_dir/$file" ] || return "$KSFT_SKIP"
	done

	write_value "$cgroup_dir/memory.high" "$PRESSURE_HIGH" || return 1
	write_value "$cgroup_dir/memory.async_ratio" 50 || return 1
	write_value "$cgroup_dir/memory.async_distance_factor" 100000 ||
		return 1

	async_high=$(cat "$cgroup_dir/memory.async_high")
	async_low=$(cat "$cgroup_dir/memory.async_low")
	[ "$async_high" -eq 33554432 ] && [ "$async_low" -gt 0 ] &&
		[ "$async_low" -lt "$async_high" ]
}

generate_memory_pressure()
{
	events_before=$(awk '$1 == "high" { print $2 }' \
		"$cgroup_dir/memory.events")
	sh -c 'echo $$ > "$1/cgroup.procs"; dd if=/dev/zero of="$2" \
		bs=1M count="$3" conv=fsync 2>/dev/null' sh \
		"$cgroup_dir" "$pressure_file" "$PRESSURE_SIZE_MB" || return 1

	i=0
	while [ "$i" -lt 200 ]; do
		events_after=$(awk '$1 == "high" { print $2 }' \
			"$cgroup_dir/memory.events")
		[ "$events_after" -gt "$events_before" ] && return 0
		i=$((i + 1))
		sleep 0.01
	done

	return 1
}

pressure_is_below_memory_high()
{
	current=$(cat "$cgroup_dir/memory.current")
	[ "$current" -lt "$PRESSURE_HIGH" ]
}

load_rue_testmod()
{
	[ -r "$script_dir/rue_testmod.ko" ] || return 1
	insmod "$script_dir/rue_testmod.ko" 2>/dev/null || return 1
	rue_testmod_loaded=1
}

echo "TAP version 13"

[ "$(id -u)" -eq 0 ] || ksft_skip_all "root privileges are required"
[ -r "$ASYNC" ] && [ -w "$ASYNC" ] &&
	[ -r "$MEMORY_QOS" ] || ksft_skip_all \
	"CONFIG_MEMCG and CONFIG_TKERNEL are required"

old_async=$(cat "$ASYNC")
trap cleanup EXIT
trap 'exit 1' INT TERM

# The handler intentionally rejects writes until a RUE implementation is
# installed. Load the kselftest-only provider when the system has no provider.
if ! write_value "$ASYNC" 0; then
	load_rue_testmod || ksft_skip_all \
		"the RUE provider and rue_testmod.ko are unavailable"
	write_value "$ASYNC" 0 || ksft_skip_all \
		"rue_testmod did not activate the RUE interface"
fi
ksft_plan "$TESTS"

[ "$(cat "$ASYNC")" -eq 0 ]
ksft_result $? "asynchronous memcg reclaim can be disabled"

if write_value "$ASYNC" 2; then
	rc=1
else
	rc=0
fi
ksft_result "$rc" "an out-of-range asynchronous mode is rejected"

qos_before_enable=$(cat "$MEMORY_QOS")
write_value "$ASYNC" 1
ksft_result $? "asynchronous memcg reclaim can be enabled"

[ "$(cat "$ASYNC")" -eq 1 ]
ksft_result $? "the enabled state is reported through sysctl"

if [ "$qos_before_enable" -eq 0 ]; then
	expected_qos=$MEMCG_ASYNC_QOS_MAGIC
else
	expected_qos=$qos_before_enable
fi
[ "$(cat "$MEMORY_QOS")" -eq "$expected_qos" ]
ksft_result $? "enabling activates memory QoS when needed"

setup_memory_pressure
rc=$?
if [ "$rc" -eq "$KSFT_SKIP" ]; then
	ksft_result_skip "asynchronous reclaim watermarks can be configured" \
		"a writable cgroup v2 memory controller is unavailable"
	ksft_result_skip "memory pressure queues asynchronous reclaim" \
		"a writable cgroup v2 memory controller is unavailable"
	ksft_result_skip "asynchronous reclaim starts below memory.high" \
		"a writable cgroup v2 memory controller is unavailable"
elif [ "$rc" -ne 0 ]; then
	ksft_result "$rc" "asynchronous reclaim watermarks can be configured"
	ksft_result_skip "memory pressure queues asynchronous reclaim" \
		"watermark configuration failed"
	ksft_result_skip "asynchronous reclaim starts below memory.high" \
		"watermark configuration failed"
else
	ksft_result 0 "asynchronous reclaim watermarks can be configured"
	generate_memory_pressure
	ksft_result $? "memory pressure queues asynchronous reclaim"
	pressure_is_below_memory_high
	ksft_result $? "asynchronous reclaim starts below memory.high"
fi

write_value "$ASYNC" 0
ksft_result $? "asynchronous memcg reclaim can be disabled after use"

if [ "$expected_qos" -eq "$MEMCG_ASYNC_QOS_MAGIC" ]; then
	expected_qos=0
fi
[ "$(cat "$MEMORY_QOS")" -eq "$expected_qos" ]
ksft_result $? "disabling removes only the implicit memory QoS setting"

ksft_finished
