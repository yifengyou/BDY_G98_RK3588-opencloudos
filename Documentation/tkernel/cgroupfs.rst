.. SPDX-License-Identifier: GPL-2.0-only

================================
Cgroup resource view file system
================================

``cgroupfs`` presents a cgroup-scoped alternative to selected host resource
files.  It is intended for a container runtime that needs CPU and memory
information to reflect a pod cgroup rather than the whole host.  It does not
replace the kernel's cgroup v1 or v2 control file system.

Requirements
============

The feature is selected with ``CONFIG_CGROUPFS``.  Its Kconfig dependencies
include x86, cgroups, sysfs, CFS bandwidth control, and cpusets.  A useful
container memory view also requires the memory controller.

The cgroup hierarchy must provide the ``cpu``, ``cpuset``, and ``memory``
controllers.  The pod cgroup uses these OpenCloudOS-specific files:

``cgroup.role``
  Set this to ``1`` on the cgroup that defines the pod boundary.  Descendants
  inherit the role.  Resource lookups made by a task in a descendant stop at
  the outermost consecutive cgroup with this role.

``cpu.quota_aware``
  Set this to ``1`` for ``cpu.max`` to constrain the number of CPUs reported
  by cgroupfs.  The reported count is the smaller of the effective cpuset and
  the rounded-up CPU quota, except for the documented one-CPU fallback in the
  implementation.

Mounting and layout
===================

After configuring a pod and placing the container task in a descendant
cgroup, mount the resource view in the container's mount namespace::

    mount -t cgroupfs cgroupfs /run/cgroupfs-view

The mount contains::

    proc/cpuinfo
    proc/meminfo
    proc/stat
    proc/uptime
    proc/loadavg
    proc/vmstat
    sys/devices/system/cpu/online
    sys/devices/system/cpu/cpu*/
    cgroup/cpu.quota_period_us

For example, a runtime may bind the files under ``proc`` and ``sys`` over the
matching paths in a container after mounting its normal procfs and sysfs.
Reading the host's original ``/proc`` and ``/sys`` paths is unaffected.

An empty ``cpuset.cpus`` in a child cgroup uses the inherited effective cpuset.
The view still resolves the resource boundary at the role-marked pod cgroup.
``proc/meminfo`` reports the pod's ``memory.max`` as ``MemTotal`` and derives
the remaining values from that memory cgroup.

Only one cgroupfs superblock exists at a time.  Unmount the view when the
container is destroyed.  ``/proc/sys/kernel/cgroupfs_mounted`` reports whether
an instance is currently mounted and is useful for lifecycle diagnostics.

Selftest
========

Run the contract test as root with at least three available CPUs::

    make -C tools/testing/selftests TARGETS=tkernel run_tests

``cgroupfs.sh`` creates a temporary cgroup v2 pod and an empty-cpuset child,
then mounts cgroupfs in a private mount namespace.  It checks the CPU, memory,
quota, host-view, and unmount/remount contracts.  All cgroups and mounts are
removed when the test exits.

The test reports kselftest ``SKIP`` when cgroupfs, cgroup v2, a required
controller, ``cgroup.role``, root privileges, mount-namespace support, or
three effective CPUs are unavailable.  A skip is not a pass and should not be
used as evidence that the resource view was exercised.
