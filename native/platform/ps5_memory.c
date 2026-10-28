// SPDX-License-Identifier: GPL-3.0-or-later
#include <stddef.h>

// Large Neo Geo sets keep compressed-ROM scratch, encrypted graphics and
// expanded sprite data alive together. Use the native allocator's existing
// CPU-cached direct-memory path instead of its 128 MiB preview-sized default.
const size_t ps5_opengl_heap_size = 1024u * 1024u * 1024u;
