// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {Updater} from './fm1ota.js?v=0.9.22-r4';
import {installStockSmk} from './smk37-update.js';
let cfg,sends,serves,confirms,phase,logs;
const link=()=>({send(){sends++;return true;},close(){}});
const mk=(model,version=16)=>({model,version,text:model+'_'+String(version).padStart(3,'0')});
const pkg={format:2,image:new Uint8Array(64),stockModel:'SMK-37 Elite',product:'SMK-37_936',version:'Sloop SMK 2.6.0-b7'};
globalThis.document={getElementById:()=>({checked:cfg.optIn})};
globalThis.confirm=()=>{confirms++;return cfg.confirm;};
Updater.prototype.find=async function(filter){
 const ids=[mk('FM-1'),mk('unrelated'),...(cfg.start==='stock'?[mk(pkg.stockModel)]:cfg.start==='exact'?[mk('ota-'+pkg.stockModel)]:cfg.start==='generic'?[mk('ota-FM-1')]:[])];
 for(const id of ids){if(filter(id)){assert.ok(id.model.startsWith('ota-')||id.model===pkg.stockModel);return {id,name:'test MIDI',link:link()};}}
 return null;
};
Updater.prototype.waitFor=async function(filter){
 const id=phase===1?mk('ota-'+pkg.stockModel):{model:'SMK-37',text:cfg.wrongReturn?'SMK-37_931':pkg.product};
 if(phase===1&&cfg.noLoader)return null;
 assert.ok(filter(id));return {id,link:link()};
};
Updater.prototype.serve=async function(l,image,finish){assert.equal(image,pkg.image);serves.push(finish);phase=finish===0xe0000000?1:2;return {finished:!cfg.failWrite};};
async function test(name,options,expected,error){cfg=options;sends=confirms=phase=0;serves=[];logs=[];pkg.stockModel=options.model||'SMK-37 Elite';const act=()=>installStockSmk({inputs:new Map(),outputs:new Map()},pkg,m=>logs.push(m));if(error)await assert.rejects(act,error);else await act();assert.deepEqual(serves,expected);if(!expected.length)assert.equal(sends,0);console.log('PASS '+name);}
await test('exact loader resumes without stage one',{start:'exact'},[0xf0000000]);
await test('normal stock performs both stages',{start:'stock'},[0xe0000000,0xf0000000]);
await test('missing loader stops before stage two',{start:'stock',noLoader:true},[0xe0000000],/did not appear/);
await test('generic loader disabled by default',{start:'generic'},[],/No programming started/);assert.equal(confirms,0);
await test('generic loader cancelled',{start:'generic',optIn:true,confirm:false},[],/No programming started/);assert.equal(confirms,1);
await test('generic Elite loader explicit opt-in and confirmation',{start:'generic',optIn:true,confirm:true},[0xf0000000]);assert.equal(confirms,1);
await test('generic fallback refused for Pro',{start:'generic',optIn:true,confirm:true,model:'SMK-37 Pro'},[],/No programming started/);assert.equal(confirms,0);
await test('no SMK does not send START',{start:'missing'},[],/No SMK found/);
await test('write failure does not report success',{start:'exact',failWrite:true},[0xf0000000],/did not finish/);
await test('wrong returned firmware rejected',{start:'exact',wrongReturn:true},[0xf0000000],/Expected/);
console.log('No physical MIDI ports were opened.');
