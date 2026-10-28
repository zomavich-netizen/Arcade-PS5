#!/usr/bin/env python3
"""Back up, migrate and verify console content. --clean-old removes verified sources.

Cleanup also requires the installed application's metadata to name the new paths.
Existing different destination files and symbolic links stop migration for review.
"""
from pathlib import Path, PurePosixPath
from datetime import datetime, timezone
import argparse, hashlib, json, time, urllib.request, urllib.parse, urllib.error

root = Path(__file__).resolve().parents[1]
base = 'http://192.168.0.181:8888'
sources = ('/data/arcade-roms', '/data/homebrew/PPSA99984/roms', '/data/homebrew/PPSA99984/bios')

def post(endpoint, values=None):
    request = urllib.request.Request(base + endpoint, data=urllib.parse.urlencode(values or {}).encode())
    with urllib.request.urlopen(request, timeout=120) as response:
        result = json.load(response)
    if not result.get('ok'):
        raise RuntimeError(result)
    return result

def listing(path):
    try:
        return post('/api/list?' + urllib.parse.urlencode({'path': path}))['entries']
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return None
        raise

def walk(path):
    result = []
    for entry in listing(path) or []:
        if entry['type'] == 'd':
            result.extend(walk(entry['path']))
        elif entry['type'] == '-':
            result.append(entry)
        else:
            raise RuntimeError('Unsupported file type; preserving source: ' + entry['path'])
    return result

def download(path, output=None):
    task = post('/api/download/prepare', {'paths': path})['task_id']
    digest = hashlib.sha256()
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
    stream = output.open('wb') if output else None
    try:
        with urllib.request.urlopen(base + '/api/download?' + urllib.parse.urlencode({'id': task}), timeout=120) as response:
            while chunk := response.read(1024 * 1024):
                digest.update(chunk)
                if stream:
                    stream.write(chunk)
    finally:
        if stream:
            stream.close()
    post('/api/tasks?' + urllib.parse.urlencode({'download_ack': task}))
    return digest.hexdigest()

def mkdir(path):
    if listing(path) is not None:
        return
    parent = str(PurePosixPath(path).parent)
    mkdir(parent)
    post('/api/mkdir?' + urllib.parse.urlencode({'path': parent, 'name': PurePosixPath(path).name}))

def upload(source, destination):
    parent, name = str(PurePosixPath(destination).parent), PurePosixPath(destination).name
    mkdir(parent)
    size = source.stat().st_size
    task = post('/api/upload/prepare', {'path': parent, 'src': 'Arcade content migration', 'total': size,
        'count': 1, 'rels': name, 'sizes': str(size), 'overwrite': '0'})['task_id']
    headers = {'Content-Type': 'application/octet-stream', 'X-WFM-Task-ID': str(task),
        'X-WFM-Path': urllib.parse.quote(parent, safe=''), 'X-WFM-Rel': urllib.parse.quote(name, safe=''),
        'X-WFM-Size': str(size), 'X-WFM-Overwrite': '0'}
    with urllib.request.urlopen(urllib.request.Request(base + '/api/upload-file', data=source.read_bytes(), headers=headers), timeout=120) as response:
        result = json.load(response)
    if not result.get('ok'):
        raise RuntimeError(result)
    post('/api/upload/finish?' + urllib.parse.urlencode({'task_id': task}))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clean-old', action='store_true')
    args = parser.parse_args()
    backup = root / '.local/console-backups' / ('content-migration-' + datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S'))
    backup.mkdir(parents=True, exist_ok=False)
    snapshots = {folder: walk(folder) for folder in sources if listing(folder) is not None}
    records = []
    receipt = {'backup': str(backup), 'files': records, 'old_folders_removed': False}
    mkdir('/data/arcade/roms')
    mkdir('/data/arcade/bios')
    for folder, entries in snapshots.items():
        for entry in entries:
            source = entry['path']
            relative = PurePosixPath(source).relative_to(folder)
            category = 'bios' if folder.endswith('/bios') or (folder == '/data/arcade-roms' and relative == PurePosixPath('qsound.zip')) else 'roms'
            destination = '/data/arcade/' + category + '/' + str(relative)
            local = backup / source.lstrip('/')
            digest = download(source, local)
            if local.stat().st_size != entry['size']:
                raise RuntimeError('Source size changed: ' + source)
            existing = {e['name']: e for e in listing(str(PurePosixPath(destination).parent)) or []}
            if PurePosixPath(destination).name in existing:
                if download(destination) != digest:
                    raise RuntimeError('Destination conflict; originals retained: ' + destination)
            else:
                upload(local, destination)
                if download(destination) != digest:
                    raise RuntimeError('Copied checksum mismatch: ' + destination)
            records.append({'source': source, 'destination': destination, 'sha256': digest, 'size': entry['size']})
            (backup / 'MIGRATION.json').write_text(json.dumps(receipt, indent=2) + '\n')
            print('SHA256 verified:', source, '->', destination, flush=True)
    (backup / 'MIGRATION.json').write_text(json.dumps(receipt, indent=2) + '\n')
    if args.clean_old and snapshots:
        # Never remove content while the installed frontend still defaults to its old location.
        meta_file = backup / 'installed-BUILD.json'
        download('/data/homebrew/PPSA99984/BUILD.json', meta_file)
        if json.loads(meta_file.read_text()).get('content_directories') != {'roms': '/data/arcade/roms', 'bios': '/data/arcade/bios'}:
            raise RuntimeError('Install the new folder-path build before cleaning old directories')
        sums_file = backup / 'installed-SHA256SUMS'
        download('/data/homebrew/PPSA99984/SHA256SUMS', sums_file)
        sums = {relative: digest for digest, relative in (line.split('  ', 1) for line in sums_file.read_text().splitlines())}
        if hashlib.sha256(meta_file.read_bytes()).hexdigest() != sums.get('BUILD.json') or download('/data/homebrew/PPSA99984/eboot.bin') != sums.get('eboot.bin'):
            raise RuntimeError('Installed executable or metadata checksum mismatch; preserving old folders')
        for folder, entries in snapshots.items():
            if walk(folder) != entries:
                raise RuntimeError('Source directory changed; cleanup stopped: ' + folder)
        for record in records:
            if download(record['source']) != record['sha256'] or download(record['destination']) != record['sha256']:
                raise RuntimeError('Content changed; cleanup stopped: ' + record['source'])
        task = post('/api/delete', {'paths': '\n'.join(snapshots)})['task_id']
        for _ in range(120):
            tasks = post('/api/tasks').get('tasks', [])
            item = next((t for t in tasks if str(t['id']) == str(task)), None)
            if item and item['state'] in ('failed', 'canceled'):
                raise RuntimeError(item)
            try:
                if all(listing(folder) is None for folder in snapshots):
                    break
            except urllib.error.HTTPError as error:
                # The file manager blocks directory reads while deletion runs.
                if error.code != 409:
                    raise
            time.sleep(.5)
        else:
            raise RuntimeError('Cleanup has not finished; inspect task ' + str(task))
        receipt['old_folders_removed'] = True
        (backup / 'MIGRATION.json').write_text(json.dumps(receipt, indent=2) + '\n')
    (root / '.local/content-migration.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('Migration verified; old folders removed:', receipt['old_folders_removed'], flush=True)

if __name__ == '__main__':
    main()
