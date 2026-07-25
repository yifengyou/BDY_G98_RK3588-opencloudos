.. SPDX-License-Identifier: GPL-2.0-only

=====================
Memory Buffer Cgroup
=====================

Overview
========

The mbuf (memory buffer) cgroup controller
(``CONFIG_CGROUP_MBUF``) provides per-cgroup accounting and limiting
of kernel memory buffers (sk_buff and related networking buffers).
It extends the cgroup v2 hierarchy with an ``mbuf`` controller that
tracks the number of buffer slots consumed by each cgroup.

Interface Files
===============

``mbuf.max``
    Maximum number of buffer slots allowed for this cgroup.
    Write ``max`` to remove the limit.

``mbuf.current``
    Current number of buffer slots in use by this cgroup and its
    descendants.

``mbuf.stat``
    Detailed statistics including peak usage, allocation failures,
    and per-NUMA-node breakdown.

Enabling
========

The mbuf controller is available when ``CONFIG_CGROUP_MBUF=y`` is
set.  It must be explicitly enabled on the cgroup v2 hierarchy::

    echo "+mbuf" > /sys/fs/cgroup/cgroup.subtree_control

Use Cases
=========

- Preventing a single container from exhausting kernel networking
  buffers and causing packet drops for other containers
- Fair sharing of kernel buffer memory in multi-tenant hosts
- Monitoring buffer pressure per cgroup for capacity planning

Relationship to Other Controllers
===================================

The mbuf controller is independent of the memory controller
(``CONFIG_MEMCG``).  Buffer memory is accounted separately because
sk_buff allocations use per-CPU pools and SLAB caches that are not
easily attributed to a memcg.  The mbuf controller uses a dedicated
bitmap-based slot allocator for accurate per-cgroup tracking.
