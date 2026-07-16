==================
Lockup tracepoints
==================

:Author: Kernel Tracing
:License: GPL-2.0

Overview
========

The lockup detectors in ``kernel/watchdog.c`` expose four tracepoints
that allow filtering, aggregation, and stack capture via the standard
ftrace event framework::

  events/lockup/
    softlockup_sample   - per-tick observation event (soft IRQ ctx)
    softlockup_warn     - emitted at the softlockup report threshold
    hardlockup_warn     - emitted by the NMI hardlockup detector
    long_oncpu_sample   - per-task on-CPU duration event

All four are off by default. When disabled the cost is one
``static_branch`` jump in the relevant detector path.

Trigger contexts
================

``softlockup_sample`` fires once per ``watchdog_hrtimer`` tick on every
CPU that has the softlockup detector enabled, carrying ``cpu``,
``stuck_ns`` (ns since last reschedule), ``thresh_ms``, ``pid``,
``comm`` and ``ip`` (preempted instruction pointer; 0 if unavailable).

Cadence:

* legacy seconds path (``watchdog_thresh_ms == 0``): every
  ``watchdog_thresh * 2 / 5`` seconds (4 s at default thresh=10)
* sub-second path (``watchdog_thresh_ms > 0``): every
  ``thresh_ms * 2 / 5`` ms (200 ms at thresh_ms=500)

``softlockup_warn`` fires from the same hrtimer when the report
threshold is exceeded, in lockstep with the ``BUG: soft lockup``
``pr_emerg``. Same field set as ``softlockup_sample``.

``hardlockup_warn`` fires from ``watchdog_hardlockup_check()`` in NMI
context, in lockstep with the ``Watchdog detected hard LOCKUP``
``pr_emerg``. Fields: ``cpu``, ``pid``, ``comm``, ``ip`` (``pid=0`` /
``comm="?"`` / ``ip=0`` when reporting a remote CPU, since cross-CPU
rq access is unsafe in NMI).

Sub-second softlockup with ``watchdog_thresh_ms``
=================================================

``watchdog_thresh`` is in whole seconds, so the softlockup report
threshold cannot drop below 2 s. To detect kernel-mode windows below
1 s, set ``watchdog_thresh_ms``::

  echo 500 > /proc/sys/kernel/watchdog_thresh_ms
  # hrtimer period: 200 ms; softlockup report threshold: 1 s

``softlockup_sample`` carries true ns-precision ``stuck_ns`` either
way, so you can observe sub-second windows even without flipping
``thresh_ms``.

"Long on-CPU" detection with ``long_oncpu_sample``
==================================================

``softlockup`` resets at every context switch, so a task that politely
calls ``cond_resched()`` but always wins the rebid never triggers it.
``long_oncpu_sample`` fills that gap: it resets only on a real context
switch into a different task. Enable via::

  echo 100 > /proc/sys/kernel/long_oncpu_thresh_ms
  echo 1   > /sys/kernel/tracing/events/lockup/long_oncpu_sample/enable

Fields: ``cpu``, ``oncpu_ns`` (ns @current has been continuously on
CPU, approximated by ``local_clock - p->se.exec_start``), ``thresh_ms``,
``pid``, ``comm``, ``ip``.

Recipes
=======

1. Enable per-tick observation::

     echo 1 > /sys/kernel/tracing/events/lockup/softlockup_sample/enable
     cat /sys/kernel/tracing/trace_pipe

2. Record only "long" samples (>= 200 ms)::

     echo 'stuck_ns >= 200000000' > \
       /sys/kernel/tracing/events/lockup/softlockup_sample/filter

3. Capture the preempted stack inside long windows::

     echo 'stacktrace:50 if stuck_ns >= 300000000' > \
       /sys/kernel/tracing/events/lockup/softlockup_sample/trigger

4. Aggregate "which kernel function is most often on-CPU during long
   windows"::

     echo 'hist:keys=ip.sym,comm:vals=hitcount,stuck_ns' \
          ':sort=stuck_ns.descending' \
       > /sys/kernel/tracing/events/lockup/softlockup_sample/trigger

5. Catch the formal softlockup warning and its stack::

     echo 1 > /sys/kernel/tracing/events/lockup/softlockup_warn/enable
     echo 'stacktrace' > \
       /sys/kernel/tracing/events/lockup/softlockup_warn/trigger

Caveats
=======

* ``softlockup_sample`` defaults to off; even disabled it costs only
  one ``static_branch`` jump per hrtimer tick. At
  ``watchdog_thresh_ms = 100`` (20 ms tick) per-CPU rate is 50 events/s
  -- filter aggressively.

* ``hardlockup_warn`` runs in NMI context and does not read the
  locked-up remote CPU's ``current`` (rq race). For remote reports,
  ``pid``, ``comm`` and ``ip`` are zero/``"?"``; rely on dmesg for the
  remote stack.

* The native ``stacktrace`` trigger captures the watchdog hrtimer
  callback's own stack -- which IS the stack of whatever was running
  on-CPU when the timer fired, so it works for "what was on-CPU during
  a long window".

* The legacy ``softlockup_panic`` and ``hardlockup_panic`` paths are
  unchanged; the tracepoints fire just before the panic message.
