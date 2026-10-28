#!/usr/bin/env python3
"""Build Inter glyph atlases for the native OpenGL frontend (Pillow only at build time)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[1]
header='#pragma once\nnamespace arcade::native {\nconstexpr float glyph_widths[3][96] = {\n'
for index, weight in enumerate((400,800,900)):
    font=ImageFont.truetype(str(root/'assets/Inter.ttf'),48)
    font.set_variation_by_axes([14,weight])
    atlas=Image.new('RGBA',(1024,480));draw=ImageDraw.Draw(atlas);widths=[]
    for code in range(32,128):
        i=code-32
        draw.text(((i%16)*64,(i//16)*80),chr(code),font=font,fill='white')
        widths.append(f'{font.getlength(chr(code)):.6f}f')
    atlas.save(root/f'assets/native-font-{index}.png')
    header+='{'+','.join(widths)+'},\n'
header+='};\n}\n'
(root/'native/gfx/font_metrics.hpp').write_text(header)
