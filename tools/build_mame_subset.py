#!/usr/bin/env python3
"""Build a static PS5 MAME backend; --arcade-all selects every arcade driver."""
from pathlib import Path
import argparse,json,os,re,subprocess
parser=argparse.ArgumentParser()
parser.add_argument("--arcade-all",action="store_true",help="Build every arcade-classified MAME system and required BIOS roots")
parser.add_argument("--jobs",type=int,default=6)
options=parser.parse_args()
if options.jobs<1:parser.error("--jobs must be positive")
root=Path(__file__).resolve().parents[1]
source=root/'.deps/PS5_MAME-8f90a8135ef402e04042160537b76cf6b17bcfa5'
sdk=root/'.deps/payload-v0.42/ps5-payload-sdk'
for path in (sdk/'bin').iterdir():path.chmod(path.stat().st_mode|0o111)
main=source/'scripts/src/main.lua';code=main.read_text();old='configuration { "libretro*" }\n\t\tkind "SharedLib"';new='configuration { "libretro*" }\n\t\tkind "StaticLib"'
if old in code:main.write_text(code.replace(old,new,1))
elif new not in code:raise SystemExit('MAME libretro target layout changed')
aes=source/'3rdparty/lzma/C/AesOpt.c'
text=aes.read_text();anchor='#ifndef ATTRIB_AES'
fix='#ifdef __PROSPERO__\n/* PS5 Zen 2 has AES-NI, not VAES; use the upstream AES/software fallback. */\n#undef USE_INTEL_VAES\n#endif\n\n'
if fix not in text:aes.write_text(text.replace(anchor,fix+anchor,1))
# Search the sibling BIOS directory as well as the selected ROM folder.
retro_init=source/'src/osd/libretro/libretro-internal/retro_init.cpp'
code=retro_init.read_text()
old='snprintf(tmp_dir, sizeof(tmp_dir), "%s;%s%c%s%c%s;%s%c%s%c%s",\n            g_rom_dir,\n            retro_system_directory, slash, CORE_NAME, slash, "bios",'
new='snprintf(tmp_dir, sizeof(tmp_dir), "%s;%s%c..%cbios;%s%c%s%c%s;%s%c%s%c%s",\n            g_rom_dir, g_rom_dir, slash, slash,\n            retro_system_directory, slash, CORE_NAME, slash, "bios",'
if old in code:retro_init.write_text(code.replace(old,new,1))
elif new not in code:raise SystemExit('MAME ROM search path layout changed')
# The PS5 fork adds CAVE blitter units only for the original arcade subtarget.
video=source/'scripts/src/video_retro.lua'
code=video.read_text();old='if _OPTIONS["subtarget"]=="arcade" then';new='if _OPTIONS["subtarget"]=="arcade" or _OPTIONS["subtarget"]=="arcade_all" then'
if old in code:video.write_text(code.replace(old,new,1))
elif new not in code:raise SystemExit('MAME CAVE video build layout changed')
coin_bridge=source/'src/osd/libretro/libretro-internal/libretro.cpp'
code=coin_bridge.read_text()
if 'int arcade_mame_coin_lockout(' not in code:coin_bridge.write_text(code+'\n// Arcade frontend extension: defer queued coins until firmware unlocks the slot.\nextern "C" int arcade_mame_coin_lockout(unsigned slot)\n{\n   auto *manager = mame_machine_manager::instance();\n   if (!manager || !manager->machine()) return -1;\n   return manager->machine()->bookkeeping().coin_lockout_get_state(slot);\n}\n')
env=dict(os.environ,PS5_PAYLOAD_SDK=str(sdk),LLVM_CONFIG='/opt/homebrew/opt/llvm@20/bin/llvm-config',CC=str(sdk/'bin/prospero-clang'),CXX=str(sdk/'bin/prospero-clang++'),AR=str(sdk/'bin/prospero-ar'))
args=['make','-C',str(source),'-j'+str(options.jobs),'OSD=retro','CONFIG=libretro','TARGET=mame','SUBTARGET=arcade','SOURCES=src/mame/bally/astrocde.cpp,src/mame/bally/gridlee.cpp,src/mame/capcom/cps1.cpp','TARGETOS=freebsd','PTR64=1','CLANG_VERSION=20.1.8','PRECOMPILE=0','NOWERROR=1','REGENIE=1','CROSS_BUILD=1','PYTHON_EXECUTABLE=python3','NO_USE_MIDI=1','NO_USE_PORTAUDIO=1','NO_USE_BGFX=1','DONT_USE_NETWORK=1','NO_OPENGL=1','USE_QTDEBUG=0','OVERRIDE_CC='+env['CC'],'OVERRIDE_CXX='+env['CXX']+' -fexceptions -fcxx-exceptions','OVERRIDE_AR='+env['AR']]
if options.arcade_all:
 (root/'.local/arcade-all-backend.json').unlink(missing_ok=True)
 args=[a for a in args if not a.startswith(('SOURCES=','SUBTARGET='))]
 subprocess.run(['python3',str(root/'tools/generate_arcade_filter.py')],check=True)
 args+=['SUBTARGET=arcade_all','BUILDDIR=build/arcade-all','SOURCEFILTER=src/mame/arcade_all.flt']
if 'int arcade_mame_coin_counter(' not in coin_bridge.read_text():coin_bridge.write_text(coin_bridge.read_text()+'\n// Neo Geo has a firmware-driven coin meter: confirm acceptance, not just input.\nextern "C" int arcade_mame_coin_counter(unsigned slot)\n{\n   auto *manager = mame_machine_manager::instance();\n   if (!manager || !manager->machine()) return -1;\n   auto *machine = manager->machine();\n   const game_driver *system = &machine->system();\n   for (unsigned depth = 0; depth < 8; ++depth)\n   {\n      if (strcmp(system->name, "neogeo") == 0)\n         return machine->bookkeeping().coin_counter_get_count(slot);\n      int parent = driver_list::find(system->parent);\n      if (parent < 0) break;\n      system = &driver_list::driver(parent);\n   }\n   return -1;\n}\n')
print('MAME build mode:', 'arcade-all' if options.arcade_all else 'subset',flush=True)
subprocess.run(args,env=env,check=True)

# GENie's libretro shared-target overrides leave the StaticLib archive empty.
# Archive the five entrypoint objects explicitly; dependency archives stay separate.
build_tree=source/('build/arcade-all' if options.arcade_all else 'build')
subtarget='arcade_all' if options.arcade_all else 'arcade'
objects=build_tree/'libretro/obj/x64/libretro'
entries=[objects/'src/mame/mame.o',objects/'src/osd/libretro/libretro-internal/libretro.o',objects/'src/osd/libretro/retromain.o',objects/('generated/mame/'+subtarget+'/drivlist.o'),objects/'generated/version.o']
for entry in entries:
    if not entry.is_file():raise SystemExit('Missing native MAME entry object: '+str(entry))
entry_archive=source/('arcade_all_libretro.a' if options.arcade_all else 'arcade_libretro.a')
subprocess.run([str(sdk/'bin/prospero-ar'),'rcs',str(entry_archive),*map(str,entries)],env=env,check=True)
if options.arcade_all:
 driver_list=(build_tree/'generated/mame/arcade_all/drivlist.cpp').read_text()
 count=int(re.search(r's_driver_count\s*=\s*(\d+)',driver_list).group(1))
 selection=json.loads((root/'.local/arcade-driver-selection.json').read_text())
 expected=set(selection['drivers'])
 actual=set(re.findall(r'GAME_EXTERN\(([^)]+)\)',driver_list))-{'___empty'}
 if actual!=expected or count!=len(actual)+1:raise SystemExit('Arcade registry mismatch: missing='+str(sorted(expected-actual)[:10])+' extra='+str(sorted(actual-expected)[:10]))
 receipt={'scope':'arcade-all','mame_revision':'8f90a8135ef402e04042160537b76cf6b17bcfa5','entry_archive':str(entry_archive),'archives':[str(p) for p in sorted((build_tree/'libretro/bin').rglob('*.a'))],'driver_count':count,'arcade_driver_count':selection['arcade_driver_count'],'bios_dependencies':selection['bios_dependencies'],'bios_layout':'roms/../bios','source_filter':'machine_flags::TYPE_ARCADE plus required BIOS roots'}
 output=root/'.local/arcade-all-backend.json';output.parent.mkdir(exist_ok=True);output.write_text(json.dumps(receipt,indent=2)+'\n')
 print('All-arcade MAME backend receipt:',output,'drivers:',count,flush=True)
print('Static native MAME entry archive ready:',entry_archive,flush=True)
