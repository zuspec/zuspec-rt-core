"""Substrate hygiene: every public header must be self-contained and compile as
both C and C++ (the ABI header is included from the engine / C++ consumers).

This enforces the P2 "substrate compiles standalone" gate criterion as a test
rather than an assumption, and guards the generated ``zbc_format.h`` against a
regression to C-only ``_Static_assert`` (it now uses a portable macro).
"""

import glob
import os

import pytest

from zuspec.rt.core import build, include_dir

HEADERS = sorted(os.path.basename(h)
                 for h in glob.glob(os.path.join(include_dir(), "*.h")))


def test_there_are_headers():
    # Sanity: the migrated substrate + generated ABI header are present.
    assert {"zsp_alloc.h", "zsp_timebase.h",
            "zsp_indexed_pool.h", "zbc_format.h"} <= set(HEADERS)


@pytest.mark.parametrize("header", HEADERS)
def test_header_self_contained_as_c(header):
    src = '#include "%s"\n' % header
    ok, out = build.compile_check(src, lang="c", extra_flags=["-std=c11"])
    assert ok, "header %s is not self-contained as C11:\n%s" % (header, out)


@pytest.mark.parametrize("header", HEADERS)
def test_header_self_contained_as_cxx(header):
    src = '#include "%s"\n' % header
    ok, out = build.compile_check(src, lang="c++", extra_flags=["-std=c++11"])
    assert ok, "header %s is not self-contained as C++11:\n%s" % (header, out)


def test_headers_are_idempotent_under_double_include():
    # Include guards: including a header twice must be a no-op.
    for header in HEADERS:
        src = '#include "%s"\n#include "%s"\nint main(void){return 0;}\n' % (
            header, header)
        ok, out = build.compile_check(src, lang="c", extra_flags=["-std=c11"])
        assert ok, "header %s lacks a working include guard:\n%s" % (header, out)
