#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the SMK website for Pages; never copy private device/build files."""
import argparse
import shutil
import json
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('out', type=Path)
    args = parser.parse_args()
    out = args.out.resolve()
    if out.exists():
        parser.error('Output must be a new directory (no destructive cleanup).')
    out.mkdir(parents=True)
    web = ROOT / 'web'
    for source in web.iterdir():
        if source.is_file() and (source.suffix in ('.js', '.css', '.ttf', '.txt', '.svg', '.png', '.ico') or
                                source.name in ('sloop-preview.wasm', 'index.html', 'editor.html', 'simulator.html', 'midi-diagnostics.html', 'elite-resume.html')):
            shutil.copy2(source, out / source.name)
    for name in ('elite-test', 'elite-resume'):
        (out / name).mkdir()
        for source in (web / name).glob('*.js'):
            shutil.copy2(source, out / name / source.name)
    for name in ('fonts', 'screenshots'):
        shutil.copytree(web / name, out / name)
    (out / 'audio').mkdir()
    for source in (web / 'audio').iterdir():
        if source.is_file() and source.suffix in ('.js', '.wasm', '.json'):
            shutil.copy2(source, out / 'audio' / source.name)
    for name in ('LICENSE', 'LICENSING.md'):
        shutil.copy2(ROOT / name, out / name)
    (out / 'restore-stock').mkdir()
    for asset in (web / 'restore-stock').iterdir():
        if asset.suffix in ('.html', '.css', '.js'):
            shutil.copy2(asset, out / 'restore-stock' / asset.name)
    # Explicit public allowlist: app code only, no private or vendor packages.
    release_path=web/'firmware/public-release.json'
    release=json.loads(release_path.read_text())
    if release.get('public') is not True or release.get('app')!='sloop-smk-2.8.0-b3-app.bin' or release.get('product')!='SMK-37_954':
        raise ValueError('Unsupported public release manifest')
    app=web/'firmware'/release['app'];data=app.read_bytes()
    if len(data)!=release['bytes'] or hashlib.sha256(data).hexdigest()!=release['sha256']:
        raise ValueError('Public application digest mismatch')
    (out/'firmware').mkdir()
    shutil.copy2(release_path,out/'firmware/public-release.json')
    shutil.copy2(app,out/'firmware'/app.name)
    shutil.copytree(ROOT / 'LICENSES', out / 'LICENSES')
    (out / '.nojekyll').write_text('')
    print(f'SMK site: {out}')

if __name__ == '__main__':
    main()
