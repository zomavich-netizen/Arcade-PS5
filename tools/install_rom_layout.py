#!/usr/bin/env python3
"""Install a rebuilt title; external game data lives under /data/arcade.
Run only with authorization to update the console installation. Old ROMs are retained.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,urllib.request,urllib.parse
root=Path(__file__).resolve().parents[1]
base='http://192.168.0.181:8888';destination='/data/homebrew/PPSA99984'
staged=root/'build/title/PPSA99984'
checksums=(staged/'SHA256SUMS').read_text().splitlines()
# Install only signed-off package files, excluding stray local/sync copies.
files={relative:staged/relative for _,relative in (line.split('  ',1) for line in checksums)}
files['SHA256SUMS']=staged/'SHA256SUMS'
def post(path,values):
 request=urllib.request.Request(base+path,data=urllib.parse.urlencode(values).encode())
 with urllib.request.urlopen(request,timeout=30) as response:result=json.load(response)
 if not result.get('ok'):raise RuntimeError(result)
 return result
# Refuse to install the old subset after the user requested all arcade drivers.
backend=json.loads((root/'build/ps5/backend.json').read_text())
full=json.loads((root/'.local/arcade-all-backend.json').read_text())
if backend.get('scope')!='arcade-all' or backend.get('driver_count')!=full.get('driver_count') or backend.get('entry_archive')!=full.get('entry_archive'):
 raise SystemExit('The staged executable must be linked with the verified complete arcade MAME backend before installation')
for line in checksums:
 expected,relative=line.split('  ',1)
 if relative not in files or hashlib.file_digest(files[relative].open('rb'),'sha256').hexdigest()!=expected:
  raise SystemExit('Staged package checksum mismatch: '+relative)
for source in files.values():
 if not source.is_file():raise SystemExit('Missing installation input: '+str(source))
backup=root/'.local/console-backups'/('before-arcade-update-'+datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S'));backup.mkdir(parents=True,exist_ok=False)
for relative in ('eboot.bin','BUILD.json','SHA256SUMS','sce_module/libc.prx'):
 target=backup/relative
 if target.exists():continue
 task=post('/api/download/prepare',{'paths':destination+'/'+relative})['task_id']
 target.parent.mkdir(parents=True,exist_ok=True)
 temporary=target.with_name(target.name+'.partial')
 with urllib.request.urlopen(base+'/api/download?'+urllib.parse.urlencode({'id':task}),timeout=90) as response,temporary.open('wb') as output:
  while chunk:=response.read(1024*1024):output.write(chunk)
 post('/api/tasks?'+urllib.parse.urlencode({'download_ack':task}),{})
 temporary.replace(target)
 print('Backed up installed file:',relative,flush=True)
task=post('/api/upload/prepare',{'path':destination,'src':'Arcade ROM and BIOS layout','total':sum(p.stat().st_size for p in files.values()),'count':len(files),'rels':'\n'.join(files),'sizes':'\n'.join(str(p.stat().st_size) for p in files.values()),'overwrite':'1'})['task_id']
for relative,source in files.items():
 payload=source.read_bytes()
 headers={'Content-Type':'application/octet-stream','X-WFM-Task-ID':str(task),'X-WFM-Path':urllib.parse.quote(destination,safe=''),'X-WFM-Rel':urllib.parse.quote(relative,safe=''),'X-WFM-Size':str(len(payload)),'X-WFM-Overwrite':'1'}
 with urllib.request.urlopen(urllib.request.Request(base+'/api/upload-file',data=payload,headers=headers),timeout=90) as response:
  result=json.load(response)
 if not result.get('ok'):raise RuntimeError(result)
 print('Uploaded:',relative,len(payload),'bytes',flush=True)
post('/api/upload/finish?'+urllib.parse.urlencode({'task_id':task}),{})
for relative,source in files.items():
 task=post('/api/download/prepare',{'paths':destination+'/'+relative})['task_id'];digest=hashlib.sha256()
 with urllib.request.urlopen(base+'/api/download?'+urllib.parse.urlencode({'id':task}),timeout=90) as response:
  while chunk:=response.read(1024*1024):digest.update(chunk)
 post('/api/tasks?'+urllib.parse.urlencode({'download_ack':task}),{})
 if digest.hexdigest()!=hashlib.file_digest(source.open('rb'),'sha256').hexdigest():raise RuntimeError('Remote checksum mismatch: '+relative)
 print('Verified SHA256:',relative,flush=True)
receipt={'scope':backend['scope'],'driver_count':backend['driver_count'],'destination':destination,'remote_sha256_verified':True,'backup':str(backup),'files':{relative:hashlib.file_digest(source.open('rb'),'sha256').hexdigest() for relative,source in files.items()}}
(root/'.local/arcade-all-install.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Installed title; external /data/arcade content preserved.',flush=True)
