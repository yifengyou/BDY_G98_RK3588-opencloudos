#!/bin/bash
#
# test_vmscan_ring_hist.sh — Ring-mode hist trigger for vmscan events + kprobe latency
#
# For begin/end pairs: measures latency via synthetic events, aggregated
# per-second using overflow=ring.
# For single events: counts hits per-second using overflow=ring.
# For kprobe functions: measures function execution latency via kretprobes,
# aggregated per-second using overflow=ring.
#
# Usage:
#   ./test_vmscan_ring_hist.sh setup     # configure triggers
#   ./test_vmscan_ring_hist.sh show      # display all histograms
#   ./test_vmscan_ring_hist.sh cleanup   # remove all triggers
#   ./test_vmscan_ring_hist.sh pipe      # display + clear (consume mode)
#
# Ring mode key distribution:
#   With ring_key_divisor optimization, .buckets=N values are divided by N
#   before index computation, so all ring slots are fully utilized.
#   e.g., RING_SIZE=8192 gives ~8192-second sliding window.
#
# Kprobe overhead analysis:
#   Per kprobe/kretprobe pair overhead: ~500-1000 ns per hit (INT3 exception +
#   handler dispatch + trampoline + hist trigger hash update).
#   With ring mode O(1) insert, the hist trigger cost is minimal (~50 ns).
#
#   For N functions at F calls/sec average:
#     Total CPU overhead ≈ N × F × 750ns
#
#   Examples:
#     10 functions @ 1K calls/s  → 10 × 1K × 750ns = 7.5 ms/s  (0.75%)
#     50 functions @ 1K calls/s  → 50 × 1K × 750ns = 37.5 ms/s (3.75%)
#    100 functions @ 100 calls/s → 100 × 100 × 750ns = 7.5 ms/s (0.75%)
#    100 functions @ 1K calls/s  → 100 × 1K × 750ns = 75 ms/s   (7.5%)
#
#   Recommendation: for always-on monitoring, choose slow-path functions
#   with < 1000 calls/sec aggregate rate. Hot-path functions like
#   __alloc_pages can exceed 100K calls/sec under load — enable only
#   for targeted debugging.
#

set -e

TRACEFS=${TRACEFS:-/sys/kernel/debug/tracing}
RING_SIZE=8192
BUCKET=1000000

SYNTH="$TRACEFS/synthetic_events"
VMSCAN="$TRACEFS/events/vmscan"
KPROBE_EVENTS="$TRACEFS/kprobe_events"

# name:begin_event:end_event
PAIRS=(
    "direct_reclaim:mm_vmscan_direct_reclaim_begin:mm_vmscan_direct_reclaim_end"
    "memcg_reclaim:mm_vmscan_memcg_reclaim_begin:mm_vmscan_memcg_reclaim_end"
    "memcg_softlimit:mm_vmscan_memcg_softlimit_reclaim_begin:mm_vmscan_memcg_softlimit_reclaim_end"
    "shrink_slab:mm_shrink_slab_start:mm_shrink_slab_end"
    "node_reclaim:mm_vmscan_node_reclaim_begin:mm_vmscan_node_reclaim_end"
    "kswapd_run:mm_vmscan_kswapd_wake:mm_vmscan_kswapd_sleep"
)

SINGLE_EVENTS=(
    mm_vmscan_wakeup_kswapd
    mm_vmscan_lru_isolate
    mm_vmscan_write_folio
    mm_vmscan_lru_shrink_inactive
    mm_vmscan_lru_shrink_active
    mm_vmscan_throttled
)

# Kprobe functions: function_name only (kprobe+kretprobe pair auto-generated)
# These are memory subsystem slow-path functions suitable for always-on monitoring.
KPROBE_FUNCTIONS=(
    __alloc_pages_slowpath
    try_to_free_pages
    compact_zone
    migrate_pages
    balance_pgdat
)

# Extended list of slow-path functions for targeted debugging.
# Uncomment and add to KPROBE_FUNCTIONS as needed.
# Call rates vary; check /proc/vmstat or ftrace function_profile before enabling.
#
# KPROBE_FUNCTIONS_EXTENDED=(
#     # Reclaim internals
#     shrink_node
#     shrink_lruvec
#     shrink_active_list
#     shrink_inactive_list
#     shrink_folio_list
#     do_try_to_free_pages
#     # Slab
#     __kmem_cache_alloc_node
#     do_shrink_slab
#     # Compaction
#     compact_node
#     compaction_alloc
#     isolate_migratepages_range
#     # Page fault slow paths
#     handle_mm_fault
#     do_anonymous_page
#     wp_page_copy
#     do_cow_fault
#     do_shared_fault
#     do_swap_page
#     # Swap
#     get_swap_pages
#     swap_writepage
#     swapin_readahead
#     # Huge pages
#     alloc_hugepage_direct
#     collapse_huge_page
#     # Migration
#     move_to_new_folio
#     migrate_folio_move
#     unmap_and_move
#     # OOM
#     out_of_memory
#     select_bad_process
#     oom_kill_process
#     # Writeback
#     balance_dirty_pages
#     wb_writeback
#     # Others
#     __alloc_pages          # hot path - use with caution
#     isolate_lru_pages
#     page_vma_mapped_walk
#     folio_referenced
#     folio_check_references
#     mem_cgroup_charge
#     shrink_node_memcgs
#     vmpressure
#     wakeup_kswapd
#     kswapd_shrink_node
#     # File system slow paths
#     generic_file_read_iter
#     filemap_fault
#     filemap_get_pages
#     # Networking memory pressure
#     __sk_mem_reclaim
#     sk_forced_mem_schedule
# )

remove_trigger() {
    local trig="$1/trigger"
    [ -f "$trig" ] || return 0
    grep -o 'hist:[^ ]*' "$trig" 2>/dev/null | while read -r h; do
        echo "!$h" > "$trig" 2>/dev/null || true
    done
}

do_cleanup() {
    echo "=== Cleaning up triggers ==="

    for spec in "${PAIRS[@]}"; do
        IFS=: read -r name begin_evt end_evt <<< "$spec"
        if [ -d "$TRACEFS/events/synthetic/${name}_lat" ]; then
            remove_trigger "$TRACEFS/events/synthetic/${name}_lat"
        fi
        echo "!${name}_lat" >> "$SYNTH" 2>/dev/null || true
        remove_trigger "$VMSCAN/$end_evt"
        remove_trigger "$VMSCAN/$begin_evt"
    done

    for evt in "${SINGLE_EVENTS[@]}"; do
        if [ -d "$VMSCAN/$evt" ]; then
            remove_trigger "$VMSCAN/$evt"
        fi
    done

    # Clean up kprobe triggers and events
    for func in "${KPROBE_FUNCTIONS[@]}"; do
        local name="${func}"
        if [ -d "$TRACEFS/events/synthetic/${name}_lat" ]; then
            remove_trigger "$TRACEFS/events/synthetic/${name}_lat"
        fi
        echo "!${name}_lat" >> "$SYNTH" 2>/dev/null || true
        if [ -d "$TRACEFS/events/kprobes/${name}_exit" ]; then
            remove_trigger "$TRACEFS/events/kprobes/${name}_exit"
        fi
        if [ -d "$TRACEFS/events/kprobes/${name}_entry" ]; then
            remove_trigger "$TRACEFS/events/kprobes/${name}_entry"
        fi
    done

    # Remove all our kprobe events
    for func in "${KPROBE_FUNCTIONS[@]}"; do
        echo "-:kprobes/${func}_entry" >> "$KPROBE_EVENTS" 2>/dev/null || true
        echo "-:kprobes/${func}_exit" >> "$KPROBE_EVENTS" 2>/dev/null || true
    done

    echo "Cleanup done."
}

do_setup() {
    echo "=== Setting up vmscan ring-mode histograms ==="
    echo "    Ring size: $RING_SIZE (~${RING_SIZE}s window with ring_key_divisor)"
    echo ""

    # ── Latency pairs: begin/end → synthetic event → ring histogram ──
    for spec in "${PAIRS[@]}"; do
        IFS=: read -r name begin_evt end_evt <<< "$spec"

        if [ ! -d "$VMSCAN/$begin_evt" ] || [ ! -d "$VMSCAN/$end_evt" ]; then
            echo "[SKIP] $name: event not available"
            continue
        fi

        echo "${name}_lat u64 lat u64 end_ts" >> "$SYNTH"

        echo "hist:keys=common_pid:ts0=common_timestamp.usecs" \
            > "$VMSCAN/$begin_evt/trigger"

        echo "hist:keys=end_ts.buckets=$BUCKET:vals=lat:overflow=ring:size=$RING_SIZE:key_unit=s:val_unit=us:wallclock" \
            > "$TRACEFS/events/synthetic/${name}_lat/trigger"

        echo "hist:keys=common_pid:lat=common_timestamp.usecs-\$ts0:end_ts=common_timestamp.usecs:onmatch(vmscan.$begin_evt).trace(${name}_lat,\$lat,\$end_ts)" \
            > "$VMSCAN/$end_evt/trigger"

        echo "[OK] $name: $begin_evt → $end_evt → ${name}_lat"
    done

    echo ""

    # ── Single events: simple per-second hit count ──
    #
    # With chained modifier support, we can now use:
    #   common_timestamp.usecs.buckets=1000000
    # This gives microsecond timestamps bucketed to 1-second intervals.
    #
    for evt in "${SINGLE_EVENTS[@]}"; do
        if [ ! -d "$VMSCAN/$evt" ]; then
            echo "[SKIP] $evt: not available"
            continue
        fi

        echo "hist:keys=common_timestamp.usecs.buckets=$BUCKET:vals=hitcount:overflow=ring:size=$RING_SIZE:key_unit=s:wallclock" \
            > "$VMSCAN/$evt/trigger"

        echo "[OK] $evt: per-second hitcount (${RING_SIZE}s window)"
    done

    echo ""

    # ── Kprobe functions: function entry/exit → synthetic event → ring histogram ──
    #
    # Uses kprobe + kretprobe to measure function execution latency.
    # Same 3-stage pipeline as tracepoint pairs.
    #
    for func in "${KPROBE_FUNCTIONS[@]}"; do
        local name="${func}"

        # Resolve actual symbol name from kallsyms.
        # Compiler optimizations like constprop rename symbols, e.g.
        #   __alloc_pages_slowpath → __alloc_pages_slowpath.constprop.98
        # Kprobes require the exact symbol name.
        local real_sym
        real_sym=$(awk -v f="$func" '$3 == f || $3 ~ "^"f"\\." {print $3; exit}' /proc/kallsyms)
        if [ -z "$real_sym" ]; then
            echo "[SKIP] $name: function not in kallsyms"
            continue
        fi
        if [ "$real_sym" != "$func" ]; then
            echo "[INFO] $name: resolved to $real_sym"
        fi

        # Create kprobe and kretprobe events
        echo "p:kprobes/${name}_entry ${real_sym}" >> "$KPROBE_EVENTS" || {
            echo "[SKIP] $name: failed to create kprobe"
            continue
        }
        echo "r:kprobes/${name}_exit ${real_sym}" >> "$KPROBE_EVENTS" || {
            echo "[SKIP] $name: failed to create kretprobe"
            echo "-:kprobes/${name}_entry" >> "$KPROBE_EVENTS" 2>/dev/null || true
            continue
        }

        echo "${name}_lat u64 lat u64 end_ts" >> "$SYNTH"

        echo "hist:keys=common_pid:ts0=common_timestamp.usecs" \
            > "$TRACEFS/events/kprobes/${name}_entry/trigger"

        echo "hist:keys=end_ts.buckets=$BUCKET:vals=lat:overflow=ring:size=$RING_SIZE:key_unit=s:val_unit=us:wallclock" \
            > "$TRACEFS/events/synthetic/${name}_lat/trigger"

        echo "hist:keys=common_pid:lat=common_timestamp.usecs-\$ts0:end_ts=common_timestamp.usecs:onmatch(kprobes.${name}_entry).trace(${name}_lat,\$lat,\$end_ts)" \
            > "$TRACEFS/events/kprobes/${name}_exit/trigger"

        echo "[OK] $name: kprobe latency → ${name}_lat"
    done

    echo ""
    echo "Setup complete. Use '$0 show' to view results, '$0 pipe' to view & clear."
}

do_show() {
    echo "================================================================"
    echo "  vmscan Ring-Mode Histogram Results"
    echo "================================================================"

    # ── Latency pair histograms ──
    for spec in "${PAIRS[@]}"; do
        IFS=: read -r name begin_evt end_evt <<< "$spec"
        hist_file="$TRACEFS/events/synthetic/${name}_lat/hist"
        if [ ! -f "$hist_file" ]; then
            continue
        fi

        echo ""
        echo "──── ${name} latency (usecs per second) ────"
        echo ""
        cat "$hist_file"
    done

    # ── Single event histograms ──
    for evt in "${SINGLE_EVENTS[@]}"; do
        hist_file="$VMSCAN/$evt/hist"
        if [ ! -f "$hist_file" ]; then
            continue
        fi

        echo ""
        echo "──── ${evt} (hits per second) ────"
        echo ""
        cat "$hist_file"
    done

    # ── Kprobe function histograms ──
    for func in "${KPROBE_FUNCTIONS[@]}"; do
        local name="${func}"
        hist_file="$TRACEFS/events/synthetic/${name}_lat/hist"
        if [ ! -f "$hist_file" ]; then
            continue
        fi

        echo ""
        echo "──── ${name} kprobe latency (usecs per second) ────"
        echo ""
        cat "$hist_file"
    done
}

do_pipe() {
    echo "================================================================"
    echo "  vmscan Ring-Mode Histogram Results (pipe: clear after read)"
    echo "================================================================"

    for spec in "${PAIRS[@]}"; do
        IFS=: read -r name begin_evt end_evt <<< "$spec"
        pipe_file="$TRACEFS/events/synthetic/${name}_lat/hist_pipe"
        if [ ! -f "$pipe_file" ]; then
            continue
        fi

        echo ""
        echo "──── ${name} latency (usecs per second) ────"
        echo ""
        cat "$pipe_file"
    done

    for evt in "${SINGLE_EVENTS[@]}"; do
        pipe_file="$VMSCAN/$evt/hist_pipe"
        if [ ! -f "$pipe_file" ]; then
            continue
        fi

        echo ""
        echo "──── ${evt} (hits per second) ────"
        echo ""
        cat "$pipe_file"
    done

    for func in "${KPROBE_FUNCTIONS[@]}"; do
        local name="${func}"
        pipe_file="$TRACEFS/events/synthetic/${name}_lat/hist_pipe"
        if [ ! -f "$pipe_file" ]; then
            continue
        fi

        echo ""
        echo "──── ${name} kprobe latency (usecs per second) ────"
        echo ""
        cat "$pipe_file"
    done
}

case "${1:-}" in
    setup)
        do_cleanup
        do_setup
        ;;
    show)
        do_show
        ;;
    pipe)
        do_pipe
        ;;
    cleanup)
        do_cleanup
        ;;
    *)
        echo "Usage: $0 {setup|show|pipe|cleanup}"
        echo ""
        echo "  setup   - Configure ring-mode hist triggers on all vmscan events + kprobes"
        echo "  show    - Display all histogram results"
        echo "  pipe    - Display all histograms and clear ring buffers (consume mode)"
        echo "  cleanup - Remove all triggers, synthetic events, and kprobes"
        exit 1
        ;;
esac
