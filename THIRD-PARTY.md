# Third-party notices

Arcade is GPL-3.0-or-later. Components keep their own terms.

- **ProsperoEden**: BlackBearReloaded and contributors, GPL-3.0-or-later.
  `vendor/prosperoeden/` contains its EGL display, controller input, log helper and native C runtime shims and kernel directory reader.
  Native shutdown and diagnostic register layout also follow its lifecycle and crash reporter.
  Settings category structure, AGC import facade, RADV symbol isolation and
  display lifetime adaptations also follow its retained GPL source.
  `SOURCE.json` records provenance and local adaptations.
  https://github.com/Swordpdf/ProsperoEden
- **PS5 Native App Boilerplate**: BlackBearReloaded, GPL-3.0-or-later.
  Native title startup, ELF/FSELF tools and reproducible libc runtime source in
  `vendor/native-app/`; see its SOURCE.md and retained copyright headers.
- **ps5-opengl SDK 1.0.2**: BlackBearReloaded and upstream Mesa/compiler authors.
  Verified SDK is local in `.deps/`; its LICENSE, LICENSES, THIRD_PARTY_NOTICES.md,
  source and provenance are retained. See docs/SDK-RECEIPT.json. The native heap
  integration source is retained in vendor/native-app/app_heap.c (GPL-3.0-or-later).
- **PS5 Vulkan / Mesa RADV**: Mihawk-99 and Mesa contributors. Build recipes are
  GPL-3.0-or-later; Mesa retains its per-file licenses (primarily MIT). Pinned
  PS5_Vulkan 5b5e4fc2, PS5_Mesa 7b59ef27 and PS5_PayloadSDK b83202be sources
  are retained in `.deps/`, with ProsperoEden's display adaptations in `tools/`.
  See docs/VULKAN-RECEIPT.json and docs/settings/PORT.md.
- **LLVM compiler runtime 18**: Apache-2.0 WITH LLVM-exception and the retained
  notices in `.deps/arcade-radv/compiler-rt-COPYRIGHT`. Official Ubuntu package
  supplies the x86-64 builtins archive; no Linux runtime is shipped.
- **MAME/libretro PS5 fork**: MAMEdev, libretro and PS5 port contributors; source
  snapshot in `.deps/PS5_MAME-*`. MAME as a whole is GPL-2.0-or-later with many
  BSD-3-Clause files. Original source licenses are retained. Any release must
  include the matching corresponding source/patches and notices.
- **libretro API header**: libretro contributors, MIT; notice retained in
  `vendor/libretro.h`. Arcade does not include the RetroArch frontend.
- **stb_image**: Sean Barrett and contributors, public domain or MIT at choice;
  terms retained in vendor/stb_image.h.
- **Inter**: SIL Open Font License 1.1; assets/Inter-OFL.txt.
- **Barlow**: SIL Open Font License 1.1; assets/OFL.txt. Used by the superseded
  optional SDL reference renderer only.
- **SDL2**: zlib license, host installation only for the optional reference.
  Not used by Arcade's native production frontend or its default host preview.

The approved Gallery art is generated concept artwork. It is not authentic game
art. No commercial game, BIOS, keys or firmware is shipped. MAMEdev test ROMs
remain local and are excluded from source/release artifacts.

- **Automatic cover HTTPS support**: curl 8.18.0 (curl license), OpenSSL 3.5.2
  (Apache-2.0), and libpsl 0.21.5 (MIT), from the SHA-verified PacBrew 0.40.2
  dependency archive pinned by ProsperoEden. License texts are retained under
  `.deps/cover-licenses/` and included in the installed title's `licenses/`.
  Native resolver, certificate-path and socket adaptation is ProsperoEden's
  GPL-3.0-or-later `console_curl.c`, retained in `vendor/prosperoeden/network/`.
  Its libc substitutions are isolated to the HTTPS object.
- **Downloaded cover artwork**: retrieved on demand from Libretro's MAME
  thumbnail collection (https://thumbnails.libretro.com/MAME/). Artwork remains
  owned by its respective rights holders. No downloaded cover is bundled in
  the application; only MAME title/parent metadata is shipped for matching.

## Menu soundtrack

“Arcade — Arcade Loop” by Colorosse is distributed under the publisher's CC0-1.0 public-domain dedication. The unmodified PCM WAV is bundled as
`assets/arcade-menu.wav`; the declaration, source URLs and checksum are retained
in `assets/arcade-menu.LICENSE.txt`. A resampled, loudness-normalized ATRAC9 version is included as `ps5/sce_sys/snd0.at9` for PS5 home-screen playback.

Source: https://colorosse.com/assets/audio/music/arcade-music-loop

License: https://creativecommons.org/publicdomain/zero/1.0/
