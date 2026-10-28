#!/usr/bin/env python3
"""Build the Arcade-only PS5 media-registration repair ELF from public sources."""
from pathlib import Path
import os, subprocess
root = Path(__file__).resolve().parents[1]
sdk = root / '.deps/payload-v0.42/ps5-payload-sdk'
sqlite = root / '.deps/PS5_MAME-8f90a8135ef402e04042160537b76cf6b17bcfa5/3rdparty/sqlite3'
build = root / 'build/ps5'
build.mkdir(parents=True, exist_ok=True)
llvm = Path(os.environ.get('LLVM_BINDIR', '/opt/homebrew/opt/llvm@20/bin'))
linker = Path(os.environ.get('PS5_LLD', str(root.parent / 'Pulse/.local/linker20/lld@20/20.1.8/bin/ld.lld')))
common = [str(llvm/'clang'), f'-ffile-prefix-map={root}=.', f'-fdebug-prefix-map={root}=.', '-target', 'x86_64-sie-ps5', '-fvisibility-nodllstorageclass=default', '-isysroot', str(sdk), '-isystem', str(sdk/'target/include'), '-I', str(sqlite), '-fno-stack-protector', '-fno-plt', '-femulated-tls', '-O2']
for source, output, extra in [(root/'tools/register_shell_media.c', build/'register-shell-media.o', []), (sqlite/'sqlite3.c', build/'shell-media-sqlite.o', ['-DSQLITE_ENABLE_JSON1', '-DSQLITE_THREADSAFE=0', '-DSQLITE_OMIT_LOAD_EXTENSION', '-DSQLITE_DQS=0'])]:
 subprocess.run(common + extra + ['-c', str(source), '-o', str(output)], check=True)
subprocess.run([str(linker), '-m', 'elf_x86_64', '-pie', '-T', str(root/'tools/shell-media.ld'), '--eh-frame-hdr', '-z', 'max-page-size=0x4000', '-mllvm', '-emulated-tls', '--hash-style=gnu', '-L', str(sdk/'target/lib'), str(sdk/'target/lib/crt1.o'), str(build/'register-shell-media.o'), str(build/'shell-media-sqlite.o'), '-lc', '-lkernel_sys', '-lSceLibcInternal', '-lSceNet', '-o', str(build/'register-shell-media.elf')], check=True)
print(build/'register-shell-media.elf')
