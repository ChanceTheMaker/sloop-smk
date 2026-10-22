# SPDX-License-Identifier: GPL-3.0-only
"""Private, explicitly invoked 2.8 Pro candidate stages; not used by the website."""
import hashlib,json,os,sys,subprocess,tempfile,time
from pathlib import Path

PINNED={'model': 'SMK-37 Pro',
 'expected': 'SMK-37_954',
 'version': 'SLOOP SMK 2.8.0-b3',
 'application_sha256': '93bff5f4ac5f8f91e8ada1cdc8ba7b79b67790e21ecca53ff738095d98b0f7ff',
 'upstream_revision': 'b77bf55e3bf804665b8f5dffb5955bd8443311f0',
 'image': {'bytes': 705216,
           'sha256': '9062569d62eae440a422adcdbbfe5d429a9a3118bb34ec426caf497244d13d29'},
 'packet': {'bytes': 627712,
            'sha256': '28a1cdd53eca83f5300bfe259e9d81fff3a01f2a8ede62f0af4d2b1cbe3c56bb'}}


def validate_candidate(directory,manifest):
    if any(manifest.get(k)!=v for k,v in PINNED.items()):
        raise RuntimeError('Not the reviewed Sloop 2.8 Pro candidate')
    # Verify both payloads before any handoff, even when only one is needed.
    for kind in ('image','packet'):
        data=(directory/(kind+'.bin')).read_bytes();meta=PINNED[kind]
        if len(data)!=meta['bytes'] or hashlib.sha256(data).hexdigest()!=meta['sha256']:
            raise RuntimeError('Candidate payload changed: '+kind)

def install(directory):
    """Explicit local command; hold the shared lock across disposable USB stages."""
    import msvcrt
    from native_smk import emit
    with open(Path(tempfile.gettempdir())/'fm1-smk-native-usb.lock','a+b') as lock:
        lock.seek(0)
        try:msvcrt.locking(lock.fileno(),msvcrt.LK_NBLCK,1)
        except OSError:raise RuntimeError('Another local SMK operation is running')
        def stage(name,expected=''):
            command=[sys.executable,str(Path(__file__).resolve()),name,str(directory.resolve()),expected]
            result=subprocess.run(command,capture_output=True,text=True,timeout=360,
                                  creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
            events=[]
            for line in result.stdout.splitlines():
                event=json.loads(line);events.append(event)
                # Keep progress output concise; retain final status and failures.
                if 'log' in event or 'ok' in event:emit(**event)
            final=next((e for e in reversed(events) if 'ok' in e),{})
            if result.returncode or not final.get('ok'):
                raise RuntimeError(final.get('error') or result.stderr[-1000:] or 'Candidate stage failed')
            return final
        first=stage('begin')
        if first.get('handoff'):time.sleep(1)
        stage('write',first['loader'])
        time.sleep(1)
        stage('verify',PINNED['expected'])

if __name__=='__main__':
    from native_smk import worker,emit
    try:
        if len(sys.argv)<3 or sys.argv[1] not in ('install','begin','write','verify','check'):
            raise RuntimeError('Usage: sloop_candidate.py check|install|begin|write|verify DIRECTORY [EXPECTED_ID]')
        directory=Path(sys.argv[2])
        validate_candidate(directory,json.loads((directory/'verified.json').read_text()))
        if sys.argv[1]=='check':emit(ok=True,log='Reviewed candidate payloads verified; no MIDI opened')
        elif sys.argv[1]=='install':install(directory)
        else:worker(sys.argv[1],directory,sys.argv[3] if len(sys.argv)>3 else '',candidate=True)
        code=0
    except Exception as exc:emit(ok=False,error=str(exc));code=1
    sys.stdout.flush();sys.stderr.flush();os._exit(code)
