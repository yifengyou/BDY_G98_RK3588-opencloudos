#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

ASYNC=/proc/sys/vm/memcg_async
MEMORY_QOS=/proc/sys/vm/memory_qos
MEMCG_ASYNC_QOS_MAGIC=58324299
TESTS=7
rue_testmod_loaded=0

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$script_dir/tkernel.sh"

write_value()
{
	printf '%s\n' "$2" > "$1" 2>/dev/null
}

cleanup()
{
	if [ "$rue_testmod_loaded" -eq 1 ]; then
		write_value "$ASYNC" 0 || true
		rmmod rue_testmod 2>/dev/null || true
	else
		write_value "$ASYNC" "$old_async" || true
	fi
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

write_value "$ASYNC" 0
ksft_result $? "asynchronous memcg reclaim can be disabled after use"

if [ "$expected_qos" -eq "$MEMCG_ASYNC_QOS_MAGIC" ]; then
	expected_qos=0
fi
[ "$(cat "$MEMORY_QOS")" -eq "$expected_qos" ]
ksft_result $? "disabling removes only the implicit memory QoS setting"

ksft_finished
