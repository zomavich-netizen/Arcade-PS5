#!/usr/bin/env python3
"""Deploy the requested Cover Vault update with backups and a verified executable swap.

Only the existing Arcade title directory is updated. Game data, settings and
saves are never uploaded or deleted. Old executable inodes remain valid for a
currently suspended process: activation uses rename, not in-place rewriting.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import urllib.parse
import urllib.request

root = Path(__file__).resolve().parents[1]
stage = root / 'build/title/PPSA99984'
base = 'http://192.168.0.181:8888'
destination = '/data/homebrew/PPSA99984'
stamp = datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S')
backup = root / '.local/console-backups' / ('before-cover-vault-' + stamp)
backup.mkdir(parents=True, exist_ok=False)

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def post(path, values=None, query=False):
    encoded = urllib.parse.urlencode(values or {})
    request = urllib.request.Request(base + path + ('?' + encoded if query and encoded else ''),
                                     data=b'' if query else encoded.encode())
    with urllib.request.urlopen(request, timeout=40) as response:
        result = json.load(response)
    if not result.get('ok'):
        raise RuntimeError(result)
    return result

def download(remote, target):
    task = post('/api/download/prepare', {'paths': remote})['task_id']
    target.parent.mkdir(parents=True, exist_ok=True)
    partial = target.with_name(target.name + '.partial')
    try:
        with urllib.request.urlopen(base + '/api/download?' + urllib.parse.urlencode({'id': task}), timeout=180) as response, partial.open('wb') as output:
            while chunk := response.read(1024 * 1024):
                output.write(chunk)
        partial.replace(target)
    finally:
        post('/api/tasks', {'download_ack': task}, query=True)
    return digest(target)

def rename(remote, name):
    return post('/api/rename', {'path': remote, 'name': name}, query=True)

meta = json.loads((stage / 'BUILD.json').read_text())
if meta.get('ux_revision') != 'cover-vault-v1' or meta.get('mame_scope') != 'arcade-all':
    raise SystemExit('Refusing a package without Cover Vault and the complete arcade backend')
backend = json.loads((root / '.local/arcade-all-backend.json').read_text())
if meta.get('mame_driver_count') != backend.get('driver_count'):
    raise SystemExit('Arcade backend coverage changed')
expected = {}
for line in (stage / 'SHA256SUMS').read_text().splitlines():
    checksum, relative = line.split('  ', 1)
    if relative.startswith('/') or '..' in Path(relative).parts:
        raise SystemExit('Unsafe staged path')
    if digest(stage / relative) != checksum:
        raise SystemExit('Staged checksum mismatch: ' + relative)
    expected[relative] = checksum
expected['SHA256SUMS'] = digest(stage / 'SHA256SUMS')

download(destination + '/SHA256SUMS', backup / 'SHA256SUMS')
old = {}
for line in (backup / 'SHA256SUMS').read_text().splitlines():
    checksum, relative = line.split('  ', 1)
    old[relative] = checksum
old['SHA256SUMS'] = digest(backup / 'SHA256SUMS')
changed = [name for name, checksum in expected.items() if old.get(name) != checksum]
if not changed:
    print('Cover Vault package is already installed.', flush=True)
    raise SystemExit(0)

saved = {}
for relative in changed:
    if relative in old:
        checksum = digest(backup / relative) if relative == 'SHA256SUMS' else download(destination + '/' + relative, backup / relative)
        if checksum != old[relative]:
            raise RuntimeError('Installed file differs from its manifest: ' + relative)
        saved[relative] = checksum
        print('Backed up and verified:', relative, flush=True)
(backup / 'BACKUP.json').write_text(json.dumps({'destination': destination, 'sha256': saved}, indent=2) + '\n')

temporary = {name: str(Path(name).with_name('.cover-vault-' + stamp + '-' + Path(name).name)) for name in changed}
task = post('/api/upload/prepare', {'path': destination, 'src': 'Approved Cover Vault native UI',
            'total': sum((stage / name).stat().st_size for name in changed), 'count': len(changed),
            'rels': '\n'.join(temporary[name] for name in changed),
            'sizes': '\n'.join(str((stage / name).stat().st_size) for name in changed), 'overwrite': '0'})['task_id']
for relative in changed:
    source = stage / relative
    headers = {'Content-Type': 'application/octet-stream', 'X-WFM-Task-ID': str(task),
               'X-WFM-Path': urllib.parse.quote(destination, safe=''),
               'X-WFM-Rel': urllib.parse.quote(temporary[relative], safe=''),
               'X-WFM-Size': str(source.stat().st_size), 'X-WFM-Overwrite': '0'}
    request = urllib.request.Request(base + '/api/upload-file', data=source.read_bytes(), headers=headers)
    with urllib.request.urlopen(request, timeout=240) as response:
        result = json.load(response)
    if not result.get('ok'):
        raise RuntimeError(result)
    print('Staged on console:', relative, source.stat().st_size, 'bytes', flush=True)
post('/api/upload/finish', {'task_id': task}, query=True)
verify = backup / 'verified-new'
for relative in changed:
    if download(destination + '/' + temporary[relative], verify / relative) != expected[relative]:
        raise RuntimeError('Temporary console file failed SHA-256 verification: ' + relative)
    print('Verified new file:', relative, flush=True)

# Assets and metadata first; signed executable last; publish the final manifest.
order = [name for name in changed if name not in ('eboot.bin', 'SHA256SUMS')]
order += [name for name in ('eboot.bin', 'SHA256SUMS') if name in changed]
promoted = []
retained = {}
try:
    for relative in order:
        remote = destination + '/' + relative
        previous = '.before-cover-vault-' + stamp + '-' + Path(relative).name
        if relative in saved:
            rename(remote, previous)
            retained[relative] = str(Path(relative).with_name(previous))
        try:
            rename(destination + '/' + temporary[relative], Path(relative).name)
        except Exception:
            if relative in retained:
                rename(destination + '/' + retained.pop(relative), Path(relative).name)
            raise
        promoted.append(relative)
        print('Activated:', relative, flush=True)
    for relative in changed:
        if download(destination + '/' + relative, verify / ('active-' + relative)) != expected[relative]:
            raise RuntimeError('Active console file failed SHA-256 verification: ' + relative)
        print('Verified active file:', relative, flush=True)
except Exception:
    for relative in reversed(promoted):
        try:
            rename(destination + '/' + relative, '.failed-cover-vault-' + stamp + '-' + Path(relative).name)
            if relative in retained:
                rename(destination + '/' + retained[relative], Path(relative).name)
        except Exception as error:
            print('Rollback needs attention:', relative, str(error), flush=True)
    raise

receipt = {'installed_at': datetime.now(timezone.utc).isoformat(), 'destination': destination,
           'ux_revision': meta['ux_revision'], 'mame_scope': meta['mame_scope'],
           'mame_driver_count': meta['mame_driver_count'], 'backup': str(backup),
           'changed': changed, 'sha256': {name: expected[name] for name in changed},
           'retained_console_files': retained, 'remote_sha256_verified': True,
           'native_host_gameplay_smoke_passed': True, 'console_launch_verified': False,
           'games_settings_saves_preserved': True}
(root / '.local/cover-vault-install.json').write_text(json.dumps(receipt, indent=2) + '\n')
(backup / 'INSTALL.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('Cover Vault installed and verified. Close and reopen Arcade to use the new executable.', flush=True)
