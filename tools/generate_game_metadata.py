#!/usr/bin/env python3
"""Export offline game-card metadata from the matching MAME -listxml registry.

Export .local/mame-full.xml with the matching host core first. The ROM-free
command file can contain `mame -listxml`; see README for the host invocation.
"""
from pathlib import Path
import argparse
import json
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--xml', type=Path, default=root / '.local/mame-full.xml')
args = p.parse_args()
source = root / '.deps/PS5_MAME-8f90a8135ef402e04042160537b76cf6b17bcfa5'
compiled = set(re.findall(r'GAME_EXTERN\(([^)]+)\)', (source / 'build/arcade-all/generated/mame/arcade_all/drivlist.cpp').read_text())) - {'___empty'}
selection = json.loads((root / '.local/arcade-driver-selection.json').read_text())['drivers']
if set(selection) != compiled:
    raise SystemExit('Metadata selection does not match the compiled PS5 registry')
boards = {
    'cps1': 'Capcom CPS1', 'cps2': 'Capcom CPS2', 'cps3': 'Capcom CPS3',
    'neogeo': 'Neo Geo', 'namcos11': 'Namco System 11', 'namcos12': 'Namco System 12',
    'namcos22': 'Namco System 22', 'namcos23': 'Namco System 23', 'namcos2': 'Namco System 2',
    'namcos1': 'Namco System 1', 'taitotx': 'Taito Type X', 'taitof2': 'Taito F2',
    'taito_f3': 'Taito F3', 'taitogn': 'Taito G-NET', 'taitosj': 'Taito SJ',
    'pgm': 'IGS PGM', 'pgm2': 'IGS PGM2', 'naomi': 'Sega NAOMI', 'naomi2': 'Sega NAOMI 2',
    'segas16a': 'Sega System 16A', 'segas16b': 'Sega System 16B', 'segas18': 'Sega System 18',
    'segas32': 'Sega System 32', 'system1': 'Sega System 1', 'stv': 'Sega ST-V',
    'model1': 'Sega Model 1', 'model2': 'Sega Model 2', 'model3': 'Sega Model 3',
    'astrocde': 'Bally Astrocade', 'gridlee': 'Videa Gridlee', 'pacman': 'Namco Pac-Man',
    'galaga': 'Namco Galaga', 'mcr': 'Bally Midway MCR', 'mcr3': 'Bally Midway MCR III',
    'neogeo_noslot': 'Neo Geo', 'zn': 'Sony ZN', 'konamigx': 'Konami GX',
}
def clean(value):
    return (value or '').replace('\t', ' ').replace('\r', ' ').replace('\n', ' ')
def region_for(title, board):
    # Report only explicit region text; Neo Geo's standard region is BIOS-selectable.
    variants = ' '.join(re.findall(r'\(([^)]*)\)', title))
    for pattern, label in [
        (r'\bWorld\b', 'World'), (r'\b(?:USA|US|U\.S\.A\.)\b', 'USA'),
        (r'\b(?:Europe|European|Euro)\b', 'Europe'), (r'\b(?:Japan|Japanese)\b', 'Japan'),
        (r'\b(?:Korea|Korean)\b', 'Korea'), (r'\b(?:Hong Kong)\b', 'Hong Kong'),
        (r'\bTaiwan\b', 'Taiwan'), (r'\b(?:China|Chinese)\b', 'China'),
        (r'\b(?:Asia|Asian)\b', 'Asia'), (r'\bAustralia\b', 'Australia'),
        (r'\bGermany\b', 'Germany'), (r'\bSpain\b', 'Spain'), (r'\bItaly\b', 'Italy'),
        (r'\bFrance\b', 'France'), (r'\bBrazil\b', 'Brazil'), (r'\bUK\b', 'UK'),
    ]:
        if re.search(pattern, variants, re.I):
            return label
    return 'Multi' if board in ('neogeo', 'neogeo_noslot') else 'Unknown'
records = {}
for _, machine in ET.iterparse(args.xml, events=('end',)):
    if machine.tag != 'machine':
        continue
    name = machine.get('name')
    if name in compiled:
        title = machine.findtext('description') or name
        driver = machine.find('driver')
        controls = machine.find('input')
        board = Path(machine.get('sourcefile', '')).stem
        status = driver.get('status', 'unknown') if driver is not None else 'unknown'
        save = driver.get('savestate', 'unknown') if driver is not None else 'unknown'
        players = controls.get('players', '0') if controls is not None else '0'
        records[name] = tuple(map(clean, (title, machine.findtext('manufacturer'), machine.findtext('year'),
            boards.get(board, 'Unknown'), region_for(title, board), players, status, save)))
    machine.clear()
if set(records) != compiled:
    raise SystemExit('XML does not cover the compiled PS5 registry: ' + str(sorted(compiled - records.keys())[:20]))
output = root / 'assets/mame-metadata.tsv'
output.write_text(''.join('\t'.join((name, *records[name])) + '\n' for name in sorted(compiled)), encoding='utf-8')
print('Generated game-card metadata:', len(records), 'compiled entries')
for name in ('dino', 'mslug3', 'kof98um', 'ffight', 'tekken3'):
    print(name, records[name])
