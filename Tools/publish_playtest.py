"""Publish a requested Windows playtest through the existing Git credential helper.

Credentials stay in process memory and are never printed or saved to disk.
Only the named project's GitHub repository and upload host receive requests.
"""
import argparse,json,subprocess,urllib.request,urllib.error,hashlib
from pathlib import Path
REPO='ATritle/AnEmberRemains'
parser=argparse.ArgumentParser();parser.add_argument('--inspect',action='store_true');parser.add_argument('--zip',type=Path);parser.add_argument('--notes',type=Path);parser.add_argument('--tag');parser.add_argument('--target');parser.add_argument('--name',default='An Ember Remains playtest');parser.add_argument('--replace-prior',action='append',default=[]);args=parser.parse_args()
result=subprocess.run(['git','credential','fill'],input='protocol=https\nhost=github.com\npath='+REPO+'.git\n\n',capture_output=True,text=True,check=True)
credential=dict(line.split('=',1) for line in result.stdout.splitlines() if '=' in line)
token=credential.get('password');assert token,'GitHub credential unavailable'
headers={'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json','X-GitHub-Api-Version':'2022-11-28','User-Agent':'AER-Playtest-Publisher'}
def api(path,data=None,method=None):
    req=urllib.request.Request('https://api.github.com/repos/'+REPO+path,data=json.dumps(data).encode() if data is not None else None,headers=headers,method=method)
    with urllib.request.urlopen(req,timeout=90) as response:
        return None if response.status==204 else json.load(response)
if args.inspect:
    repo=api('');print(json.dumps({'repository':repo['full_name'],'private':repo['private'],'can_push':repo.get('permissions',{}).get('push')}))
    print(json.dumps([{'tag':r['tag_name'],'draft':r['draft'],'url':r['html_url']} for r in api('/releases?per_page=5')]))
else:
    assert args.zip and args.notes and args.tag and args.target
    notes=args.notes.read_text(encoding='utf-8')
    try:release=api('/releases/tags/'+args.tag)
    except urllib.error.HTTPError as error:
        if error.code!=404:raise
        release=api('/releases',{'tag_name':args.tag,'target_commitish':args.target,'name':args.name,'body':notes,'draft':True,'prerelease':True})
    existing=next((a for a in release['assets'] if a['name']==args.zip.name),None)
    if existing is None:
        import http.client,urllib.parse
        connection=http.client.HTTPSConnection('uploads.github.com',timeout=600)
        path='/repos/'+REPO+'/releases/'+str(release['id'])+'/assets?name='+urllib.parse.quote(args.zip.name)
        connection.putrequest('POST',path)
        for k,v in headers.items():connection.putheader(k,v)
        connection.putheader('Content-Type','application/zip');connection.putheader('Content-Length',str(args.zip.stat().st_size));connection.endheaders()
        with args.zip.open('rb') as stream:
            while block:=stream.read(1024*1024):connection.send(block)
        response=connection.getresponse();payload=response.read()
        if response.status!=201:raise RuntimeError('Upload failed with status '+str(response.status))
        existing=json.loads(payload)
    assert existing['size']==args.zip.stat().st_size,'Uploaded size mismatch'
    with args.zip.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
    remote_digest=existing.get('digest')
    if remote_digest:
        assert remote_digest=='sha256:'+digest,'GitHub SHA-256 mismatch'
    else:
        # Repository is public. No credential is forwarded to the download/CDN URL.
        with urllib.request.urlopen(existing['browser_download_url'],timeout=600) as stream:
            assert hashlib.file_digest(stream,'sha256').hexdigest()==digest,'Download SHA-256 mismatch'
    notes+='\n\nSHA-256: `'+digest+'`\n\nRelease-files commit: `'+args.target+'`\n'

    # Final publication occurs only after the complete downloadable asset exists.
    req=urllib.request.Request('https://api.github.com/repos/'+REPO+'/releases/'+str(release['id']),data=json.dumps({'draft':False,'prerelease':False,'make_latest':'true','name':args.name,'body':notes}).encode(),headers=headers,method='PATCH')
    with urllib.request.urlopen(req,timeout=90) as response:published=json.load(response)
    published_asset=next(a for a in published['assets'] if a['name']==args.zip.name)
    removed=[]
    # Replace only explicitly named prior releases, after the new asset is verified.
    # Retain historical Git tags and commit history.
    for tag in args.replace_prior:
        assert tag!=args.tag
        try:old=api('/releases/tags/'+tag)
        except urllib.error.HTTPError as error:
            if error.code==404:continue
            raise
        api('/releases/'+str(old['id']),method='DELETE');removed.append(tag)
    print(json.dumps({'release':published['html_url'],'download':published_asset['browser_download_url'],'bytes':published_asset['size'],'sha256':digest,'replaced_releases':removed}))
