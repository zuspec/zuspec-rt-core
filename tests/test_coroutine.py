"""ctypes tests driving *real* zsp_task_func coroutines on the migrated timebase.

Where ``test_timebase.py`` drives the queues with bare threads, these tests run
actual coroutines (C task funcs in the test shim) to exercise the frame/stack
machinery the substrate exists for: ``alloc_frame`` / ``wait`` / ``return``,
idx-based resume, and locals surviving across suspends. Each coroutine logs
``(tag, completion_time)`` so ordering and timing are asserted from Python.
"""

import pytest

from conftest import ZSP_TIME_PS


@pytest.fixture()
def tb(lib):
    alloc = lib.zsp_alloc_malloc_create()
    tb = lib.zsp_timebase_create(alloc, ZSP_TIME_PS)
    yield tb
    lib.zsp_timebase_destroy(tb)


@pytest.fixture()
def log(lib):
    l = lib.zspt_log_new()
    yield l
    lib.zspt_log_free(l)


def _entries(lib, log):
    """Return the log as a list of (tag, time) in completion order."""
    return [(lib.zspt_log_tag(log, i), lib.zspt_log_time(log, i))
            for i in range(lib.zspt_log_count(log))]


def test_single_waiter_completes_at_delay(lib, tb, log):
    # One coroutine, sole runnable -> zsp_timebase_wait takes the inline
    # fast-path (advances time without suspending); it still finishes at t=7.
    lib.zspt_spawn_waiter(tb, log, 1, 7)
    assert lib.zspt_tb_active(tb) == 1
    lib.zspt_run_all(tb)
    assert _entries(lib, log) == [(1, 7)]
    assert lib.zspt_tb_active(tb) == 0
    assert lib.zsp_timebase_has_pending(tb) == 0


def test_two_waiters_resume_in_time_order(lib, tb, log):
    # Two runnable coroutines force the real suspend path: each sees the other
    # pending, so wait() blocks + schedules on the heap. They resume in time
    # order (delay 3 before delay 5) regardless of spawn order.
    lib.zspt_spawn_waiter(tb, log, 5, 5)
    lib.zspt_spawn_waiter(tb, log, 3, 3)
    lib.zspt_run_all(tb)
    assert _entries(lib, log) == [(3, 3), (5, 5)]
    assert lib.zspt_tb_current(tb) == 5


def test_double_wait_accumulates_time_and_persists_locals(lib, tb, log):
    # wait 3 then wait 4 in one frame: logs at t=3 (phase 1) and t=7 (phase 2),
    # proving locals (tag, d2) survive the first suspend and idx advances 1->2->3.
    lib.zspt_spawn_double(tb, log, 1, 3, 4)
    lib.zspt_run_all(tb)
    assert _entries(lib, log) == [(11, 3), (12, 7)]


def test_many_coroutines_interleave_by_time(lib, tb, log):
    # A double (waits 2 then 6 -> logs at 2 and 8) interleaves with singles at
    # 4 and 8. Completion order is purely by time across independent frames.
    lib.zspt_spawn_double(tb, log, 9, 2, 6)   # -> (91, 2), (92, 8)
    lib.zspt_spawn_waiter(tb, log, 4, 4)      # -> (4, 4)
    lib.zspt_spawn_waiter(tb, log, 8, 8)      # -> (8, 8)
    lib.zspt_run_all(tb)
    entries = _entries(lib, log)
    # Sorted by completion time; the two t=8 events keep spawn/heap order.
    assert [t for (_, t) in entries] == [2, 4, 6 + 2, 8]
    assert dict(entries)[91] == 2 and dict(entries)[92] == 8
    assert lib.zspt_tb_current(tb) == 8
