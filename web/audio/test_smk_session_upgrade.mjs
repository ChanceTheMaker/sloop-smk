// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {readFileSync} from 'node:fs';
const oldBytes=execFileSync('git',['show','10eaeeb9286ec92ee644593ddf128f8c4493439b:web/audio/smk-engine.wasm'],{maxBuffer:4*1024*1024});
const old=(await WebAssembly.instantiate(oldBytes)).instance.exports;
const now=(await WebAssembly.instantiate(readFileSync(new URL('./smk-engine.wasm',import.meta.url)))).instance.exports;
old.synth_init();now.synth_init();
const om=new Uint8Array(old.memory.buffer), nm=new Uint8Array(now.memory.buffer);
function cmd(e,m,...args){const b=[125,70,76,...args];m.set(b,e.fw_request());const n=e.fw_command(b.length);assert(n);return m.slice(e.fw_reply(),e.fw_reply()+n);}
cmd(old,om,3,0,1,42,64); // Attack 42, an observable value in the saved instrument.
const ptr=old.fw_snapshot(),legacy=om.slice(ptr,ptr+old.fw_snapshot_size());
assert.equal(legacy.length,now.fw_snapshot_v1_size(),'legacy struct offsets and size must stay compatible');
nm.set(legacy,now.fw_snapshot());assert.equal(now.fw_restore(),0);
assert.equal(cmd(now,nm,2,0,1)[7],42);
const freshPtr=now.fw_snapshot(),fresh=nm.slice(freshPtr,freshPtr+now.fw_snapshot_size());
assert.equal(new DataView(fresh.buffer).getUint32(0,true),0x534c4f32);
assert(fresh.length>legacy.length);
fresh[fresh.length-1]^=1;nm.set(fresh,freshPtr);
assert.equal(now.fw_restore(),-1);assert.equal(cmd(now,nm,2,0,1)[7],42);
console.log('PASS: real published 2.6 WASM session imports into 2.8; upgraded CRC rejects corruption without changing the song');
