"""
zuspec.rt.core.build -- compile the substrate into a shared library.

The runtime substrate is plain C with a stable ``zsp_`` ABI, so it is tested by
compiling it to a ``.so`` and driving it from Python through ``ctypes`` (the
project convention for native code -- no gtest). This module is that compile
step, factored out so both the substrate tests and any downstream consumer can
reuse it.

``build_shared_lib`` compiles :func:`zuspec.rt.core.source_files` (plus any
``extra_sources`` -- e.g. a test-support shim exposing struct accessors) against
:func:`zuspec.rt.core.include_dir` and returns the path to the produced library.
"""

import os
import subprocess
import sysconfig
from typing import Iterable, List, Optional

from . import include_dir, source_files


def default_cc() -> str:
    """The C compiler to use (``$CC`` if set, else the platform default, else cc)."""
    return os.environ.get("CC") or sysconfig.get_config_var("CC") or "cc"


def _shared_flag() -> List[str]:
    # macOS wants -dynamiclib; ELF platforms use -shared. Default to -shared.
    if os.uname().sysname == "Darwin":
        return ["-dynamiclib"]
    return ["-shared"]


def build_shared_lib(out_path: str,
                     extra_sources: Optional[Iterable[str]] = None,
                     extra_include_dirs: Optional[Iterable[str]] = None,
                     extra_link_args: Optional[Iterable[str]] = None,
                     cc: Optional[str] = None) -> str:
    """Compile the substrate (+ ``extra_sources``) into ``out_path``.

    ``extra_link_args`` are appended after the sources (e.g. ``-L``/``-l``/
    ``-Wl,-rpath`` to link a downstream shared library such as libdv_solve).

    Returns ``out_path`` on success; raises :class:`RuntimeError` with the
    compiler output on failure.
    """
    cc = cc or default_cc()
    srcs = list(source_files()) + list(extra_sources or [])
    incs = [include_dir()] + list(extra_include_dirs or [])

    # $CC may carry flags (e.g. "gcc -pthread"); split so they are honored.
    cmd = cc.split() + _shared_flag() + ["-fPIC", "-O2", "-g"]
    for inc in incs:
        cmd += ["-I", inc]
    cmd += srcs + ["-o", out_path]
    cmd += list(extra_link_args or [])

    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            "failed to build rt-core shared lib:\n"
            "  cmd: %s\n%s%s" % (" ".join(cmd), proc.stdout, proc.stderr))
    return out_path


def compile_check(source: str, lang: str = "c",
                  extra_flags: Optional[Iterable[str]] = None,
                  compiler: Optional[str] = None) -> "tuple[bool, str]":
    """Compile ``source`` (syntax-only) against the substrate include dir.

    ``lang`` is ``"c"`` or ``"c++"``. Returns ``(ok, output)`` -- used by the
    substrate hygiene tests to assert headers are self-contained and compile as
    both C and C++.
    """
    if compiler is None:
        compiler = default_cc() if lang == "c" else (
            os.environ.get("CXX") or "c++")
    cmd = compiler.split() + ["-fsyntax-only", "-x", lang,
                              "-I", include_dir()]
    cmd += list(extra_flags or [])
    cmd += ["-"]
    proc = subprocess.run(cmd, input=source, capture_output=True, text=True)
    return proc.returncode == 0, (proc.stdout + proc.stderr)
