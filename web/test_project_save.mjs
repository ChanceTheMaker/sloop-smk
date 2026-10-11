// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync(new URL('./editor.html',import.meta.url),'utf8');
const part=(a,b)=>html.slice(html.indexOf(a),html.indexOf(b,html.indexOf(a)));
let response,messages=[],errors=[];
const c=vm.createContext({console,confirm:()=>true,rq:async()=>response,renderSlots:()=>{},selfLoad:()=>{},afterSoundChange:async()=>{},say:x=>messages.push(x),sayK:x=>messages.push(x),report:e=>errors.push(e.message),t:x=>x});
vm.runInContext(part('/*PROTO-BEGIN*/','/*PROTO-END*/')+`
let dev={slotUsed:[0,0,0,0]},busy=0,browserMode=false;
${part('async function project(op, slot)', '/* ---------------------------------------------------------- steps')}
globalThis.api={project,dev,browser:v=>browserMode=v,busy:()=>busy};`,c);
for(const rc of [1,2,3,4,99]){
 response=[1,0,0,rc];messages=[];errors=[];await c.api.project(1,0);
 assert.equal(errors.length,1);assert.equal(messages.length,0);assert.equal(c.api.dev.slotUsed[0],0);assert.equal(c.api.busy(),0);
}
response=[1,0,1,0];messages=[];errors=[];await c.api.project(1,0);assert.deepEqual(messages,['savedSlot']);assert.equal(errors.length,0);
response=[1,0,1];messages=[];await c.api.project(1,0);assert.match(messages[0],/does not confirm flash persistence/);
c.api.browser(true);messages=[];await c.api.project(1,0);assert.deepEqual(messages,['savedSlot']);
console.log('PASS: device project save failures never report success; b2 missing status disclosed; verified and browser saves accepted');
