.. SPDX-License-Identifier: GPL-2.0-only

=======================
TKernel Feature Guide
=======================

:Copyright: Tencent Corporation

Overview
========

TKernel is a collection of production-hardened kernel extensions
developed by Tencent for the OpenCloudOS distribution.  The features
are gated behind ``CONFIG_TKERNEL`` and can be individually enabled
or disabled through Kconfig options.

Features
========

Non-privileged Port Binding (``CONFIG_TKERNEL_NONPRIV_NETBIND``)
-----------------------------------------------------------------

Allows unprivileged processes to bind to specific low-numbered ports
(< 1024) that have been explicitly allowed by the administrator
through ``/proc/tkernel/nonpriv_netbind``.

TTools (``CONFIG_TKERNEL_TTOOLS``)
-----------------------------------

Provides ptrace-based process protection.  A process can mark itself
as protected, preventing other processes from attaching via ptrace.
Useful for security-sensitive daemons.

Netatop (``CONFIG_TKERNEL_NETATOP``)
--------------------------------------

Per-task network statistics module (from the atop tool suite).
Exposes per-process network counters through ``/proc/netatop``.
Requires ``CONFIG_NETFILTER``.

Shield Mounts (``CONFIG_TKERNEL_SHIELD_MOUNTS``)
--------------------------------------------------

Allows the administrator to mark specific mount points as "shielded",
preventing unprivileged users from accessing or listing them.
Configured through a proc interface.

Security Monitor / Aegis (``CONFIG_TKERNEL_SECURITY_MONITOR``)
----------------------------------------------------------------

Kernel-level security event monitoring framework.  Captures exec,
socket, and credential events and exposes them through
``/proc/security_monitor/``.  Used by the Tencent Aegis host
security agent.

IRQ Latency Detector (``CONFIG_TKERNEL_IRQ_LATENCY``)
-------------------------------------------------------

Detects and records long IRQ and softirq latencies.  Uses hrtimers
to measure the time between a timer firing and the interrupt handler
actually running.  Records stack traces of the offending code paths
and exposes them through ``/proc/irq_latency/``.

Kill Hook / Kill Block (``CONFIG_TKERNEL_KILL_BLOCK``)
--------------------------------------------------------

Intercepts kill signals and blocks them based on configurable rules.
A whitelist interface at ``/proc/kill_block/whitelist`` allows
administrators to exempt matching source, destination, and cgroup
combinations from blocking.  Statistics are exposed through
``/proc/kill_block/stat``, and the feature is controlled by
``/proc/sys/kernel/sig_kill_block``.  It depends on cgroups for scope
control.

Kill Protect (``CONFIG_TKERNEL_KILL_PROTECT``)
-------------------------------------------------

Protects specific processes from SIGKILL and SIGTERM.  Simpler than
kill block; it matches process command names rather than cgroups.  Rules
are configured through ``/proc/kill_protect/blacklist``, statistics are
reported through ``/proc/kill_protect/stat``, and the feature is
controlled by ``/proc/sys/kernel/sig_kill_protect``.

Async Fork (``CONFIG_TKERNEL_ASYNC_FORK``)
--------------------------------------------

Optimises fork() performance for memory-heavy processes by
performing page-table duplication asynchronously.

Memory Cgroup Async Reclaim (``CONFIG_MEMCG_ASYNC``)
------------------------------------------------------

Enables asynchronous memory reclaim within memory cgroups, reducing
allocation latency spikes for containerised workloads.

Sysctl interfaces
=================

TKernel registers a top-level sysctl directory at
``/proc/sys/tkernel/`` for global feature parameters.  Individual
features can also register controls in standard sysctl directories; the
kill block and kill protect controls are under ``/proc/sys/kernel/``.

Building
========

Enable in ``make menuconfig`` under::

    General setup -> Tencent Kernel Features (TKERNEL)

Individual features can then be toggled within the TKernel submenu.
