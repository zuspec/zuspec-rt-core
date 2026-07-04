"""
zuspec.rt.core -- shared runtime-core substrate for the ZBC execution stack.

For now this package's payload is the generated ABI header(s) under
``share/include`` (see :mod:`zuspec.be.bc.format.emit_c`). The runtime
substrate migrates here in a later phase.
"""

import os

__version__ = "0.0.1"

#: Absolute path to the package's ``share`` tree (headers, runtime sources).
SHARE_DIR = os.path.join(os.path.dirname(__file__), "share")


def include_dir() -> str:
    """Return the absolute path to the generated C include directory."""
    return os.path.join(SHARE_DIR, "include")
