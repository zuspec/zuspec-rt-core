"""
Substrate test harness: compile the rt-core C substrate (+ the test-support
shim) into a shared library once per session and hand tests a configured
``ctypes.CDLL``.

Per project convention, native code is tested via pytest + ctypes rather than a
C++ test framework -- see :mod:`zuspec.rt.core.build`.
"""

import ctypes
import os

import pytest

from zuspec.rt.core import build as rtbuild

_HERE = os.path.dirname(__file__)
_CSUPPORT = os.path.join(_HERE, "csupport", "zsp_testsupport.c")

# zsp_time_unit_e values (exponents; S is the special 10^0 case).
ZSP_TIME_S, ZSP_TIME_MS, ZSP_TIME_US = 1, -3, -6
ZSP_TIME_NS, ZSP_TIME_PS, ZSP_TIME_FS = -9, -12, -15


@pytest.fixture(scope="session")
def libpath(tmp_path_factory):
    out = tmp_path_factory.mktemp("rtcore") / "libzsp_rtcore_test.so"
    return rtbuild.build_shared_lib(str(out), extra_sources=[_CSUPPORT])


@pytest.fixture(scope="session")
def _rawlib(libpath):
    return ctypes.CDLL(libpath)


def _declare(lib):
    """Attach argtypes/restypes for the ABI + test-support surface."""
    c = ctypes
    P = c.c_void_p

    sigs = {
        # allocator
        "zsp_alloc_malloc_create": ([], P),
        "zspt_malloc_alloc": ([], P),
        "zspt_alloc_call": ([P, c.c_size_t], P),
        "zspt_alloc_free": ([P, P], None),
        # timebase lifecycle
        "zsp_timebase_create": ([P, c.c_int], P),
        "zsp_timebase_destroy": ([P], None),
        "zsp_timebase_current_ticks": ([P], c.c_uint64),
        # zsp_time_t-by-value helpers (scalar-arg wrappers, see the C shim)
        "zspt_to_ticks": ([P, c.c_uint64, c.c_int32], c.c_uint64),
        "zspt_schedule_at": ([P, P, c.c_uint64, c.c_int32], None),
        # scheduling
        "zsp_timebase_schedule": ([P, P], None),
        "zsp_timebase_run": ([P], c.c_int),
        "zsp_timebase_advance": ([P], c.c_int),
        "zsp_timebase_has_pending": ([P], c.c_int),
        "zsp_timebase_cancel": ([P, P], None),
        # test support
        "zspt_thread_new": ([P, c.c_void_p], P),
        "zspt_thread_free": ([P], None),
        "zspt_thread_tag": ([P], c.c_void_p),
        "zspt_thread_set_exit": ([P, P], None),
        "zspt_tb_current": ([P], c.c_uint64),
        "zspt_tb_event_count": ([P], c.c_uint32),
        "zspt_tb_event_capacity": ([P], c.c_uint32),
        "zspt_tb_active": ([P], c.c_int32),
        "zspt_tb_ready_empty": ([P], c.c_int),
        "zspt_tb_resolution": ([P], c.c_int32),
        # coroutine-driven test support
        "zspt_log_new": ([], P),
        "zspt_log_free": ([P], None),
        "zspt_log_count": ([P], c.c_uint32),
        "zspt_log_tag": ([P, c.c_uint32], c.c_uint64),
        "zspt_log_time": ([P, c.c_uint32], c.c_uint64),
        "zspt_spawn_waiter": ([P, P, c.c_uint64, c.c_uint64], P),
        "zspt_spawn_double": ([P, P, c.c_uint64, c.c_uint64, c.c_uint64], P),
        "zspt_run_all": ([P], None),
        # indexed pool
        "zsp_indexed_pool_acquire": ([P, c.c_int], c.c_int),
        "zsp_indexed_pool_acquire_random": ([P], c.c_int),
        "zsp_indexed_pool_release": ([P, c.c_int], None),
        "zspt_pool_new": ([c.c_uint32], P),
        "zspt_pool_free": ([P], None),
        "zspt_pool_is_used": ([P, c.c_int], c.c_int),
        "zspt_pool_seed": ([P], c.c_uint32),
        "zspt_pool_set_seed": ([P, c.c_uint32], None),
        # interpreter-as-coroutine spike
        "zspt_prog_new": ([], P),
        "zspt_prog_free": ([P], None),
        "zspt_prog_push": ([P, c.c_uint64], None),
        "zspt_prog_add": ([P], None),
        "zspt_prog_wait": ([P, c.c_uint64], None),
        "zspt_prog_logacc": ([P], None),
        "zspt_prog_halt": ([P], None),
        "zspt_spawn_interp": ([P, P, P, c.c_uint64], P),
    }
    for name, (args, res) in sigs.items():
        fn = getattr(lib, name)
        fn.argtypes = args
        fn.restype = res
    return lib


@pytest.fixture()
def lib(_rawlib):
    """A ctypes.CDLL of the substrate with ABI signatures declared."""
    return _declare(_rawlib)


# zsp_time_t is passed *by value* as {uint64 amt; int32 unit;}. The System V
# AMD64 ABI packs that 12-byte aggregate into two integer registers, so passing
# (amt, unit) as two scalar args matches the calling convention on the platforms
# we test. The signatures above declare to_ticks/schedule_at that way.
EXIT_FUNC = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
