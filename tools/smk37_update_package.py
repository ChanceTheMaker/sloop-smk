#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Private unit-specific USB update package. No device access. Never publish."""
import argparse,json,hashlib,struct,zlib
from pathlib import Path
from smk37_test_image import replace_application
from smk37_backup import inspect_backup
MAGIC=b'SMK37-SALT-USB1'.ljust(16,b'\0')
LO,HI,HEADER=0x4000,0x9d000,1024

def package(stock,app,product="SMK-37_911",version="SMK37 TEST11 USB"):
    import re
    if not re.fullmatch(r"SMK-37_9[0-9]{2}",product) or not re.fullmatch(r"(?:SMK37 TEST[0-9]{2} USB|SLOOP SMK [0-9.]+-b[0-9]+)",version):raise ValueError("Invalid test identity")
    image=replace_application(stock,app); payload=image[LO:HI]
    h=bytearray(HEADER); h[:16]=MAGIC
    struct.pack_into('<7I',h,16,1,LO,HI,len(payload),zlib.crc32(stock[:LO]),zlib.crc32(stock[0xc1000:0xc1000+556]),zlib.crc32(payload))
    h[48:80]=product.encode('ascii').ljust(32,b'\0');h[80:112]=version.encode('ascii').ljust(32,b'\0')
    for i in range((HI-LO)//4096):struct.pack_into('<I',h,128+i*4,zlib.crc32(payload[i*4096:(i+1)*4096]))
    struct.pack_into('<I',h,44,zlib.crc32(h))
    return bytes(h)+payload,image

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--backups',nargs=2,type=Path,required=True);p.add_argument('--stock-package',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
    stock=a.backups[0].read_bytes();inspect_backup(stock,a.backups[1].read_bytes(),a.stock_package.read_bytes())
    m=json.loads(a.manifest.read_text());app=(a.manifest.parent/m['image']).read_bytes()
    if m['board'] not in ('SMK-37 Pro','SMK-37 Pro & Elite') or not m.get('usb_update_support') or hashlib.sha256(app).hexdigest()!=m['sha256']:raise SystemExit('Not a verified updater-enabled SMK artifact')
    product=m.get('update_product','SMK-37_911');version=m.get('update_version','SMK37 TEST11 USB')
    data,image=package(stock,app,product,version);out=a.manifest.parent
    number=product[-2:]
    name=f'smk37-test{number}-private.smkupdate';(out/name).write_bytes(data)
    (out/f'smk37-test{number}-device-image.bin').write_bytes(image)
    meta={'version':version,'product':product,'package':name,'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'private':True,'hardware_validated':False,'application_sha256':m['sha256'],'image_sha256':hashlib.sha256(image).hexdigest()}
    (out/'web-candidate.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta,indent=2))
if __name__=='__main__':main()
