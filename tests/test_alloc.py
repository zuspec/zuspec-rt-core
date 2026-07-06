"""ctypes tests for the migrated ``zsp_alloc`` malloc allocator."""

import ctypes


def test_malloc_alloc_create_returns_usable_allocator(lib):
    alloc = lib.zsp_alloc_malloc_create()
    assert alloc is not None

    # The vtable is callable: allocate a block, write and read it back.
    p = lib.zspt_alloc_call(alloc, 64)
    assert p is not None
    buf = (ctypes.c_ubyte * 64).from_address(p)
    for i in range(64):
        buf[i] = i & 0xFF
    assert list(buf) == [i & 0xFF for i in range(64)]
    lib.zspt_alloc_free(alloc, p)


def test_shim_and_direct_create_agree(lib):
    # The test-support factory just forwards to the ABI entry point.
    a1 = lib.zsp_alloc_malloc_create()
    a2 = lib.zspt_malloc_alloc()
    assert a1 is not None and a2 is not None and a1 != a2  # distinct allocations
