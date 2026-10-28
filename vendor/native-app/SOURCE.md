Native title startup, ELF/FSELF conversion and reproducible libc runtime tools
come from BlackBearReloaded's ps5-native-app-boilerplate (GPL-3.0-or-later),
using the already-local copy preserved in Pulse/control-app/tooling/native.
They are copied without changing the existing Pulse project.

app_heap.c is the exact native allocator integration shipped with the verified
ps5-opengl SDK 1.0.2. unwind.ld comes from Swordpdf/ProsperoEden's tools/unwind.ld.
Original copyright/SPDX notices are retained in each source file.

The minimal allocation bridge app_cpp_runtime.cpp is retained for provenance
but is not linked by Arcade: the full SDK libc++ runtime is used instead.
