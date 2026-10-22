// SPDX-License-Identifier: GPL-3.0-only
// Reuse Sloop's verified public package builders; never accept caller firmware.
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {buildSmkPublicPackage,smkLogicalImage} from '../web/smk37-package.js';
import {buildSmkStock} from '../web/restore-stock/package-smk.js';
import {sha256hex} from '../web/fm1pkg.js';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const [action,mode,out]=process.argv.slice(2);
if(!['install','restore'].includes(action)||mode!=='public'||!out)throw Error('Invalid local installer request');
const stock=new Uint8Array(await fs.readFile(path.join(root,'build/smk37/stock-pro.fwsc')));
const original=await buildSmkStock(stock,'SMK-37 Pro');
let image,packet,expected;
if(action==='restore'){
  image=smkLogicalImage(stock).raw;packet=original.bytes;expected=original.product;
}else{
  const release=JSON.parse(await fs.readFile(path.join(root,'web/firmware/public-release.json'),'utf8'));
  if(release.product!=='SMK-37_954'||release.app!=='sloop-smk-2.8.0-b3-app.bin'||release.sha256!=='93bff5f4ac5f8f91e8ada1cdc8ba7b79b67790e21ecca53ff738095d98b0f7ff')throw Error('Unrecognized Sloop public release');
  const app=new Uint8Array(await fs.readFile(path.join(root,'web/firmware',release.app)));
  const built=await buildSmkPublicPackage(stock,app,release);
  if(built.stockModel!=='SMK-37 Pro')throw Error('Incorrect physical model');
  image=built.image;packet=built.bytes;expected=built.product;
}
await fs.mkdir(out,{recursive:true});
await fs.writeFile(path.join(out,'image.bin'),image);
await fs.writeFile(path.join(out,'packet.bin'),packet);
await fs.writeFile(path.join(out,'verified.json'),JSON.stringify({model:'SMK-37 Pro',expected,image:{bytes:image.length,sha256:await sha256hex(image)},packet:{bytes:packet.length,sha256:await sha256hex(packet)}}));
