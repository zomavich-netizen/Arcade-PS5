#!/usr/bin/env python3
"""Install the explicitly requested subset frontend test, preserving existing data."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,urllib.request,urllib.parse,os
root=Path(__file__).resolve().parents[1];stage=Path(os.environ.get('ARCADE_TEST_STAGE',str(root/'build/title/PPSA99984')))
base='http://192.168.0.181:8888';destination='/data/homebrew/PPSA99984'
meta=json.loads((stage/'BUILD.json').read_text())
if meta.get('mame_scope')!='subset' or not meta.get('vulkan',{}).get('compiled'):raise SystemExit('Expected the verified subset settings/Vulkan test package')
files={str(p.relative_to(stage)):p for p in sorted(stage.rglob('*')) if p.is_file() and p.relative_to(stage).parts[0] not in ('roms','bios')}
for line in (stage/'SHA256SUMS').read_text().splitlines():
 expected,relative=line.split('  ',1)
 if hashlib.file_digest(files[relative].open('rb'),'sha256').hexdigest()!=expected:raise SystemExit('Staged checksum mismatch: '+relative)
for source in files.values():
 if not source.is_file():raise SystemExit('Missing input: '+str(source))
def post(path,values):
 request=urllib.request.Request(base+path,data=urllib.parse.urlencode(values).encode())
 with urllib.request.urlopen(request,timeout=30) as response:result=json.load(response)
 if not result.get('ok'):raise RuntimeError(result)
 return result
def listing(path):return post('/api/list?'+urllib.parse.urlencode({'path':path}),{})['entries']
def download(path,target=None):
 task=post('/api/download/prepare',{'paths':path})['task_id'];digest=hashlib.sha256()
 output=None
 if target:target.parent.mkdir(parents=True,exist_ok=True);output=target.open('wb')
 try:
  with urllib.request.urlopen(base+'/api/download?'+urllib.parse.urlencode({'id':task}),timeout=120) as response:
   while chunk:=response.read(1024*1024):
    digest.update(chunk)
    if output:output.write(chunk)
 finally:
  if output:output.close()
 post('/api/tasks?'+urllib.parse.urlencode({'download_ack':task}),{})
 return digest.hexdigest()
backup=root/'.local/console-backups'/('before-settings-test-'+datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True,exist_ok=False);saved={};directories={}
for relative in files:
 parent=str(Path(relative).parent);remote_parent=destination+('/'+parent if parent!='.' else '')
 if parent not in directories:
  try:directories[parent]={entry['name']:entry for entry in listing(remote_parent)}
  except urllib.error.HTTPError as error:
   if error.code!=404:raise
   directories[parent]={}
 if Path(relative).name in directories[parent]:
  saved[relative]=download(destination+'/'+relative,backup/relative)
(backup/'BACKUP.json').write_text(json.dumps({'destination':destination,'sha256':saved},indent=2)+'\n')
print('Working installation backed up:',backup,len(saved),'files',flush=True)
# Switch the executable last. Existing settings and original ROM folders are not touched.
order=[name for name in files if name!='eboot.bin']+['eboot.bin']
task=post('/api/upload/prepare',{'path':destination,'src':'Arcade settings and Vulkan subset test','total':sum(p.stat().st_size for p in files.values()),'count':len(files),'rels':'\n'.join(order),'sizes':'\n'.join(str(files[name].stat().st_size) for name in order),'overwrite':'1'})['task_id']
for relative in order:
 path=files[relative];payload=path.read_bytes()
 headers={'Content-Type':'application/octet-stream','X-WFM-Task-ID':str(task),'X-WFM-Path':urllib.parse.quote(destination,safe=''),'X-WFM-Rel':urllib.parse.quote(relative,safe=''),'X-WFM-Size':str(len(payload)),'X-WFM-Overwrite':'1'}
 request=urllib.request.Request(base+'/api/upload-file',data=payload,headers=headers)
 with urllib.request.urlopen(request,timeout=120) as response:result=json.load(response)
 if not result.get('ok'):raise RuntimeError(result)
 print('Uploaded:',relative,len(payload),'bytes',flush=True)
post('/api/upload/finish?'+urllib.parse.urlencode({'task_id':task}),{})
for relative in order:
 expected=hashlib.file_digest(files[relative].open('rb'),'sha256').hexdigest()
 if download(destination+'/'+relative)!=expected:raise RuntimeError('Remote checksum mismatch: '+relative)
 print('Verified:',relative,flush=True)
record={'scope':'subset frontend test','destination':destination,'backup':str(backup),'eboot_sha256':hashlib.file_digest(files['eboot.bin'].open('rb'),'sha256').hexdigest(),'files':len(files),'remote_checksums_verified':True,'console_launch_tested':False}
(root/'.local/settings-test-install.json').write_text(json.dumps(record,indent=2)+'\n')
print('Settings/Vulkan test installation verified. Original ROM folder and user settings retained.',flush=True)
