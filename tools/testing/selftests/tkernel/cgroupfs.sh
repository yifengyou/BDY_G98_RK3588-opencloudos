#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

CGROUPFS_ROLE_POD_GROUPS=1
CPU_QUOTA="150000 100000"
MEMORY_LIMIT=67108864
MEMORY_LIMIT_KB=65536
TESTS=8

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
script_path="$script_dir/$(basename -- "$0")"
. "$script_dir/tkernel.sh"

expand_cpulist()
{
	echo "$1" | tr ',' '\n' | while IFS=- read -r first last; do
		if [ -z "$last" ]; then
			echo "$first"
		else
			while [ "$first" -le "$last" ]; do
				echo "$first"
				first=$((first + 1))
			done
		fi
	done
}

cpulist_count()
{
	expand_cpulist "$1" | awk 'NF { count++ } END { print count + 0 }'
}

write_value()
{
	printf '%s\n' "$2" > "$1" 2>/dev/null
}

inner_cleanup()
{
	umount "$view_root" 2>/dev/null || true
}

run_inner()
{
	leaf=$2
	view_root=$3
	expected_cpus=$4
	effective_cpus=$5
	host_cpu_count=$6
	host_memtotal=$7

	trap inner_cleanup EXIT HUP INT TERM
	mount --make-rprivate / || ksft_skip_all "cannot isolate mount propagation"
	write_value "$leaf/cgroup.procs" "$$" ||
		ksft_skip_all "cannot move test process into delegated cgroup"
	mount -t cgroupfs cgroupfs "$view_root" ||
		ksft_skip_all "cannot mount cgroupfs"

	ksft_plan "$TESTS"

	cpuinfo_count=$(grep -c '^processor[[:space:]]*:' \
		"$view_root/proc/cpuinfo")
	[ "$cpuinfo_count" -eq "$expected_cpus" ]
	ksft_result "$?" "cpuinfo applies the pod CPU quota"

	stat_count=$(grep -c '^cpu[0-9][0-9]*[[:space:]]' \
		"$view_root/proc/stat")
	[ "$stat_count" -eq "$expected_cpus" ]
	ksft_result "$?" "proc stat does not expose extra CPUs"

	online=$(cat "$view_root/sys/devices/system/cpu/online")
	online_count=$(cpulist_count "$online")
	dir_count=0
	for cpu_dir in "$view_root"/sys/devices/system/cpu/cpu[0-9]*; do
		[ -e "$cpu_dir" ] || continue
		dir_count=$((dir_count + 1))
	done
	[ "$online_count" -eq "$expected_cpus" ] &&
		[ "$dir_count" -eq "$expected_cpus" ]
	ksft_result "$?" "sysfs online mask and CPU directories are filtered"

	quota=$(cat "$view_root/cgroup/cpu.quota_period_us")
	[ "$quota" = "$CPU_QUOTA" ]
	ksft_result "$?" "CPU quota and period are reported"

	memtotal=$(awk '/^MemTotal:/ { print $2 }' \
		"$view_root/proc/meminfo")
	[ "$memtotal" -eq "$MEMORY_LIMIT_KB" ]
	ksft_result "$?" "MemTotal follows the pod memory limit"

	configured=$(cat "$leaf/cpuset.cpus")
	effective=$(cat "$leaf/cpuset.cpus.effective")
	effective_count=$(cpulist_count "$effective")
	[ -z "$configured" ] && [ "$effective_count" -eq "$effective_cpus" ]
	ksft_result "$?" "an empty child cpuset inherits all effective pod CPUs"

	current_host_cpus=$(grep -c '^processor[[:space:]]*:' /proc/cpuinfo)
	current_host_memtotal=$(awk '/^MemTotal:/ { print $2 }' /proc/meminfo)
	[ "$current_host_cpus" -eq "$host_cpu_count" ] &&
		[ "$current_host_memtotal" -eq "$host_memtotal" ]
	ksft_result "$?" "the host proc view remains unchanged"

	umount "$view_root"
	first_unmount=$?
	[ "$(cat /proc/sys/kernel/cgroupfs_mounted)" -eq 0 ]
	cleared=$?
	mount -t cgroupfs cgroupfs "$view_root"
	remounted=$?
	[ -r "$view_root/proc/cpuinfo" ]
	readable=$?
	umount "$view_root"
	second_unmount=$?
	[ "$first_unmount" -eq 0 ] && [ "$cleared" -eq 0 ] &&
		[ "$remounted" -eq 0 ] && [ "$readable" -eq 0 ] &&
		[ "$second_unmount" -eq 0 ] &&
		[ "$(cat /proc/sys/kernel/cgroupfs_mounted)" -eq 0 ]
	ksft_result "$?" "unmount and remount leave a reusable clean instance"

	trap - EXIT HUP INT TERM
	ksft_finished
}

test_root=
cgroup_root=
cgroup_mounted=0
root_subtree_before=

cleanup()
{
	if [ -n "$cgroup_root" ]; then
		rmdir "$cgroup_root/pod/leaf" 2>/dev/null || true
		rmdir "$cgroup_root/pod" 2>/dev/null || true
		rmdir "$cgroup_root" 2>/dev/null || true
	fi
	if [ "$cgroup_mounted" -eq 1 ]; then
		for controller in cpu cpuset memory; do
			case " $root_subtree_before " in
			*" $controller "*) ;;
			*) write_value "$test_root/cgroup.subtree_control" \
				"-$controller" || true ;;
			esac
		done
		umount "$test_root" 2>/dev/null || true
	fi
	if [ -n "$test_root" ]; then
		rmdir "$test_root" 2>/dev/null || true
		test_parent=$(dirname -- "$test_root")
		rmdir "$test_parent/view" 2>/dev/null || true
		rmdir "$test_parent" 2>/dev/null || true
	fi
}

if [ "$1" = "--inner" ]; then
	run_inner "$@"
	exit $?
fi

[ "$(id -u)" -eq 0 ] || ksft_skip_all "root is required"
for command in awk grep mount umount unshare; do
	command -v "$command" >/dev/null 2>&1 ||
		ksft_skip_all "$command is required"
done
grep -qw cgroupfs /proc/filesystems ||
	ksft_skip_all "CONFIG_CGROUPFS is not enabled"
[ -r /proc/sys/kernel/cgroupfs_mounted ] ||
	ksft_skip_all "cgroupfs lifecycle sysctl is unavailable"
[ "$(cat /proc/sys/kernel/cgroupfs_mounted)" -eq 0 ] ||
	ksft_skip_all "another cgroupfs instance is already mounted"

test_parent=$(mktemp -d /tmp/cgroupfs-selftest.XXXXXX) ||
	ksft_skip_all "cannot create temporary directory"
test_root="$test_parent/cgroup2"
view_root="$test_parent/view"
mkdir "$test_root" "$view_root" || ksft_skip_all "cannot create mountpoints"
trap cleanup EXIT HUP INT TERM

mount -t cgroup2 cgroup2 "$test_root" ||
	ksft_skip_all "cannot mount cgroup v2"
cgroup_mounted=1
[ -w "$test_root/cgroup.subtree_control" ] ||
	ksft_skip_all "cgroup v2 hierarchy is not writable"
for controller in cpu cpuset memory; do
	grep -qw "$controller" "$test_root/cgroup.controllers" ||
		ksft_skip_all "$controller controller is unavailable"
done

root_subtree_before=$(cat "$test_root/cgroup.subtree_control")
for controller in cpu cpuset memory; do
	write_value "$test_root/cgroup.subtree_control" "+$controller" ||
		ksft_skip_all "cannot enable $controller controller"
done

cgroup_root="$test_root/cgroupfs-selftest-$$"
mkdir "$cgroup_root" || ksft_skip_all "cannot create test cgroup"
write_value "$cgroup_root/cgroup.subtree_control" "+cpu +cpuset +memory" ||
	ksft_skip_all "cannot delegate controllers to the pod cgroup"

available=$(cat "$cgroup_root/cpuset.cpus.effective")
set -- $(expand_cpulist "$available")
[ "$#" -ge 3 ] || ksft_skip_all "at least three effective CPUs are required"
pod_cpus="$1,$2,$3"
pod_mems=$(cat "$cgroup_root/cpuset.mems.effective")
[ -n "$pod_mems" ] || ksft_skip_all "no effective NUMA memory node"

pod="$cgroup_root/pod"
leaf="$pod/leaf"
mkdir "$pod" || ksft_skip_all "cannot create pod cgroup"
[ -e "$pod/cgroup.role" ] || ksft_skip_all "cgroup.role is unavailable"
write_value "$pod/cpuset.mems" "$pod_mems" ||
	ksft_skip_all "cannot configure pod memory nodes"
write_value "$pod/cpuset.cpus" "$pod_cpus" ||
	ksft_skip_all "cannot configure pod CPUs"
write_value "$pod/cpu.max" "$CPU_QUOTA" ||
	ksft_skip_all "cannot configure CPU quota"
write_value "$pod/cpu.quota_aware" 1 ||
	ksft_skip_all "cpu.quota_aware is unavailable"
write_value "$pod/memory.max" "$MEMORY_LIMIT" ||
	ksft_skip_all "cannot configure memory limit"
write_value "$pod/cgroup.role" "$CGROUPFS_ROLE_POD_GROUPS" ||
	ksft_skip_all "cannot assign the pod cgroup role"
write_value "$pod/cgroup.subtree_control" "+cpu +cpuset +memory" ||
	ksft_skip_all "cannot delegate controllers to the leaf cgroup"
mkdir "$leaf" || ksft_skip_all "cannot create leaf cgroup"

host_cpu_count=$(grep -c '^processor[[:space:]]*:' /proc/cpuinfo)
host_memtotal=$(awk '/^MemTotal:/ { print $2 }' /proc/meminfo)

unshare -m "$script_path" --inner "$leaf" "$view_root" 2 3 \
	"$host_cpu_count" "$host_memtotal"
result=$?

trap - EXIT HUP INT TERM
cleanup
exit "$result"
