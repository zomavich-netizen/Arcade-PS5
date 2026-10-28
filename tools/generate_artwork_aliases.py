#!/usr/bin/env python3
"""Resolve MAME IDs to exact artwork filenames; refresh the source with --refresh."""
from pathlib import Path, PurePosixPath
import argparse
import json
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
CATEGORIES = ('Named_Boxarts', 'Named_Titles', 'Named_Snaps')

def thumbnail(title):
    return ''.join('_' if char in '&*/:`<>?\\|"' else char for char in title)

def key(title):
    return re.sub(r'[\W_]+', ' ', thumbnail(title).casefold()).strip()

def base_title(title):
    return re.sub(r'(?:\s*\([^()]*\))+$', '', title).strip()

def resolve(records, paths):
    exact = {category: set() for category in CATEGORIES}
    folded = {category: {} for category in CATEGORIES}
    generic = {category: {} for category in CATEGORIES}
    for path in paths:
        parts = PurePosixPath(path).parts
        if len(parts) != 2 or parts[0] not in exact or not parts[1].endswith('.png'):
            continue
        category, filename = parts
        title = filename[:-4]
        exact[category].add(title)
        folded[category].setdefault(key(title), set()).add(title)
        if base_title(title) == title:
            generic[category].setdefault(key(title), set()).add(title)
    aliases = []
    for rom, title in sorted(records.items()):
        for category in CATEGORIES:
            if thumbnail(title) in exact[category]:
                continue
            candidates = folded[category].get(key(title), set())
            if not candidates:
                candidates = generic[category].get(key(base_title(title)), set())
            # Never choose between different releases or ambiguous title matches.
            if len(candidates) == 1:
                aliases.append((rom, category, next(iter(candidates))))
    return aliases

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--refresh', action='store_true')
    parser.add_argument('--index', type=Path, default=ROOT / '.local/libretro-artwork-tree.json')
    args = parser.parse_args()
    if args.refresh or not args.index.exists():
        request = urllib.request.Request('https://api.github.com/repos/libretro-thumbnails/MAME/git/trees/master?recursive=1', headers={'User-Agent': 'Arcade-artwork-catalogue'})
        with urllib.request.urlopen(request, timeout=60) as response:
            index = json.load(response)
        if index.get('truncated') or not index.get('tree') or not index.get('sha'):
            raise SystemExit('Refusing an incomplete artwork index')
        args.index.parent.mkdir(parents=True, exist_ok=True)
        args.index.write_text(json.dumps(index))
    index = json.loads(args.index.read_text())
    if index.get('truncated'):
        raise SystemExit('Refusing a truncated artwork index')
    records = {}
    for line in (ROOT / 'assets/mame-artwork.tsv').read_text().splitlines():
        rom, parent, title = line.split('\t', 2)
        records[rom] = title
    aliases = resolve(records, [entry['path'] for entry in index['tree'] if entry['type'] == 'blob'])
    output = ROOT / 'assets/mame-artwork-aliases.tsv'
    output.write_text('# libretro-thumbnails/MAME tree ' + index['sha'] + '\n' + ''.join('\t'.join(row) + '\n' for row in aliases))
    print(f'Generated {len(aliases)} exact filename aliases for {len(set(row[0] for row in aliases))} ROM IDs')
    for row in aliases:
        if row[0] == 'kof98':
            print('\t'.join(row))

if __name__ == '__main__':
    main()
