#!/usr/bin/env python3
"""Bake straight SVG room geometry into native line segments, preserving opacity."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET
root = Path(__file__).resolve().parents[1]
lines = []
def visit(element, inherited=1.):
    opacity = inherited * float(element.get('opacity', '1'))
    tag = element.tag.rsplit('}', 1)[-1]
    if tag == 'rect':
        x, y, w, h = (float(element.get(k, '0')) for k in ('x','y','width','height'))
        lines.extend((a,b,c,d,opacity) for a,b,c,d in ((x,y,x+w,y),(x+w,y,x+w,y+h),(x+w,y+h,x,y+h),(x,y+h,x,y)))
    if tag == 'path':
        tokens = re.findall(r'[MLHVZmlhvz]|-?\d*\.?\d+', element.attrib['d'])
        i = 0
        x = y = 0.
        start = None
        while i < len(tokens):
            command = tokens[i]; i += 1
            if command == 'M':
                x, y = map(float, tokens[i:i+2]); i += 2; start = (x,y); continue
            if command == 'L': nx, ny = map(float,tokens[i:i+2]); i += 2
            elif command == 'H': nx, ny = float(tokens[i]), y; i += 1
            elif command == 'V': nx, ny = x, float(tokens[i]); i += 1
            elif command == 'Z': nx, ny = start
            else: raise ValueError('Unsupported SVG command: '+command)
            lines.append((x,y,nx,ny,opacity)); x,y = nx,ny
    for child in element: visit(child, opacity)
visit(ET.parse(root/'design/backgrounds/perspective-room.svg').getroot())
rows = ['{'+','.join(format(v,'.6f')+'f' for v in line)+'}' for line in lines]
(root/'native/gfx/vault_vectors.hpp').write_text('// Generated from design/backgrounds/perspective-room.svg; native line geometry.\n#pragma once\nnamespace arcade::native::vault {\ninline constexpr float room_lines[][5]={\n'+',\n'.join(rows)+'\n};\n}\n')
print('Native room geometry:', len(lines), 'segments')
