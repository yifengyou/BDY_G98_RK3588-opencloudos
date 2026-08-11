.. SPDX-License-Identifier: GPL-2.0-only

=====================
Cgroup monitor buffer
=====================

Overview
========

The monitor buffer (mbuf) is a diagnostic ring buffer used by in-kernel
Resource Quality Monitor (RQM) producers.  With ``CONFIG_RQM=y``, an mbuf
slot can be assigned to each eligible cgroup and exposed through the
read-only ``mbuf`` file in that cgroup's directory.

Mbuf is not a cgroup resource controller.  It does not account or limit
network buffers, does not add files such as ``mbuf.max`` or ``mbuf.stat``,
and must not be added to ``cgroup.subtree_control``.

Configuration
=============

The cgroup interface requires::

    CONFIG_PSI=y
    CONFIG_RQM=y

``CONFIG_RQM`` depends on ``CONFIG_PSI``.  The optional
``CONFIG_NETNS_MBUF`` setting adds a monitor buffer for each network
namespace and exposes it as ``/proc/net/twatcher/log``.  Network namespace
buffers and cgroup buffers share the global slot pool.

The following kernel command-line parameters size that pool:

``mbuf_len=<size>``
    Total buffer size.  The default is 4 MiB.  Values are rounded up to a
    power of two and clamped to the range 2 MiB through 8 MiB.

``mbuf_max_items=<count>``
    Maximum number of slots shared by cgroups and network namespaces.  The
    default is 1024.  Values are rounded up to a power of two and clamped to
    the range 256 through 1024.

Each slot receives ``mbuf_len / mbuf_max_items`` bytes.  Once all slots are
in use, additional cgroups do not get a buffer and their ``mbuf`` file reads
as empty.

Runtime enable
==============

Cgroup allocation and writes are disabled by default.  Enable them with::

    sysctl -w kernel.qos_mbuf_enable=1

The sysctl accepts 0 or 1 and is writable only by root.  A cgroup receives
its slot when the cgroup is created, so enable the sysctl before creating
the cgroups that need monitoring.  Disabling the sysctl stops cgroup mbuf
producers, but does not control the network namespace interface.

Interface
=========

For a non-root cgroup v2 directory, read the diagnostic snapshot with::

    cat /sys/fs/cgroup/WORKLOAD/mbuf

Each output record has this form::

    <timestamp-ns>:<message>

The timestamp comes from ``local_clock()``.  Non-printable bytes, non-ASCII
bytes, and backslashes are omitted from the message.  Reading the file takes
a snapshot; it does not consume or clear the underlying ring.

The file is not a userspace logging endpoint.  Records appear only when a
kernel feature calls an mbuf producer such as ``mbuf_print()`` or
``mbuf_print_task()``.  Writers are rate-limited per slot, and the oldest
records are discarded when a slot wraps.
