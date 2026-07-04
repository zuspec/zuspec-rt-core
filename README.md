# zuspec-rt-core

Runtime-core substrate for the Zuspec ByteCode (ZBC) execution stack.

At this stage the package exists to *receive* generated ABI artifacts — most
importantly `share/include/zbc_format.h`, the C view of the `.zbc` container
format emitted from the single spec in `zuspec-be-bc`
(`zuspec.be.bc.format.spec`). The header is checked in as a generated artifact
and guarded by a regen no-diff test in `zuspec-be-bc`.

The larger runtime substrate (scheduler, memory pools) migrates here in a later
phase (roadmap P2); the native engine (`zuspec-rt-eng`) links against these
shared headers.
