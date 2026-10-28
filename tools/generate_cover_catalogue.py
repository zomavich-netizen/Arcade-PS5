#!/usr/bin/env python3
"""Export exact MAME display names and clone parents for artwork lookup."""
from pathlib import Path
import ctypes,json
root=Path(__file__).resolve().parents[1]
class Driver(ctypes.Structure):
 _fields_=[(name,ctypes.c_void_p) for name in ('type','parent','year','maker','machine','input','init','rom','compatible','layout')]+[('flags',ctypes.c_uint32),('name',ctypes.c_char*17)]
class DeviceType(ctypes.Structure):
 _fields_=[('creator',ctypes.c_void_p),('typeinfo',ctypes.c_void_p),('shortname',ctypes.c_char_p),('fullname',ctypes.c_char_p)]
lib=ctypes.CDLL(str(root/'.local/cores/mame_libretro.dylib'))
count=ctypes.c_size_t.in_dll(lib,'_ZN11driver_list14s_driver_countE').value
registry=(ctypes.POINTER(Driver)*count).in_dll(lib,'_ZN11driver_list16s_drivers_sortedE')
records={}
for pointer in registry:
 d=pointer.contents;name=d.name.decode();t=ctypes.cast(d.type,ctypes.POINTER(DeviceType)).contents
 assert t.shortname.decode()==name,name
 records[name]=(ctypes.string_at(d.parent).decode(),t.fullname.decode('utf-8'))
selection=json.loads((root/'.local/arcade-driver-selection.json').read_text())['drivers']
assert records['dino'][1]=='Cadillacs and Dinosaurs (World 930201)'
output=root/'assets/mame-artwork.tsv'
output.write_text(''.join(name+'\t'+records[name][0]+'\t'+records[name][1].replace('\t',' ').replace('\n',' ')+'\n' for name in sorted(selection)),encoding='utf-8')
print('Generated artwork catalogue:',len(selection),'arcade/BIOS entries')
