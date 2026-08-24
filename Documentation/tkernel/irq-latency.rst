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
    Write 1 to start detection and 0 to stop.  Mode 2 additionally marks
    softirq delays observed from the IRQ timer.  Values above 2 are
    rejected.

``freq_ms``
    Probe frequency in milliseconds (5 -- 5000, default 10).
    Lower values increase detection resolution but add overhead.  The
    value cannot exceed half of ``latency_thresh_ms`` and can only be
    changed while detection is disabled.

``latency_thresh_ms``
    Stack-recording threshold in milliseconds (default 30).  The value
    is kept at least twice ``freq_ms`` and can only be changed while
    detection is disabled.

``trace_stack``
    Reports the recorded IRQ and softirq stack traces with latency,
    PID, and command name information.  Write 0 to clear the stack
    records and latency histograms; other values are rejected.

``trace_dist``
    Reports the accumulated IRQ-disable and softirq-disable latency
    distributions.

Runtime and CPU Hotplug
=======================

Enable, disable, CPU hotplug, and module exit are serialized through a
single runtime state machine.  A CPU that comes online starts its timers
only when detection is enabled.  Taking a CPU offline synchronously stops
both of its timers before the hotplug operation completes.

Stack records and histogram counters are retained when a CPU goes offline.
The stack report marks such CPUs with ``(offline)``, and the distribution
continues to include their counters.  Write 0 to ``trace_stack`` to clear
records and counters for both online and offline CPUs.

Readers copy the possible and online CPU masks while holding the CPU read
lock, then release that lock before copying records and formatting output.
Consequently, a long stack report does not hold up a CPU hotplug operation.

Module exit changes the detector to an exiting state before removing its
proc controls.  The registered CPU hotplug teardown then cancels every
per-CPU timer before the per-CPU storage is released.

Runtime Impact
==============

Loading the module allocates fixed-size storage for every possible CPU.
Periodic timers do not run until detection is enabled.  While enabled, the
existing sampling cost is controlled by ``freq_ms``; using a lower value
increases timer and interrupt activity on every online CPU.  State changes,
CPU hotplug, and trace reads add only bounded synchronization outside the
normal sampling path.

Example
=======

::

    # modprobe irqlatency
    echo 10 > /proc/irq_latency/freq_ms
    echo 50 > /proc/irq_latency/latency_thresh_ms
    echo 1 > /proc/irq_latency/enable
    # ... reproduce the latency issue ...
    cat /proc/irq_latency/trace_stack
    cat /proc/irq_latency/trace_dist
    echo 0 > /proc/irq_latency/enable
    echo 0 > /proc/irq_latency/trace_stack

Interpreting Output
===================

Each record shows:

- Latency in milliseconds
- PID and command name of the task running in the affected context
- Kernel stack trace at the time of detection

Records are kept per-CPU in fixed-size storage.  Once that storage is
full, new stack records are dropped until 0 is written to
``trace_stack``.  Records survive CPU offline and are marked accordingly.
The report separates IRQ and softirq records.

Selftests
=========

``tools/testing/selftests/tkernel/irqlatency.sh`` exercises concurrent
state changes, CPU hotplug, trace reads, and module unload.  The hotplug and
unload cases are intended for a dedicated test system rather than a
production host.  The test restores a CPU that it takes offline and unloads
the module only when the test loaded that module itself.
