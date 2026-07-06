"""ctypes tests for the migrated ``zsp_timebase`` scheduler.

Exercises the scheduler substrate through the ABI without real setjmp
coroutines: a bare thread (leaf == NULL) is popped by ``zsp_timebase_run`` and
"completes" immediately, firing its ``exit_f`` -- which we use to record run
order. Covers ready-queue FIFO, the timed-event min-heap (time order + same-time
stability), time advance, and tick conversion.
"""

import ctypes

import pytest

from conftest import (
    EXIT_FUNC, ZSP_TIME_S, ZSP_TIME_MS, ZSP_TIME_US,
    ZSP_TIME_NS, ZSP_TIME_PS,
)


@pytest.fixture()
def tb(lib):
    """A fresh picosecond-resolution timebase over a malloc allocator."""
    alloc = lib.zsp_alloc_malloc_create()
    tb = lib.zsp_timebase_create(alloc, ZSP_TIME_PS)
    yield tb
    lib.zsp_timebase_destroy(tb)


@pytest.fixture()
def recorder(lib):
    """Return (exit_func, order): an exit_f that appends each thread's tag."""
    order = []

    def _on_exit(thread):
        order.append(lib.zspt_thread_tag(thread))

    cb = EXIT_FUNC(_on_exit)  # kept alive by the closure over `cb` below
    cb._order = order
    return cb, order


def _new_thread(lib, tb, tag, exit_cb):
    t = lib.zspt_thread_new(tb, tag)
    lib.zspt_thread_set_exit(t, exit_cb)
    return t


def _drain_ready(lib, tb):
    while lib.zspt_tb_ready_empty(tb) == 0:
        lib.zsp_timebase_run(tb)


# --------------------------------------------------------------------------- #
# Ready queue -- FIFO at the current time.
# --------------------------------------------------------------------------- #

def test_ready_queue_is_fifo(lib, tb, recorder):
    cb, order = recorder
    for tag in (1, 2, 3):
        lib.zsp_timebase_schedule(tb, _new_thread(lib, tb, tag, cb))
    assert lib.zspt_tb_active(tb) == 3
    _drain_ready(lib, tb)
    assert order == [1, 2, 3]
    assert lib.zspt_tb_active(tb) == 0


# --------------------------------------------------------------------------- #
# Timed-event heap -- threads wake in time order regardless of insert order.
# --------------------------------------------------------------------------- #

def test_timed_events_wake_in_time_order(lib, tb, recorder):
    cb, order = recorder
    # Insert out of order; tags encode the delay for readability.
    for delay in (30, 10, 20):
        lib.zspt_schedule_at(tb, _new_thread(lib, tb, delay, cb), delay, ZSP_TIME_PS)
    assert lib.zspt_tb_event_count(tb) == 3
    assert lib.zspt_tb_ready_empty(tb) == 1  # nothing ready yet

    for expect_time in (10, 20, 30):
        assert lib.zsp_timebase_advance(tb) == 1
        assert lib.zspt_tb_current(tb) == expect_time
        _drain_ready(lib, tb)
    assert order == [10, 20, 30]
    assert lib.zsp_timebase_advance(tb) == 0        # heap drained
    assert lib.zsp_timebase_has_pending(tb) == 0


def test_same_time_events_are_stable_by_insertion(lib, tb, recorder):
    cb, order = recorder
    # Two events at the same wake time: sequence number breaks the tie in
    # insertion order (a before b), preserved through the ready queue.
    lib.zspt_schedule_at(tb, _new_thread(lib, tb, 11, cb), 5, ZSP_TIME_PS)
    lib.zspt_schedule_at(tb, _new_thread(lib, tb, 22, cb), 5, ZSP_TIME_PS)
    assert lib.zsp_timebase_advance(tb) == 1
    assert lib.zspt_tb_current(tb) == 5
    assert lib.zspt_tb_event_count(tb) == 0         # both moved at once
    _drain_ready(lib, tb)
    assert order == [11, 22]


def test_cancelled_timed_thread_is_dropped_without_advancing_time(lib, tb, recorder):
    # Three timed threads at 3, 10, 5; cancel the one at 10. It must be dropped
    # from the heap without ever waking or influencing the clock.
    cb, order = recorder
    t3 = _new_thread(lib, tb, 3, cb)
    t10 = _new_thread(lib, tb, 10, cb)
    t5 = _new_thread(lib, tb, 5, cb)
    lib.zspt_schedule_at(tb, t3, 3, ZSP_TIME_PS)
    lib.zspt_schedule_at(tb, t10, 10, ZSP_TIME_PS)
    lib.zspt_schedule_at(tb, t5, 5, ZSP_TIME_PS)

    lib.zsp_timebase_cancel(tb, t10)

    while lib.zsp_timebase_advance(tb) == 1:
        _drain_ready(lib, tb)
    assert order == [3, 5]                    # 10 never woke
    assert lib.zspt_tb_current(tb) == 5       # clock stopped at the last live event


def test_cancelled_ready_thread_is_dropped(lib, tb, recorder):
    cb, order = recorder
    a = _new_thread(lib, tb, 1, cb)
    b = _new_thread(lib, tb, 2, cb)
    lib.zsp_timebase_schedule(tb, a)
    lib.zsp_timebase_schedule(tb, b)
    lib.zsp_timebase_cancel(tb, a)            # drop A before it runs
    _drain_ready(lib, tb)
    assert order == [2]


def test_heap_grows_past_initial_capacity(lib, tb, recorder):
    cb, order = recorder
    # INITIAL_EVENT_CAPACITY is 16; push 40 to force at least one grow.
    n = 40
    for delay in range(n, 0, -1):        # insert in descending time
        lib.zspt_schedule_at(tb, _new_thread(lib, tb, delay, cb), delay, ZSP_TIME_PS)
    assert lib.zspt_tb_event_count(tb) == n
    assert lib.zspt_tb_event_capacity(tb) >= n
    while lib.zsp_timebase_advance(tb) == 1:
        _drain_ready(lib, tb)
    assert order == list(range(1, n + 1))   # popped in ascending time order


# --------------------------------------------------------------------------- #
# has_pending across ready / timed / drained.
# --------------------------------------------------------------------------- #

def test_has_pending_tracks_ready_and_timed(lib, tb, recorder):
    cb, order = recorder
    assert lib.zsp_timebase_has_pending(tb) == 0
    lib.zspt_schedule_at(tb, _new_thread(lib, tb, 7, cb), 7, ZSP_TIME_PS)
    assert lib.zsp_timebase_has_pending(tb) == 1     # timed event pending
    lib.zsp_timebase_advance(tb)
    assert lib.zsp_timebase_has_pending(tb) == 1     # now ready
    _drain_ready(lib, tb)
    assert lib.zsp_timebase_has_pending(tb) == 0
    assert order == [7]


# --------------------------------------------------------------------------- #
# Time conversion -- coarser multiplies, finer divides, delta is zero.
# --------------------------------------------------------------------------- #

@pytest.mark.parametrize("amt,unit,expect", [
    (0, ZSP_TIME_NS, 0),          # delta (amt==0) is always 0
    (5, ZSP_TIME_PS, 5),          # same resolution
    (1, ZSP_TIME_NS, 1_000),      # ns -> ps (coarser: x1000)
    (1, ZSP_TIME_US, 1_000_000),  # us -> ps
    (3, ZSP_TIME_MS, 3_000_000_000),
    (2, ZSP_TIME_S, 2_000_000_000_000),
])
def test_to_ticks_ps_resolution(lib, tb, amt, unit, expect):
    assert lib.zspt_to_ticks(tb, amt, unit) == expect


def test_to_ticks_finer_input_divides_with_truncation(lib):
    # Resolution ns; ps input is finer -> divide by 1000 (integer truncation).
    alloc = lib.zsp_alloc_malloc_create()
    tb = lib.zsp_timebase_create(alloc, ZSP_TIME_NS)
    try:
        assert lib.zspt_to_ticks(tb, 1_500, ZSP_TIME_PS) == 1     # 1500ps -> 1ns
        assert lib.zspt_to_ticks(tb, 999, ZSP_TIME_PS) == 0       # precision loss
        assert lib.zspt_to_ticks(tb, 1, ZSP_TIME_US) == 1_000     # us -> ns
    finally:
        lib.zsp_timebase_destroy(tb)
