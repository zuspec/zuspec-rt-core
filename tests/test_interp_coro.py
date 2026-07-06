"""Feasibility spike: an *interpreted* coroutine on the migrated scheduler.

Demonstrates the P3 core mechanic (see docs/interp-as-coroutine.md):

* the coroutine entrypoint is an interp-defined task func (``zbc_interp_task``);
* its bytecode is conveyed through the thread-create args;
* its operand/variable stack lives in the coroutine frame (the ``zsp_timebase``
  stack-block arena) and survives suspends;
* interpreted and compiled coroutines interleave under one scheduler (mixed
  execution -- the seam that lets AOT/JIT tiers coexist).

The toy ISA: PUSH imm, ADD, WAIT ps (suspend), LOGACC (log top-of-stack + now),
HALT. ``LOGACC`` records ``(value, time)`` into the shared log.
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


@pytest.fixture()
def prog(lib):
    made = []

    def _new():
        p = lib.zspt_prog_new()
        made.append(p)
        return p

    yield _new
    for p in made:
        lib.zspt_prog_free(p)


def _entries(lib, log):
    return [(lib.zspt_log_tag(log, i), lib.zspt_log_time(log, i))
            for i in range(lib.zspt_log_count(log))]


def test_interpreter_runs_as_a_coroutine(lib, tb, log, prog):
    # PUSH 10; WAIT 3; PUSH 5; ADD; WAIT 4; LOGACC; HALT
    # The operand stack (10, then 15) must survive BOTH suspends; LOGACC at t=7
    # sees 15. This is the whole thesis: interp state = frame-resident pc + stack.
    p = prog()
    lib.zspt_prog_push(p, 10)
    lib.zspt_prog_wait(p, 3)
    lib.zspt_prog_push(p, 5)
    lib.zspt_prog_add(p)
    lib.zspt_prog_wait(p, 4)
    lib.zspt_prog_logacc(p)
    lib.zspt_prog_halt(p)

    lib.zspt_spawn_interp(tb, p, log, 1)
    lib.zspt_run_all(tb)

    assert _entries(lib, log) == [(15, 7)]     # value 15 logged at t=7
    assert lib.zspt_tb_active(tb) == 0


def test_interp_operand_stack_is_frame_resident_across_many_suspends(lib, tb, log, prog):
    # Accumulate 1+2+3+4 = 10 with a WAIT between each add, logging after each.
    # If the stack were not frame-resident it could not survive the suspends.
    p = prog()
    lib.zspt_prog_push(p, 1)
    for add in (2, 3, 4):
        lib.zspt_prog_wait(p, 1)
        lib.zspt_prog_push(p, add)
        lib.zspt_prog_add(p)
        lib.zspt_prog_logacc(p)
    lib.zspt_prog_halt(p)

    lib.zspt_spawn_interp(tb, p, log, 7)
    lib.zspt_run_all(tb)

    # running sums 3,6,10 logged at t=1,2,3
    assert _entries(lib, log) == [(3, 1), (6, 2), (10, 3)]


def test_interpreted_and_compiled_coroutines_mix(lib, tb, log, prog):
    # An interpreted coro and a *compiled* waiter share one scheduler and
    # interleave purely by time -- proving the task-func/frame ABI is the seam
    # for mixing interpreted / AOT (and later JIT) tiers.
    #   interp:   PUSH 99; WAIT 3; LOGACC; WAIT 4; PUSH 1; ADD; LOGACC; HALT
    #             -> logs (99, 3) then (100, 7)
    #   compiled: waiter tag 5, delay 5 -> logs (5, 5)
    p = prog()
    lib.zspt_prog_push(p, 99)
    lib.zspt_prog_wait(p, 3)
    lib.zspt_prog_logacc(p)
    lib.zspt_prog_wait(p, 4)
    lib.zspt_prog_push(p, 1)
    lib.zspt_prog_add(p)
    lib.zspt_prog_logacc(p)
    lib.zspt_prog_halt(p)

    lib.zspt_spawn_interp(tb, p, log, 0)      # interpreted
    lib.zspt_spawn_waiter(tb, log, 5, 5)      # compiled (from the shim)
    lib.zspt_run_all(tb)

    assert _entries(lib, log) == [(99, 3), (5, 5), (100, 7)]
    assert lib.zspt_tb_current(tb) == 7
