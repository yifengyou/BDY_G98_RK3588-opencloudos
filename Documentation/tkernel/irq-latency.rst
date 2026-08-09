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
``trace_stack``.  The report separates IRQ and softirq records.
