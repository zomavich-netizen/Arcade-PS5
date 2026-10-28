#!/usr/bin/env python3
"""Compile production sources for PS5 against the actual pinned SDKs; no fake stubs.
This does not link, package, deploy or claim console validation.
"""
from pathlib import Path
import argparse,hashlib,subprocess,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--compiler',default='/opt/homebrew/opt/llvm@20/bin/clang++');p.add_argument('--sdk',type=Path,default=root/'.deps/payload-v0.42/ps5-payload-sdk');p.add_argument('--static-mame',action='store_true');p.add_argument('--vulkan',action='store_true');args=p.parse_args()
gl=root/'.deps/ps5-opengl-sdk-1.0.2/sdk'
lib=gl/'lib/libps5_opengl_core33.a'
if not lib.is_file() or hashlib.file_digest(lib.open('rb'),'sha256').hexdigest()!='7fd5d7a1beb7bf5af42e6a6936b88b1266cba3f224d3a60c3a6a09285e339ae1':raise SystemExit('The exact pinned ps5-opengl SDK is missing or changed')
files=['src/cover_art.cpp','src/library.cpp','src/settings.cpp','src/settings_menu.cpp','src/model.cpp','src/core.cpp','native/main.cpp','native/gfx/renderer.cpp','native/ui/gallery.cpp','native/ui/vault.cpp','native/platform/ps5.cpp','vendor/prosperoeden/pe/platform/display_egl.cpp','vendor/prosperoeden/radio_input.c']
files += ['native/platform/ps5_memory.c','native/platform/ps5_diagnostics.cpp','vendor/prosperoeden/runtime_shims.cpp']
if args.vulkan:files.append('native/gfx/vulkan_processor.cpp')
build=root/'build/native-objects';build.mkdir(parents=True,exist_ok=True)
common=[f'-ffile-prefix-map={root}=.',f'-fdebug-prefix-map={root}=.','-target','x86_64-sie-ps5','-fvisibility-nodllstorageclass=default','-isysroot',str(args.sdk),'-isystem',str(args.sdk/'target/include/c++/v1'),'-isystem',str(args.sdk/'target/include'),'-fno-stack-protector','-fno-plt','-femulated-tls','-DARCADE_PS5=1','-DGL_GLEXT_PROTOTYPES=1','-I',str(root/'native'),'-I',str(root/'src'),'-I',str(root/'vendor'),'-I',str(root/'vendor/prosperoeden'),'-I',str(gl/'include'),'-O2','-Wall','-Wextra']
if args.vulkan:common += ['-DARCADE_HAS_VULKAN=1','-I',str(root/'.deps/arcade-radv/include')]
if args.static_mame:common.append('-DARCADE_STATIC_MAME=1')
for source in files:
    language=['-x','c','-std=c11'] if source.endswith('.c') else ['-std=c++17','-fexceptions','-fcxx-exceptions']
    subprocess.run([args.compiler,*common,*language,'-c',str(root/source),'-o',str(build/(source.replace('/','_')+'.o'))],check=True)
    print('PS5 compiled:',source,flush=True)
(build/'receipt.json').write_text(json.dumps({'target':'x86_64-sie-ps5','sources':files,'opengl':'1.0.2','payload_sdk':'0.42','scope':'compile only; native link, MAME native ABI, packaging and console testing are separate gates'},indent=2)+'\n')
