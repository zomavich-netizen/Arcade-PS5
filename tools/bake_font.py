#!/usr/bin/env python3
"""Bake OFL Barlow fonts to SDL-loadable atlases; requires Pillow."""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
root = Path(__file__).resolve().parents[1]
header = '#pragma once\n'
for style, source in [('regular', 'Barlow-Regular.ttf'), ('bold', 'Barlow-Bold.ttf')]:
    font = ImageFont.truetype(str(root/'assets'/source), 48)
    image = Image.new('RGBA', (16*64, 6*72))
    draw = ImageDraw.Draw(image)
    widths=[]
    for code in range(32, 128):
        i=code-32
        draw.text(((i%16)*64, (i//16)*72), chr(code), font=font, fill='white')
        widths.append(round(font.getlength(chr(code))))
    # BMP V4 preserves alpha in SDL_LoadBMP.
    image.save(root/f'assets/font-{style}.bmp')
    header += f'constexpr int font_{style}_widths[96] = '+'{'+','.join(map(str,widths))+'};\n'
(root/'src/font_widths.hpp').write_text(header)
