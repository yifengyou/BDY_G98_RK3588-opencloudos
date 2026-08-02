.. SPDX-License-Identifier: GPL-2.0-only

====================
IRQ Latency Detector
====================

Overview
========

The IRQ latency detector (``CONFIG_TKERNEL_IRQ_LATENCY``) identifies
code paths that keep hardware interrupts or softirqs disabled for an
excessively long time.  It uses per-CPU hrtimers and kernel timers to
measure the delta between the expected and actual firing time; when
the delta exceeds a configurable threshold, the detector captures the
stack trace of the offending context.

This is invaluable for diagnosing audio glitches, network packet
loss, and watchdog timeouts in production systems where enabling
ftrace or lockdep is too expensive.

Usage
=====

The detector is controlled through ``/proc/irq_latency/``:

``enable``
    Write 1 to start detection, 0 to stop.

``freq_ms``
    Probe frequency in milliseconds (5 -- 5000, default 10).
    Lower values increase detection resolution but add overhead.

``irq_latency_ms``
    Threshold in milliseconds (default 30).  Any IRQ-off or
    softirq-off period longer than this is recorded.

Reading ``/proc/irq_latency/irq`` or ``/proc/irq_latency/softirq``
prints the recorded stack traces with latency values, PID, and
command name of the task that was running when the latency occurred.

Example
=======

::

    # modprobe irqlatency
    echo 10 > /proc/irq_latency/freq_ms
    echo 50 > /proc/irq_latency/irq_latency_ms
    echo 1 > /proc/irq_latency/enable
    # ... reproduce the latency issue ...
    cat /proc/irq_latency/irq
    echo 0 > /proc/irq_latency/enable

Interpreting Output
===================

Each record shows:

- Latency in milliseconds
- PID and command name of the task running in the affected context
- Kernel stack trace at the time of detection

Records are kept per-CPU and are cleared when detection is restarted.
A maximum of ``MAX_STACK_ENTRIES_INDEX`` (256) records are kept per
CPU per category (IRQ / softirq).
