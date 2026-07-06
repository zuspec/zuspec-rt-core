"""ctypes tests for the migrated ``zsp_indexed_pool`` (bitmask slot pool).

Covers specific acquire/release + used-bit bookkeeping, and the deterministic
xorshift32 ``acquire_random`` -- validated to the exact slot against a Python
model of the same RNG + free-slot walk.
"""

import pytest


MASK32 = 0xFFFFFFFF


def _xorshift32(seed):
    seed ^= (seed << 13) & MASK32
    seed ^= seed >> 17
    seed ^= (seed << 5) & MASK32
    return seed & MASK32


def _model_acquire_random(seed, used, size):
    """Mirror zsp_indexed_pool_acquire_random: count free, advance seed, pick,
    then walk to the pick-th free slot. Returns (chosen_idx, new_seed)."""
    free = [i for i in range(size) if i not in used]
    seed = _xorshift32(seed)
    pick = seed % len(free)
    return free[pick], seed


@pytest.fixture()
def pool(lib):
    def _make(size):
        p = lib.zspt_pool_new(size)
        _made.append(p)
        return p
    _made = []
    yield _make
    for p in _made:
        lib.zspt_pool_free(p)


def test_acquire_specific_marks_used(lib, pool):
    p = pool(8)
    assert lib.zspt_pool_is_used(p, 3) == 0
    assert lib.zsp_indexed_pool_acquire(p, 3) == 3
    assert lib.zspt_pool_is_used(p, 3) == 1
    # other slots untouched
    assert lib.zspt_pool_is_used(p, 4) == 0


def test_release_clears_used_and_allows_reacquire(lib, pool):
    p = pool(8)
    lib.zsp_indexed_pool_acquire(p, 5)
    lib.zsp_indexed_pool_release(p, 5)
    assert lib.zspt_pool_is_used(p, 5) == 0
    assert lib.zsp_indexed_pool_acquire(p, 5) == 5   # re-acquire ok


def test_used_bits_span_word_boundary(lib, pool):
    # size 40 -> 2 mask words; touch slots on both sides of the 32-bit boundary.
    p = pool(40)
    for idx in (0, 31, 32, 39):
        lib.zsp_indexed_pool_acquire(p, idx)
    for idx in (0, 31, 32, 39):
        assert lib.zspt_pool_is_used(p, idx) == 1
    for idx in (1, 30, 33, 38):
        assert lib.zspt_pool_is_used(p, idx) == 0


def test_acquire_random_matches_model_over_full_drain(lib, pool):
    size = 6
    p = pool(size)
    assert lib.zspt_pool_seed(p) == 1     # init seed

    seed = 1
    used = set()
    for _ in range(size):
        expect, seed = _model_acquire_random(seed, used, size)
        got = lib.zsp_indexed_pool_acquire_random(p)
        assert got == expect
        assert lib.zspt_pool_seed(p) == seed   # RNG state tracks the model
        used.add(got)
        assert lib.zspt_pool_is_used(p, got) == 1
    # Every slot got acquired exactly once -> a permutation of 0..size-1.
    assert used == set(range(size))


def test_acquire_random_seed_changes_sequence(lib, pool):
    size = 6
    p1, p2 = pool(size), pool(size)
    lib.zspt_pool_set_seed(p2, 0xDEADBEEF)
    s1 = [lib.zsp_indexed_pool_acquire_random(p1) for _ in range(size)]
    s2 = [lib.zsp_indexed_pool_acquire_random(p2) for _ in range(size)]
    # Both are permutations, but the draw order differs with a different seed.
    assert sorted(s1) == sorted(s2) == list(range(size))
    assert s1 != s2
