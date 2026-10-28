#!/usr/bin/env python3
"""Upload only the rebuilt executable and its receipts to the user's existing title."""
from pathlib import Path
import json,urllib.request,urllib.parse
root=Path(__file__).resolve().parents[1];base='http://192.168.0.181:8888';destination='/data/homebrew/PPSA99984'
files=[root/'build/title/PPSA99984'/name for name in ['eboot.bin','BUILD.json','SHA256SUMS']]
def post(path,values):
 body=urllib.parse.urlencode(values).encode();request=urllib.request.Request(base+path,data=body,headers={'Content-Type':'application/x-www-form-urlencoded'})
 with urllib.request.urlopen(request,timeout=30) as response:result=json.load(response)
 if not result.get('ok'):raise RuntimeError(result)
 return result
values={'path':destination,'src':files[0].name,'total':sum(p.stat().st_size for p in files),'count':len(files),'rels':'\n'.join(p.name for p in files),'sizes':'\n'.join(str(p.stat().st_size) for p in files),'overwrite':'1'}
task=post('/api/upload/prepare',values);task_id=task['task_id'];print('Repair upload task:',task_id,flush=True)
for path in files:
 payload=path.read_bytes();headers={'Content-Type':'application/octet-stream','X-WFM-Task-ID':str(task_id),'X-WFM-Path':urllib.parse.quote(destination,safe=''),'X-WFM-Rel':urllib.parse.quote(path.name,safe=''),'X-WFM-Size':str(len(payload)),'X-WFM-Overwrite':'1'}
 request=urllib.request.Request(base+'/api/upload-file',data=payload,headers=headers)
 with urllib.request.urlopen(request,timeout=60) as response:result=response.read().decode()
 print('Uploaded:',path.name,len(payload),'bytes',result,flush=True)
finish=urllib.request.Request(base+'/api/upload/finish?'+urllib.parse.urlencode({'task_id':task_id}),data=b'')
with urllib.request.urlopen(finish,timeout=30) as response:result=json.load(response)
if not result.get('ok'):raise RuntimeError(result)
print('Repair upload finished',flush=True)

# Confirm the server stored the complete executable, rather than trusting upload acknowledgement.
import hashlib
check=post('/api/download/prepare',{'paths':destination+'/eboot.bin'})['task_id']
digest=hashlib.sha256()
with urllib.request.urlopen(base+'/api/download?'+urllib.parse.urlencode({'id':check}),timeout=60) as response:
 while chunk:=response.read(1024*1024):digest.update(chunk)
post('/api/tasks?'+urllib.parse.urlencode({'download_ack':check}),{})
expected=hashlib.sha256(files[0].read_bytes()).hexdigest()
if digest.hexdigest()!=expected:raise RuntimeError('Remote executable does not match local build')
print('Remote executable SHA256 verified:',expected,flush=True)
