#!/usr/bin/env python3
"""Build and stage a native PS5 development title. Does not deploy or launch it."""
from pathlib import Path
import subprocess,os,json,shutil,runpy,hashlib
root=Path(__file__).resolve().parents[1];build=root/'build/ps5';build.mkdir(parents=True,exist_ok=True)
sdk=root/'.deps/payload-v0.42/ps5-payload-sdk';gl=root/'.deps/ps5-opengl-sdk-1.0.2/sdk';foundation=root/'vendor/native-app'
llvm=Path('/opt/homebrew/opt/llvm@20/bin');host='/usr/bin/clang++'
env=dict(os.environ,LLVM_CONFIG=str(llvm/'llvm-config'),PS5_PAYLOAD_SDK=str(sdk))
def run(args):subprocess.run(list(map(str,args)),env=env,check=True)
vulkan=(root/'.deps/arcade-radv/lib/libvulkan_radeon.ps5.a').is_file()
run(['python3',root/'tools/check_native_compile.py','--static-mame',*(['--vulkan'] if vulkan else [])])
run(['python3',root/'tools/build_cover_http.py'])
compiler=llvm/'clang++'
common=[f'-ffile-prefix-map={root}=.',f'-fdebug-prefix-map={root}=.','-target','x86_64-sie-ps5','-fvisibility-nodllstorageclass=default','-isysroot',sdk,'-isystem',sdk/'target/include/c++/v1','-isystem',sdk/'target/include','-fno-stack-protector','-fno-plt','-femulated-tls','-O2']
for source in ['app_crt.cpp','app_heap.c']:
 flags=['-x','c','-std=c11'] if source.endswith('.c') else ['-std=c++17','-fexceptions','-fcxx-exceptions']
 run([compiler,*common,*flags,'-c',foundation/source,'-o',build/(source+'.o')])
# Reproduce the homebrew title's runtime and format tools from retained GPL source.
tool=build/'native-tool'
run([host,'-std=c++20','-O2',foundation/'native_app_builder.cpp',foundation/'self_container.cpp',foundation/'elf_object.cpp',foundation/'sce_module_writer.cpp','-lz','-o',tool])
run([host,'-std=c++20','-O2',foundation/'libc_builder.cpp','-o',build/'libc-builder'])
run([build/'libc-builder',foundation/'runtime/api-surface.txt',foundation/'runtime/imports.txt',build/'libc.raw.elf'])
run([tool,'self','--sign','--in',build/'libc.raw.elf','--out',build/'libc.prx'])
mame=root/'.deps/PS5_MAME-8f90a8135ef402e04042160537b76cf6b17bcfa5'
manifest_path=os.environ.get('MAME_BUILD_MANIFEST') or os.environ.get('ARCADE_MAME_MANIFEST')
if not manifest_path and (root/'.local/arcade-all-backend.json').is_file():manifest_path=str(root/'.local/arcade-all-backend.json')
if not manifest_path and (root/'.local/full-mame-backend.json').is_file():manifest_path=str(root/'.local/full-mame-backend.json')
manifest=json.loads(Path(manifest_path).read_text()) if manifest_path else None
entry=Path(manifest['entry_archive']) if manifest else mame/'arcade_libretro.a'
if not entry.is_file() or entry.stat().st_size<1000:raise SystemExit('Build the native MAME subset first: python3 tools/build_mame_subset.py')
compiled=json.loads((root/'build/native-objects/receipt.json').read_text())['sources']
objects=[root/'build/native-objects'/(source.replace('/','_')+'.o') for source in compiled]+[build/'app_crt.cpp.o',build/'app_heap.c.o']
archives=[Path(p) for p in manifest['archives']] if manifest else sorted((mame/'build/libretro/bin').rglob('*.a'))
output=build/'llvm-pie.elf'
flags=['--defsym=kernel_mprotect=0','--defsym=__cxa_thread_atexit_impl=0','--defsym=__dlopen=0','--defsym=__dlsym=0','--defsym=__dladdr=0','--defsym=__dlclose=0','--defsym=__dlerror=0','-L',sdk/'target/lib','-L',gl/'lib','-T',foundation/'ps5-pie.ld','-T',foundation/'unwind.ld','--eh-frame-hdr','--gc-sections','--version-script',foundation/'app-symbols.map','-e','_start','--no-dynamic-linker','--error-limit=30','-Map='+str(build/'link.map'),'-u','ps5_agc_gate2_run']
for name in ['malloc','calloc','realloc','free','posix_memalign','malloc_usable_size']:flags.append('--wrap='+name)
agc_driver=gl/'lib/libSceAgcDriver.so'
vulkan_inputs=[]
if vulkan:
 isolation=root/'build/radv-isolated/manifest.json'
 expected=hashlib.file_digest((root/'.deps/arcade-radv/lib/libvulkan_radeon.ps5.a').open('rb'),'sha256').hexdigest()
 if not isolation.is_file() or json.loads(isolation.read_text()).get('source_sha256')!=expected:run(['python3',root/'tools/isolate_vulkan.py'])
 stub_source=root/'vendor/prosperoeden/stubs/libSceAgcDriver.c'
 run([compiler,*common,'-x','c','-std=c11','-fPIC','-c',stub_source,'-o',build/'agc-driver-stub.o'])
 run([Path(os.environ.get('PS5_LLD',str(root.parent/'Pulse/.local/linker20/lld@20/20.1.8/bin/ld.lld'))),'--shared','-soname','libSceAgcDriver.prx','-o',build/'libSceAgcDriver.so',build/'agc-driver-stub.o'])
 agc_driver=build/'libSceAgcDriver.so'
 platform=root/'.deps/arcade-radv/lib/libps5platform.a'
 raw_symbols=subprocess.check_output([str(llvm/'llvm-nm'),'--defined-only',str(platform)],text=True)
 recipe=(root/'.deps/PS5_Vulkan-5b5e4fc2d80fdb67a4f61ba9e6a8a424026bb8a5/tools/radv-link.sh').read_text()
 import re
 # Bind the platform functions actually present in the pinned SDK, as its recipe does.
 names=set()
 for block in re.findall(r'for name in (.*?); do',recipe,re.S):
  for name in block.replace('\\',' ').split():
   if re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',name) and (' T ps5_'+name+'\n') in raw_symbols:names.add(name)
 # Keep Arcade's qualified allocator, thread entry shims and public C++ runtime.
 for name in sorted(names):flags.append('--defsym='+name+'=ps5_'+name)
 flags=[f for f in flags if f!='--defsym=__cxa_thread_atexit_impl=0']
 flags+=['--wrap=pthread_create','--wrap=pthread_join','--wrap=pthread_detach','--wrap=fclose','--wrap=fflush']
 vulkan_inputs=['--whole-archive',root/'build/radv-isolated/libvulkan_radeon.ps5.a','--no-whole-archive',platform,root/'.deps/arcade-radv/lib/libclang_rt.builtins-x86_64.a']
stubs=[p for p in sorted((sdk/'target/lib').glob('*.so')) if p.name not in ('libSceAgc.so','libSceAgcDriver.so','libkernel_web.so','libkernel_sys.so')]
linker=Path(os.environ.get('PS5_LLD',str(root.parent/'Pulse/.local/linker20/lld@20/20.1.8/bin/ld.lld')))
link_command=[linker,'-m','elf_x86_64','-pie','-z','max-page-size=0x4000','-mllvm','-emulated-tls','--hash-style=gnu',*flags,'-o',output,'--start-group',*objects,root/'build/cover-http/http.o',entry,*archives,*vulkan_inputs,root/'.deps/pacbrew-0.40.2/opt/ps5-payload-sdk/target/user/homebrew/lib/libssl.a',root/'.deps/pacbrew-0.40.2/opt/ps5-payload-sdk/target/user/homebrew/lib/libcrypto.a',gl/'lib/libPS5OpenGL.a',sdk/'target/lib/libc++.a',sdk/'target/lib/libc++abi.a',sdk/'target/lib/libunwind.a','--end-group','--as-needed',*stubs,gl/'lib/libSceAgc.so',agc_driver,sdk/'target/lib/libc.a']
run(link_command)
if vulkan:
 undefined=subprocess.check_output([str(llvm/'llvm-nm'),'-u',str(output)],text=True)
 optional=[]
 for line in undefined.splitlines():
  parts=line.split()
  if len(parts)==2 and parts[0]=='w' and parts[1].startswith(('radv_','vk_','arcade_radv_private_','wsi_','ctx_roll_','rmv_','rra_','utrace_','sqtt_','annotate_','threaded_')):optional.append(parts[1])
 if optional:
  # Missing weak Mesa entry points are NULL dispatch slots, never native imports.
  response=build/'vulkan-optional-null.rsp';response.write_text(''.join('--defsym='+name+'=0\n' for name in optional))
  run([*link_command,'@'+str(response)])
  print('Optional Vulkan dispatch slots resolved to NULL:',len(optional),flush=True)

# Native kernel/libc imports must resolve before payload-only libc fallbacks.
# In particular mmap must be imported from libkernel, never the payload's raw syscall.
import re
symbols=subprocess.check_output([str(llvm/'llvm-nm'),str(output)],text=True)
for name in ['mmap','munmap','sysconf']:
 if not re.search(r'^\s+U '+name+r'$',symbols,re.M):
  raise SystemExit('Native import gate failed: '+name+' is not a kernel import')
print('Native memory mapping/sysconf imports: PASS',flush=True)
# Conversion uses the SDK's import facades, with the graphics SDK's AGC facades.
stub_flags=[]
for path in stubs:stub_flags+=['--stub',path]
stub_flags+=['--stub',sdk/'target/lib/libkernel_web.so','--stub',gl/'lib/libSceAgc.so','--stub',agc_driver]
run([tool,'link','--in',output,'--out',build/'eboot.elf',*stub_flags,'--module-sdk','0x02000009','--companion-sdk','0x08050001','--file-name','eboot.elf'])
dest=root/'build/title/PPSA99984';(dest/'sce_sys').mkdir(parents=True,exist_ok=True);(dest/'sce_module').mkdir(exist_ok=True)
for folder in ('roms','bios'):
 if (dest/folder).is_dir() and not any((dest/folder).iterdir()):(dest/folder).rmdir()
run([tool,'self','--sign','--in',build/'eboot.elf','--out',dest/'eboot.bin','--magic','0x1D3D154F'])
runpy.run_path(str(root/'tools/check_load_alignment.py'))['check_load_alignment']((dest/'eboot.bin').read_bytes())
shutil.copyfile(build/'libc.prx',dest/'sce_module/libc.prx')
if (dest/'assets').exists():shutil.rmtree(dest/'assets')
shutil.copytree(root/'assets',dest/'assets')
shutil.copyfile(root/'ps5/sce_sys/param.json',dest/'sce_sys/param.json');shutil.copyfile(root/'ps5/sce_sys/icon0.png',dest/'sce_sys/icon0.png')
for name in ('pic0.png','pic1.png','pic0.dds','snd0.at9'):shutil.copyfile(root/'ps5/sce_sys'/name,dest/'sce_sys'/name)
run([tool,'self','--inspect','--file',dest/'eboot.bin']);run([tool,'self','--inspect','--file',dest/'sce_module/libc.prx'])
for path in [dest,*dest.rglob('*')]:path.chmod(0o777)
backend_receipt=dict(manifest or {'scope':'subset','entry_archive':str(entry),'archives':list(map(str,archives))});backend_receipt['vulkan_compiled']=vulkan
(build/'backend.json').write_text(json.dumps(backend_receipt,indent=2)+'\n')
run(['python3',root/'tools/finalize_package.py'])
print('PS5 load alignment: PASS')
print('Staged native PS5 development title:',dest)
print('No console deployment or qualification has been performed.')
