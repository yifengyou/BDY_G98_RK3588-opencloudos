.. SPDX-License-Identifier: GPL-2.0-only

=======
Netlat
=======

:Authors: - mengensun <mengensun@tencent.com>
          - yuehongwu <yuehongwu@tencent.com>

Overview
========

Netlat is a TCP packet latency monitor integrated into the Linux
networking stack.  It measures three per-connection latency components
that are invisible to standard tools such as ``ss`` or ``netstat``:

- **ack latency** – time between receiving a data segment and the ACK
  being processed by the TCP stack.
- **queue latency** – time a segment spends in the receive queue before
  the application reads it.
- **pick latency** – time between the application calling recvmsg()
  and the first byte being copied.

Netlat is designed for long-running production servers where adding
per-packet tracing (e.g. BPF or ftrace) is too expensive.  It uses a
static-key (``static_branch``) so that the measurement path compiles
to a single NOP when disabled.

Enabling
========

Netlat is controlled through per-netns sysctl knobs under
``/proc/sys/net/ipv4/netlat/``:

.. list-table::
   :header-rows: 1

   * - Knob
     - Type
     - Description
   * - ``enable``
     - 0/1
     - Master switch.  When 0 the static key is off and all
       measurement code is skipped.
   * - ``ack``
     - int (ms)
     - Ack-latency threshold.  Connections whose ack latency exceeds
       this value are recorded.  0 disables ack measurement.
   * - ``queue``
     - int (ms)
     - Queue-latency threshold.
   * - ``pick``
     - int (ms)
     - Pick-latency threshold.
   * - ``lports``
     - bitmap
     - Local-port filter.  Only connections whose local port is set
       in this bitmap are monitored.  Use the standard bitmap sysctl
       syntax (e.g. ``echo 0-1023 > lports``).

Example
=======

Monitor all connections on ports 80 and 443 with a 10 ms ack threshold::

    echo 1 > /proc/sys/net/ipv4/netlat/enable
    echo 10 > /proc/sys/net/ipv4/netlat/ack
    echo 80,443 > /proc/sys/net/ipv4/netlat/lports

Architecture
============

Netlat hooks into three points in the TCP receive path:

1. ``netlat_tcp_enrtxqueue()`` – stamps the first-xmit time on
   retransmit-queue skbs (``TCP_SKB_CB->first_xmit_time``).
2. ``netlat_queue_check()`` – called when a segment enters the
   receive queue; computes queue latency.
3. ``netlat_ack_check()`` – called on ACK processing; computes ack
   latency.

All three are guarded by ``static_branch_unlikely(&enable_netlat)``
so the overhead is a single branch-predicted NOP when the feature is
disabled.

Configuration
=============

Enable at build time with ``CONFIG_NETLAT=y``.

.. note::

   Netlat currently supports IPv4 (``AF_INET``) sockets only.
   IPv6 support is planned for a future release.
