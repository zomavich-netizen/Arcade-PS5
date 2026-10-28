#!/usr/bin/env python3
"""Select arcade systems using the matching MAME host core's machine flags.
Validate the entire registry against pinned sources before trusting classifications.
"""
from pathlib import Path
import ctypes,json,re,subprocess
root=Path(__file__).resolve().parents[1]
source=root/'.deps/PS5_MAME-8f90a8135ef402e04042160537b76cf6b17bcfa5'
# The pinned fork moved this driver but retained its previous mame.lst source path.
# Filter-based project generation needs the current path.
listing=source/'src/mame/mame.lst'
text=listing.read_text()
if '@source:misc/cv1k.cpp' in text:
 listing.write_text(text.replace('@source:misc/cv1k.cpp','@source:cave/cv1k.cpp'))
class Driver(ctypes.Structure):
 _fields_=[(name,ctypes.c_void_p) for name in ('type','parent','year','manufacturer','machine_creator','input','init','rom','compatible','layout')]+[('flags',ctypes.c_uint32),('name',ctypes.c_char*17)]
if Driver.flags.offset!=80 or Driver.name.offset!=84:raise SystemExit('Unexpected host game_driver ABI')
lib=ctypes.CDLL(str(root/'.local/cores/mame_libretro.dylib'))
count=ctypes.c_size_t.in_dll(lib,'_ZN11driver_list14s_driver_countE').value
if not 40000<count<100000:raise SystemExit('Unexpected full host driver registry')
registry=(ctypes.POINTER(Driver)*count).in_dll(lib,'_ZN11driver_list16s_drivers_sortedE')
records={p.contents.name.decode():{'flags':p.contents.flags,'parent':ctypes.string_at(p.contents.parent).decode()} for p in registry}
listed=re.sub(r'/\*.*?\*/','',(source/'src/mame/mame.lst').read_text(),flags=re.S)
expected={line.split('//',1)[0].strip() for line in listed.splitlines() if re.fullmatch(r'[A-Za-z0-9_]+',line.split('//',1)[0].strip())}
if set(records)-{'___empty'}!=expected:raise SystemExit('Host and pinned source registries differ; refusing classifications')
# machine_flags::TYPE_ARCADE=8; orientation occupies the low three bits.
arcade=sorted(name for name,record in records.items() if name!='___empty' and record['flags']&0x38==0x08)
if not {'dino','pacman','sf2','mslug3','robby'}<=set(arcade):raise SystemExit('Arcade classification sanity check failed')
if {'nes','a2600','apple2'}&set(arcade):raise SystemExit('Non-arcade system leaked into arcade selection')
selected=set(arcade)
bios_dependencies=set()
for name in arcade:
 parent=records[name]['parent']
 visited=set()
 while parent!='0' and parent not in visited:
  visited.add(parent)
  if parent not in records:raise SystemExit('Missing parent: '+parent)
  if parent not in selected:
   if not records[parent]['flags']&0x80:raise SystemExit('Arcade system requires non-BIOS non-arcade parent: '+name+' -> '+parent)
   selected.add(parent);bios_dependencies.add(parent)
  parent=records[parent]['parent']
output=source/'src/mame/arcade_all.flt' 
output.write_text('// Generated from MAME 0.289 machine_flags::TYPE_ARCADE; explicit inclusion and exclusion.\n'+''.join('+'+name+'\n' for name in sorted(selected))+''.join('-'+name+'\n' for name in sorted(expected-selected)))
# MAME's +name records add to the default registry; the -name records above
# are required to remove non-arcade systems. Check the real generator up front.
generated=subprocess.check_output(['python3',str(source/'scripts/build/makedep.py'),'-r',str(source),'driverlist',str(source/'src/mame/mame.lst'),'-f',str(output)],text=True)
actual=set(re.findall(r'GAME_EXTERN\(([^)]+)\)',generated))-{'___empty'}
if actual!=selected:raise SystemExit('MAME filter registry does not match the arcade selection')
receipt={'scope':'arcade-all','arcade_driver_count':len(arcade),'bios_dependencies':sorted(bios_dependencies),'driver_count':len(selected),'excluded_non_arcade_count':len(expected)-len(arcade),'classification':'MAME machine_flags::TYPE_ARCADE','source_registry_count':len(expected),'drivers':sorted(selected)}
(root/'.local/arcade-driver-selection.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Selected',len(arcade),'arcade drivers; excluded',len(expected)-len(arcade),'non-arcade systems',flush=True)
print('Generated filter:',output,flush=True)
