// SPDX-License-Identifier: GPL-3.0-only
// Offline candidate packaging only. Never opens MIDI/USB or changes the live site.
import fs from 'node:fs/promises';
import path from 'node:path';
import {buildSmkPublicPackage} from '../web/smk37-package.js';
import {sha256hex} from '../web/fm1pkg.js';
const [manifestPath,stockPath,output]=process.argv.slice(2);
if(!manifestPath||!stockPath||!output)throw Error('Usage: node tools/package_sloop_candidate.mjs MANIFEST STOCK_FWSC NEW_OUTPUT_DIR');
const m=JSON.parse(await fs.readFile(manifestPath,'utf8'));
if(!m.public_build||m.upstream_version!=='2.8'||m.update_product!=='SMK-37_952'||m.update_version!=='SLOOP SMK 2.8.0-b2')throw Error('Unexpected candidate identity');
const app=new Uint8Array(await fs.readFile(path.join(path.dirname(manifestPath),m.image)));
const stock=new Uint8Array(await fs.readFile(stockPath));
const release={public:true,product:m.update_product,version:m.update_version,bytes:m.bytes,sha256:m.sha256};
const built=await buildSmkPublicPackage(stock,app,release);
await fs.mkdir(output,{recursive:false});
for(const [name,bytes] of [['image.bin',built.image],['packet.bin',built.bytes],['candidate.fwsc',built.factory]])await fs.writeFile(path.join(output,name),bytes);
const verified={model:built.stockModel,expected:built.product,version:built.version,hardware_validated:false,application_sha256:m.sha256,upstream_revision:m.upstream_revision};
for(const [name,bytes] of [['image',built.image],['packet',built.bytes]])verified[name]={bytes:bytes.length,sha256:await sha256hex(bytes)};
await fs.writeFile(path.join(output,'verified.json'),JSON.stringify(verified,null,2)+'\n');
console.log(JSON.stringify(verified,null,2));
