"""
zuspec.rt.core -- shared runtime-core substrate for the ZBC execution stack.

The package payload is the C substrate the native engine (``zuspec-rt-eng``) and
the AOT output (``zuspec-be-sw``) both link against, plus the generated ABI
header(s):

* ``share/include`` -- the generated ABI header (``zbc_format.h``, see
  :mod:`zuspec.be.bc.format.emit_c`) and the migrated substrate headers.
* ``share/rt`` -- the migrated substrate C sources (scheduler, allocator, ...).

Downstream builds discover the headers + sources through :func:`include_dir` and
:func:`source_files`; :mod:`zuspec.rt.core.build` compiles them into a shared
library (used by the pytest+ctypes substrate tests, per project convention).

Migration is incremental (roadmap P2); ``zuspec-be-sw`` keeps its own copies as
shims until the cutover, so it is unaffected while the substrate lands here.
"""

import os
import glob

__version__ = "0.0.1"

#: Absolute path to the package's ``share`` tree (headers, runtime sources).
SHARE_DIR = os.path.join(os.path.dirname(__file__), "share")


def include_dir() -> str:
    """Return the absolute path to the C include directory (headers)."""
    return os.path.join(SHARE_DIR, "include")


def rt_dir() -> str:
    """Return the absolute path to the runtime C source directory."""
    return os.path.join(SHARE_DIR, "rt")


def source_files() -> list:
    """Return the absolute paths of the substrate C sources (sorted, stable)."""
    return sorted(glob.glob(os.path.join(rt_dir(), "*.c")))
